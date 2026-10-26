/*
 * platform/midi_dsp.c – C implementations of the MIDI library ARM assembly DSP
 * routines (lib_midi/pcm_buffer.s, pcm_read*.s, pcm_equalize.s).
 *
 * On GBA these run as hand-optimised ARM/Thumb code copied to IWRAM.
 * On PC we replace them with equivalent C that produces the same output.
 *
 * Call chain (from midi_directsound_update):
 *   midi_asm_update_scratch   – pre-fill scratch with reverb signal
 *   midi_asm_read_pcm_fixed   – mix fixed-rate PCM channel into scratch
 *   midi_asm_read_pcm_fast    – mix resampled PCM (no interpolation)
 *   midi_asm_read_pcm_accurate– mix resampled PCM (linear interpolation)
 *   midi_asm_apply_eq         – apply 1-pole IIR EQ to scratch
 *   midi_asm_update_buffer    – convert scratch → 8-bit PCM output buffers
 *
 * On GBA the midi_asm_* symbols are global function-pointer variables
 * (type ThumbFunc = void (*)()) that are installed at runtime by
 * midi_asm_init_mode / midi_asm_init_table.
 *
 * On PC we provide static implementations with pc_ prefix, then assign
 * them to the global ThumbFunc variables so the existing call sites work.
 *
 * Scratch buffer layout: interleaved stereo s32 pairs
 *   [R_s0, L_s0, R_s1, L_s1, ..., R_s(N-1), L_s(N-1)]
 * where N = wordBatchSize * 4 sample-pairs per call.
 *
 * gMidiPCMBufR/L: circular ring buffers of u8 samples packed 4-per-u32,
 *   little-endian.  gMidiPCMBufWritePos counts u32 words.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "src/midi/midi.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* ── Externals from midi_globals.c ───────────────────────────────────────── */
extern volatile u32 *gMidiPCMBufR;
extern volatile u32 *gMidiPCMBufL;
extern volatile u32  gMidiPCMBufSize32;
extern volatile u32  gMidiPCMBufWritePos;
extern s32          *gMidiSampleScratch;
extern s8            gMidiSampleTable[0x400];
extern u16           gMidiSamplerGain;
extern u32           gMidiReverb1Wet;
extern u32           gMidiReverb2Phase;
extern u32           gMidiReverb3Decay;
extern u32           gMidiReverb4LowCut;
extern s32           gMidiReverbScratch[4];
extern u32           gPcMixMult;   /* internal mixer rate / 13379 */

/* ── High-resolution output tap ──────────────────────────────────────────────
 * The GBA's DirectSound DAC is 8-bit, so pc_midi_update_buffer quantises the
 * s32 mix accumulator down to one byte per sample.  That quantisation is the
 * loudest noise source in the whole chain (~48 dB SNR); everything the mixer
 * computed above it is thrown away.
 *
 * These buffers carry the same samples through the *same* transfer curve — the
 * 10-bit wrap and the [-128,127] clamp that gMidiSampleTable implements — but
 * evaluated 128x finer and scaled to full s16 range.  Loud passages therefore
 * still wrap and clip exactly as they do on hardware; only the quantisation
 * step shrinks.  audio_pc.c reads these instead of the packed 8-bit ring.    */
#define PC_HIRES_SAMPLES (1568 * 4)
/* pc-enhanced: the tap holds the raw accumulator value; pc_hires_gba()
 * applies the GBA's wrap and clamp when reading, and audio_quality=high reads
 * it without them (no clipping, scaled the same: x * 2). */
s32 gPcPcmHiR[PC_HIRES_SAMPLES];
s32 gPcPcmHiL[PC_HIRES_SAMPLES];
u32 gPcPcmHiValid = 0;   /* number of samples the ring actually holds */

/* Faithful high-resolution version of gMidiSampleTable[(x >> 7) & 0x3FF].
 * The table wraps its input to 10 signed bits before clamping, so a mix that
 * overflows folds to the opposite rail — reproduce that, at 128x resolution. */
