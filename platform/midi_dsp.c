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
s16 gPcPcmHiR[PC_HIRES_SAMPLES];
s16 gPcPcmHiL[PC_HIRES_SAMPLES];
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
static void pc_midi_update_scratch(u32 wordBatchSize)
{
    if (wordBatchSize == 0) return;

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
                gPcPcmHiR[pos * 4 + s] = pc_scratch_to_s16(hi[s * 2]);
                gPcPcmHiL[pos * 4 + s] = pc_scratch_to_s16(hi[s * 2 + 1]);
            }
        }
    }
    gPcPcmHiValid = (bufSize * 4 <= PC_HIRES_SAMPLES) ? bufSize * 4 : 0;
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