static inline s16 pc_scratch_to_s16(s32 x)
{
    /* Wrap to [-65536, 65535]: the x-domain width of the table's 10-bit index */
    s32 y = (s32)(((u32)x + 65536u) & 0x1FFFFu) - 65536;
    if      (y >  16383) y =  16383;   /* == +127 in the 8-bit domain */
    else if (y < -16384) y = -16384;   /* == -128 */
    return (s16)(y * 2);
}

s16 pc_hires_gba(s32 x)
{
    return pc_scratch_to_s16(x);
}

extern int pc_audio_hq(void);

/* ── gMidiSampleTable init ───────────────────────────────────────────────── */
/*
 * The GBA's midi_directsound_init fills gMidiSampleTable with a saturating
 * signed 10-bit → signed 8-bit mapping.  For index i (0..0x3FF):
 *   sign-extend i as 10-bit signed → clamp to [-128, 127].
 *
 * Reconstructed from asm_08049490.s lines 118-158.
 */
void midi_dsp_init_sample_table(void)
{
    for (int i = 0; i <= 0x3FE; i++) {
        /* Sign-extend 10-bit value */
        s32 v = (s32)((u32)i << 22) >> 22;
        if      (v >  127) v =  127;
        else if (v < -128) v = -128;
        gMidiSampleTable[i] = (s8)v;
    }
    /* GBA code explicitly stores 0 at table[0x3FF] after the loop */
    gMidiSampleTable[0x3FF] = 0;
}

/* ── pc_midi_init_mode ───────────────────────────────────────────────────── */
/*
 * On GBA: copies ARM DSP blobs to IWRAM and installs function pointers.
 * On PC: just initialise the sample table (function pointers are set below
 * as static initialisers).
 */
static void pc_midi_init_mode(void)
{
    midi_dsp_init_sample_table();
}

/* ── pc_midi_update_scratch ──────────────────────────────────────────────── */
/*
 * Fill the scratch accumulator with the reverb wet signal.
 * Reads from gMidiPCMBufR/L at a delayed position, applies a 2-pole IIR
 * filter, multiplies by wet gain, and SETS (does not accumulate) scratch.
 * When wet == 0 this zeroes the scratch.
 */
static void pc_midi_update_scratch_hirate(u32 wordBatchSize);

static void pc_midi_update_scratch(u32 wordBatchSize)
{
    if (wordBatchSize == 0) return;
    if (pc_audio_hq()) {
        /* audio_quality=high: platform/reverb_pc.c replaces this echo, fed
         * from the finished mix at gMidiReverb1Wet. */
        memset(gMidiSampleScratch, 0, wordBatchSize * 4 * 2 * sizeof(s32));
        return;
    }
    if (gPcMixMult > 1) { pc_midi_update_scratch_hirate(wordBatchSize); return; }

    u32 wet    = gMidiReverb1Wet >> gMidiReverb3Decay;
    u32 lowCut = gMidiReverb4LowCut;
    u32 decay  = gMidiReverb3Decay;

    u32 bufSizeBytes = gMidiPCMBufSize32 * 4;
    u32 delayPos     = (gMidiPCMBufWritePos * 4 + gMidiReverb2Phase * 4 * gPcMixMult) % bufSizeBytes;

    const s8 *pcmR_start = (const s8 *)gMidiPCMBufR;
    const s8 *pcmL_start = (const s8 *)gMidiPCMBufL;
    const s8 *rPtr       = pcmR_start + delayPos;
    const s8 *lPtr       = pcmL_start + delayPos;
    const s8 *pcmR_end   = pcmR_start + bufSizeBytes;
    const s8 *pcmL_end   = pcmL_start + bufSizeBytes;

    /* 2-pole IIR state persisted across calls */
    s32 f1r = gMidiReverbScratch[0];
    s32 f2r = gMidiReverbScratch[1];
    s32 f1l = gMidiReverbScratch[2];
    s32 f2l = gMidiReverbScratch[3];

    s32 *scratch    = gMidiSampleScratch;
    u32  numSamples = wordBatchSize * 4;

    for (u32 i = 0; i < numSamples; i++) {
        /* R channel */
        s32 sr = *rPtr++;
        if (rPtr >= pcmR_end) rPtr = pcmR_start;
        f1r     = f1r - (f1r >> lowCut) + sr;
        s32 hpR = sr  - (f1r >> lowCut);
        f2r     = f2r - (f2r >> decay)  + hpR;
        *scratch++ = (s32)wet * f2r;

        /* L channel */
        s32 sl = *lPtr++;
        if (lPtr >= pcmL_end) lPtr = pcmL_start;
        f1l     = f1l - (f1l >> lowCut) + sl;
        s32 hpL = sl  - (f1l >> lowCut);
        f2l     = f2l - (f2l >> decay)  + hpL;
        *scratch++ = (s32)wet * f2l;
    }

    gMidiReverbScratch[0] = f1r;
    gMidiReverbScratch[1] = f2r;
    gMidiReverbScratch[2] = f1l;
    gMidiReverbScratch[3] = f2l;
}

/* The same reverb when the mixer runs at gPcMixMult x 13379 Hz (pc-enhanced).
 * The integer filters above are one-pole sections whose poles are fixed by
 * shifts (1 - 2^-lowCut, 1 - 2^-decay), so at a higher rate their corners
 * would move up by the same factor and the reverb would turn brighter and
 * thinner. Here each pole is a^(1/mult), which keeps the corner frequencies
 * the GBA has, and each section keeps unity DC gain as the shift form does
 * (f >> k with f = a f + x is (1 - a) * f). The delayed signal is read from
 * the high-resolution tap instead of the 8-bit ring, so the reverb no longer
 * carries the DAC's quantisation noise back into the mix. */
static float s_rv_lp1[2], s_rv_lp2[2];

static void pc_midi_update_scratch_hirate(u32 wordBatchSize)
{
    static u32 s_lc = ~0u, s_dc = ~0u, s_mult = 0;
    static float a1, a2;
    u32 lowCut = gMidiReverb4LowCut, decay = gMidiReverb3Decay;
    float wet = (float)((gMidiReverb1Wet >> decay) << decay);   /* the GBA truncates wet >> decay */
    u32 bufSize = gMidiPCMBufSize32 * 4;
    u32 pos = (gMidiPCMBufWritePos * 4 + gMidiReverb2Phase * 4 * gPcMixMult) % bufSize;
    s32 *scratch = gMidiSampleScratch;
    u32 n = wordBatchSize * 4, i;
    int hires = (gPcPcmHiValid == bufSize);

    if (lowCut != s_lc || decay != s_dc || gPcMixMult != s_mult) {
        a1 = (float)pow(1.0 - ldexp(1.0, -(int)lowCut), 1.0 / gPcMixMult);
        a2 = (float)pow(1.0 - ldexp(1.0, -(int)decay), 1.0 / gPcMixMult);
        s_lc = lowCut; s_dc = decay; s_mult = gPcMixMult;
    }
    if (wet == 0.0f) {
        memset(scratch, 0, n * 2 * sizeof(s32));
        s_rv_lp1[0] = s_rv_lp1[1] = s_rv_lp2[0] = s_rv_lp2[1] = 0.0f;
        return;
    }
    for (i = 0; i < n; i++) {
        float x[2];
        int c;
        if (hires) {
            x[0] = pc_scratch_to_s16(gPcPcmHiR[pos]) * (1.0f / 256.0f);
            x[1] = pc_scratch_to_s16(gPcPcmHiL[pos]) * (1.0f / 256.0f);
        } else {
            x[0] = ((const s8 *)gMidiPCMBufR)[pos];
            x[1] = ((const s8 *)gMidiPCMBufL)[pos];
        }
        if (++pos >= bufSize) pos = 0;
        for (c = 0; c < 2; c++) {
            float hp;
            s_rv_lp1[c] = a1 * s_rv_lp1[c] + (1.0f - a1) * x[c];
            hp = x[c] - s_rv_lp1[c];
            s_rv_lp2[c] = a2 * s_rv_lp2[c] + (1.0f - a2) * hp;
            *scratch++ = (s32)(wet * s_rv_lp2[c]);
        }
    }
}

/* ── pc_midi_update_buffer ───────────────────────────────────────────────── */
/*
 * Convert scratch accumulator to packed 8-bit PCM and write to ring buffers.
 * Per word: consume 8 scratch values (R0,L0,R1,L1,R2,L2,R3,L3), look up
 * each through gMidiSampleTable, pack 4 R bytes and 4 L bytes into u32 words.
 */
static void pc_midi_update_buffer(u32 wordBatchSize)
{
    if (wordBatchSize == 0) return;

    u32           writePos = gMidiPCMBufWritePos;
    u32           bufSize  = gMidiPCMBufSize32;
    volatile u32 *pcmR     = gMidiPCMBufR;
    volatile u32 *pcmL     = gMidiPCMBufL;
    const s8     *table    = gMidiSampleTable;
    const s32    *scratch  = gMidiSampleScratch;

    for (u32 w = 0; w < wordBatchSize; w++) {
        u32 rWord = 0, lWord = 0;
        for (u32 s = 0; s < 4; s++) {
            /* Logical right-shift by 7, then mask to 10-bit table index */
            u32 idxR = ((u32)(*scratch++) >> 7) & 0x3FFu;
            u32 idxL = ((u32)(*scratch++) >> 7) & 0x3FFu;
            rWord |= (u32)(u8)table[idxR] << (s * 8);
            lWord |= (u32)(u8)table[idxL] << (s * 8);
        }
        u32 pos = writePos + w;
        if (pos >= bufSize) pos -= bufSize;
        pcmR[pos] = rWord;
        pcmL[pos] = lWord;

        /* Same four samples again, without the 8-bit quantisation. */
        if (pos * 4 + 3 < PC_HIRES_SAMPLES) {
            const s32 *hi = scratch - 8;   /* rewind over the 8 values consumed */
            for (u32 s = 0; s < 4; s++) {
                gPcPcmHiR[pos * 4 + s] = hi[s * 2];
                gPcPcmHiL[pos * 4 + s] = hi[s * 2 + 1];
            }
        }
    }
    gPcPcmHiValid = (bufSize * 4 <= PC_HIRES_SAMPLES) ? bufSize * 4 : 0;
}

/* ── Replacement samples (pc-enhanced, platform/sample_replace.c) ────────
 * midi_sampler_load reports each sample it starts; when there is a
 * replacement file the stream's loop points are moved into the file's
 * sample units and pc_mix_stream reads the file's s16 data instead, with
 * the step scaled by its rate ratio (same pitch). */
#include "sample_replace.h"
extern struct SampleStream *gMidiSamplerPool;
#define PC_MAX_STREAMS 32
static const struct PcReplacement *s_stream_repl[PC_MAX_STREAMS];

void pc_sampler_loaded(u32 id, const struct SampleData *sample)
{
    const struct PcReplacement *r = pc_sample_replacement(sample);
    struct SampleStream *stream = &gMidiSamplerPool[id];
    if (id >= PC_MAX_STREAMS) return;
    s_stream_repl[id] = r;
    if (r) {
        stream->loopStart = r->loopStart << r->fbits;
        stream->loopEnd   = r->loopEnd << r->fbits;
        stream->length    = r->length / 4;
    }
}

static u32 pc_mix_replacement(u32 wordBatchSize, struct SampleStream *stream, const struct PcReplacement *r)
{
    s32 vol   = (s32)(u8)stream->volume + (s32)(u16)gMidiSamplerGain;
    float gr  = (float)(((s32)stream->rightBias * vol) >> 7) * (1.0f / 256.0f);
    float gl  = (float)(((s32)stream->leftBias  * vol) >> 7) * (1.0f / 256.0f);
    u32 pos = stream->position, loopEnd = stream->loopEnd, loopStart = stream->loopStart;
    int fb = r->fbits;
    u32 freq = (u32)lround((double)stream->frequency * r->ratio / (double)(1 << (14 - fb)));
    u32 mask = (1u << fb) - 1;
    float fscale = 1.0f / (float)(1 << fb);
    s32 end = (s32)(loopEnd >> fb), lst = (s32)(loopStart >> fb);
    int loops = loopStart < loopEnd;
    const s16 *w = r->data;
    s32 *scratch = gMidiSampleScratch;
    u32 n = wordBatchSize * 4, i, active = 1;

    for (i = 0; i < n; i++) {
        s32 ip = (s32)(pos >> fb), k;
        float y[4], t = (float)(pos & mask) * fscale, v;
        for (k = 0; k < 4; k++) {
            s32 j = ip - 1 + k;
            if (j < 0) j = 0;
            if (j >= end) {
                if (!loops) { y[k] = 0.0f; continue; }
                j = lst + (j - end) % (end - lst);
            }
            y[k] = w[j];
        }
        v = y[1] + 0.5f * t * (y[2] - y[0] + t * (2.0f * y[0] - 5.0f * y[1] + 4.0f * y[2] - y[3]
                               + t * (3.0f * (y[1] - y[2]) + y[3] - y[0])));
        scratch[i * 2]     += (s32)lrintf(v * gr);
        scratch[i * 2 + 1] += (s32)lrintf(v * gl);
        pos += freq;
        if (pos >= loopEnd) {
            if (loops) {
                do { pos += loopStart - loopEnd; } while (pos >= loopEnd);
            } else {
                active = 0;
                break;
            }
        }
    }
    stream->position = pos;
    return active;
}

/* ── Core PCM channel mixer ──────────────────────────────────────────────── */
/*
 * Mix one SampleStream into the scratch accumulator (accumulate, not set).
 *
 * position is Q18.14 fixed-point; (position >> 14) = integer sample index.
 * Advances by stream->frequency per output sample.
 *
 * interpolate=0 → nearest-neighbour (fast / fixed)
 * interpolate=1 → linear interpolation (accurate)
 *
 * Returns 1 if still active, 0 if one-shot finished.
 */
static u32 pc_mix_stream(u32 wordBatchSize, struct SampleStream *stream, u32 interpolate)
{
    const s8 *waveform = (const s8 *)stream->sample;
    if (!waveform) return 0;
    {
        u32 id = (u32)(stream - gMidiSamplerPool);
        if (id < PC_MAX_STREAMS && s_stream_repl[id])
            return pc_mix_replacement(wordBatchSize, stream, s_stream_repl[id]);
    }

    /* Scale bias by (volume + gain), then >> 7, matching GBA ASM */
    s32 vol   = (s32)(u8)stream->volume + (s32)(u16)gMidiSamplerGain;
    s32 biasR = ((s32)stream->rightBias * vol) >> 7;
    s32 biasL = ((s32)stream->leftBias  * vol) >> 7;

    u32 pos       = stream->position;
    u32 loopEnd   = stream->loopEnd;
    u32 loopStart = stream->loopStart;
    u32 freq      = stream->frequency;

    s32 *scratch    = gMidiSampleScratch;
    u32  numSamples = wordBatchSize * 4;
    u32  active     = 1;

    if (interpolate && pc_audio_hq()) {
        /* audio_quality=high: 4-point cubic (Catmull-Rom) instead of linear,
         * which keeps more of the sample's top end and leaves fewer images.
         * Neighbours past the end wrap into the loop, or read silence for a
         * one-shot sample, instead of reading past the waveform. */
        s32 end   = (s32)(loopEnd >> 14);
        s32 lst   = (s32)(loopStart >> 14);
        int loops = loopStart < loopEnd;
        for (u32 i = 0; i < numSamples; i++) {
            s32 ip = (s32)(pos >> 14), k;
            float y[4], t = (float)(pos & 0x3FFFu) * (1.0f / 16384.0f), v;
            for (k = 0; k < 4; k++) {
                s32 j = ip - 1 + k;
                if (j < 0) j = 0;
                if (j >= end) {
                    if (!loops) { y[k] = 0.0f; continue; }
                    j = lst + (j - end) % (end - lst > 0 ? end - lst : 1);
                }
                y[k] = waveform[j];
            }
            v = y[1] + 0.5f * t * (y[2] - y[0] + t * (2.0f * y[0] - 5.0f * y[1] + 4.0f * y[2] - y[3]
                                   + t * (3.0f * (y[1] - y[2]) + y[3] - y[0])));
            scratch[i * 2]     += (s32)lrintf(v * (float)biasR);
            scratch[i * 2 + 1] += (s32)lrintf(v * (float)biasL);
            pos += freq;
            if (pos >= loopEnd) {
                if (loops) {
                    do { pos += loopStart - loopEnd; } while (pos >= loopEnd);
                } else {
                    active = 0;
                    break;
                }
            }
        }
        stream->position = pos;
        return active;
    }

    for (u32 i = 0; i < numSamples; i++) {
        u32 intPos = pos >> 14;
        s32 sample;

        if (interpolate) {
            s32 s0   = waveform[intPos];
            s32 s1   = waveform[intPos + 1];
            s32 frac = (s32)(pos & 0x3FC0u); /* bits [13:6] */
            sample   = s0 + ((frac * (s1 - s0)) >> 14);
        } else {
            sample = waveform[intPos];
        }

        scratch[i * 2]     += sample * biasR;
        scratch[i * 2 + 1] += sample * biasL;

        pos += freq;

        if (pos >= loopEnd) {
            if (loopStart < loopEnd) {
                do { pos += loopStart - loopEnd; } while (pos >= loopEnd);
            } else {
                active = 0;
                break;
            }
        }
    }

    stream->position = pos;
    return active;
}

/* ── Public PCM-read shims ───────────────────────────────────────────────── */

/* The GBA picks this nearest-neighbour reader for streams flagged fastRead,
 * purely to save CPU — it is the same mixer as the "accurate" one with the
 * interpolation switched off, so a pitch-shifted instrument aliases.  A PC has
 * the cycles to spare, so interpolate here too.  RTPC_AUDIO_FASTREAD=1 restores
 * the original nearest-neighbour behaviour for comparison. */
static void pc_midi_read_pcm_fast(u32 wordBatchSize, struct SampleStream *stream)
{
    static int nearest = -1;
    if (nearest < 0) {
        const char *e = getenv("RTPC_AUDIO_FASTREAD");
        nearest = (e && atoi(e)) ? 1 : 0;
    }
    if (wordBatchSize == 0 || !stream->active) return;
    stream->active = (u8)pc_mix_stream(wordBatchSize, stream, !nearest);
}

static void pc_midi_read_pcm_accurate(u32 wordBatchSize, struct SampleStream *stream)
{
    if (wordBatchSize == 0 || !stream->active) return;
    stream->active = (u8)pc_mix_stream(wordBatchSize, stream, 1);
}

static void pc_midi_read_pcm_fixed(u32 wordBatchSize, struct SampleStream *stream)
{
    if (wordBatchSize == 0 || !stream->active) return;
    if (gPcMixMult > 1) {
        /* A fixed-rate stream steps one sample per mixer sample (0x4000), i.e.
         * it plays at the GBA's 13379 Hz. At gPcMixMult x that rate it would
         * play mult times too fast, so step 1/mult of a sample and
         * interpolate. (A stream whose computed step happens to be exactly
         * 0x4000 also lands here; at 40 kHz and up no sample is that rate.) */
        u32 freq = stream->frequency;
        stream->frequency = (freq + gPcMixMult / 2) / gPcMixMult;
        stream->active = (u8)pc_mix_stream(wordBatchSize, stream, 1);
        stream->frequency = freq;
        return;
    }
    stream->active = (u8)pc_mix_stream(wordBatchSize, stream, 0);
}

/* ── pc_midi_apply_eq ────────────────────────────────────────────────────── */
/*
 * Single-pole IIR equalizer applied to the scratch buffer in-place.
 * eqArea[0]: 0–127 = low-pass, 128–255 = high-pass.
 * eqArea[4..7]: R channel state (s32), eqArea[8..11]: L channel state (s32).
 */
static void pc_midi_apply_eq(u32 wordBatchSize, u8 *eqArea)
{
    if (wordBatchSize == 0) return;

    u8  pos    = eqArea[0];
    s32 state0, state1;
    memcpy(&state0, eqArea + 4, 4);
    memcpy(&state1, eqArea + 8, 4);

    s32 *scratch = gMidiSampleScratch;

    if (gPcMixMult > 1) {
        /* pc-enhanced: same corner at the higher mixer rate, pole a^(1/mult)
         * (see pc_midi_update_scratch_hirate). The state stays in eqArea. */
        u32 hp = pos >= 0x80;
        double a = (double)(hp ? (s32)(pos - 0x80) * 2 : (s32)pos * 2) / 256.0;
        s32 ap = (s32)(pow(a, 1.0 / gPcMixMult) * 65536.0 + 0.5);
        s32 bp = 65536 - ap;
        for (u32 n = 0; n < wordBatchSize * 4; n++) {
            s32 r = scratch[0], l = scratch[1];
            state0 = (s32)(((int64_t)r * bp + (int64_t)state0 * ap) >> 16);
            state1 = (s32)(((int64_t)l * bp + (int64_t)state1 * ap) >> 16);
            scratch[0] = hp ? r - state0 : state0;
            scratch[1] = hp ? l - state1 : state1;
            scratch += 2;
        }
        memcpy(eqArea + 4, &state0, 4);
        memcpy(eqArea + 8, &state1, 4);
        return;
    }

    if (pos < 0x80) {
        s32 alpha = (s32)pos * 2;
        s32 beta  = 0x100 - alpha;
        for (u32 w = 0; w < wordBatchSize; w++) {
            for (u32 s = 0; s < 4; s++) {
                s32 r = scratch[0], l = scratch[1];
                state0 = (r * beta + state0 * alpha) >> 8;
                state1 = (l * beta + state1 * alpha) >> 8;
                scratch[0] = state0;
                scratch[1] = state1;
                scratch += 2;
            }
        }
    } else {
        s32 alpha = (s32)(pos - 0x80) * 2;
        s32 beta  = 0x100 - alpha;
        for (u32 w = 0; w < wordBatchSize; w++) {
            for (u32 s = 0; s < 4; s++) {
                s32 r = scratch[0], l = scratch[1];
                s32 fr = (r * beta + state0 * alpha) >> 8;
                s32 fl = (l * beta + state1 * alpha) >> 8;
                state0 = fr; state1 = fl;
                scratch[0] = r - fr;
                scratch[1] = l - fl;
                scratch += 2;
            }
        }
    }

    memcpy(eqArea + 4, &state0, 4);
    memcpy(eqArea + 8, &state1, 4);
}

/* ── ThumbFunc global variable definitions ───────────────────────────────── */
/*
 * On GBA these globals are installed at runtime (in IWRAM) by
 * midi_asm_init_mode / midi_asm_init_table.  On PC we initialise them
 * statically to point at the C implementations above.
 *
 * midi.h declares each as:  extern ThumbFunc midi_asm_*;
 * Here we provide the definitions.
 */
ThumbFunc midi_asm_init_mode      = (ThumbFunc)pc_midi_init_mode;
ThumbFunc midi_asm_read_pcm_accurate = (ThumbFunc)pc_midi_read_pcm_accurate;
ThumbFunc midi_asm_update_scratch = (ThumbFunc)pc_midi_update_scratch;
ThumbFunc midi_asm_update_buffer  = (ThumbFunc)pc_midi_update_buffer;
ThumbFunc midi_asm_read_pcm_fixed = (ThumbFunc)pc_midi_read_pcm_fixed;
ThumbFunc midi_asm_read_pcm_fast  = (ThumbFunc)pc_midi_read_pcm_fast;
ThumbFunc midi_asm_apply_eq       = (ThumbFunc)pc_midi_apply_eq;

/* No-op shims for THUMB↔ARM dispatch stubs never used on PC */
void midi_asm_init_table(void)              {}
void midi_asm_call_arm_func_id(void)        {}
void midi_asm_read_pcm_fast_call_arm(void)  {}
void midi_asm_update_buffer_call_arm(void)  {}
void midi_asm_read_pcm_accurate_call_arm(void) {}

#endif /* PLATFORM_PC */
