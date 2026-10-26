/*
 * Correct definitions for MIDI library global variables.
 * auto_stubs.c generated these as function stubs; here we provide
 * proper writable data definitions with the initial values the GBA
 * midi_init() sequence would install.
 */
#ifdef PLATFORM_PC

#include <stdlib.h>
#include "global.h"
#include "src/midi/midi.h"
#include "gba_mem.h"

// Declared in asm_stubs.c
extern s32 sPCMScratchArea[0x80 * 2];

// ─── MAIN ────────────────────────────────────────────────────────────────────
// moved to src/global.c upstream: u16 gMidiVCOUNTAtStart    = 0;
// moved to src/global.c upstream: u16 gMidiVCOUNTAtEnd      = 0;
// moved to src/global.c upstream: u16 gMidiVCOUNTAtSamplerEnd = 0;
// moved to src/global.c upstream: u32 gMidiDMASampleRate    = 13379;
// moved to src/global.c upstream: u32 gMidiSamplesPerFrame  = 13379 / 60;

// ─── DIRECTSOUND ─────────────────────────────────────────────────────────────
// moved to src/global.c upstream: u16 gMidiDirectSoundEnabled = 0;
// moved to src/global.c upstream: u16 gMidiSoundChannelCount  = DIRECTSOUND_CHANNEL_COUNT;
// moved to src/global.c upstream: u16 gMidiSamplerCount       = DIRECTSOUND_CHANNEL_COUNT;
// moved to src/global.c upstream: u32 gMidiScratchSize        = SAMPLE_SCRATCHPAD_SIZE;
// moved to src/global.c upstream: volatile u32 gMidiPCMBufSize32 = DMA_SAMPLE_BUFFER_SIZE / 4;
// moved to src/global.c upstream: volatile u32 gMidiPCMBufWritePos = 0;
// moved to src/global.c upstream: volatile u32 gMidiPCMBufReadPos  = 0;
// moved to src/global.c upstream: u16 gMidiSamplerGain        = 0x100;
// moved to src/global.c upstream: u32 gMidiSoundMode          = 0; // 0 = Stereo

// PCM buffer pointers (set to the static areas from asm_stubs.c)
// moved to src/global.c upstream: volatile u32 *gMidiPCMBufR  = NULL; // initialised in midi_globals_init()
// moved to src/global.c upstream: volatile u32 *gMidiPCMBufL  = NULL;

// Scratch pad pointer
// moved to src/global.c upstream: s32 *gMidiSampleScratch = NULL; // initialised in midi_globals_init()

// Lookup table: 10-bit index → sample value (used by DirectSound mixer)
// moved to src/global.c upstream: s8  gMidiSampleTable[0x400] = {0};

// ─── SOUND CHANNEL POOLS ─────────────────────────────────────────────────────
// These arrays are the actual IWRAM pools; gMidiXxxPool points into them.
struct SoundChannel sSoundChannelArea[DIRECTSOUND_CHANNEL_COUNT];
struct SampleStream sSamplerArea[DIRECTSOUND_CHANNEL_COUNT];

// moved to src/global.c upstream: struct SoundChannel *gMidiSoundChannelPool = sSoundChannelArea;
// moved to src/global.c upstream: struct SampleStream *gMidiSamplerPool      = sSamplerArea;

// PSG channel pool (4 channels: Tone+Sweep, Tone, Wave, Noise)
// moved to src/global.c upstream: struct SoundChannel gMidiPSGChannelPool[TOTAL_PSG_CHANNELS] = {{0}};

// ─── MIDI NOTE POOL ──────────────────────────────────────────────────────────
// moved to src/global.c upstream: struct MidiNote gMidiNotePool[20] = {{0}};
// moved to src/global.c upstream: u16 gMidiNoteNext = 0;

// ─── SOUNDPLAYER ─────────────────────────────────────────────────────────────
// moved to src/global.c upstream: u32 gMidiPlayerNewDeltaTime = 0;

// ─── REVERB ──────────────────────────────────────────────────────────────────
// moved to src/global.c upstream: s32 gMidiReverbScratch[4]  = {0};
// moved to src/global.c upstream: s8  gMidiReverbControls[4] = {0};
// moved to src/global.c upstream: u32 gMidiReverb1Wet        = 0;
// moved to src/global.c upstream: u32 gMidiReverb2Phase      = 0;
// moved to src/global.c upstream: u32 gMidiReverb3Decay      = 2;
// moved to src/global.c upstream: u32 gMidiReverb4LowCut     = 4;

// ─── FILTER EQ ───────────────────────────────────────────────────────────────
// moved to src/global.c upstream: s32 gMidiEQArea[3]   = {0};
// moved to src/global.c upstream: u8  gMidiEQHighGain  = 0;
// moved to src/global.c upstream: u8  gMidiEQIsGlobal  = 0;
// moved to src/global.c upstream: s8  gMidiEQPrevPos   = 0;

// ─── LFO ─────────────────────────────────────────────────────────────────────
// moved to src/global.c upstream: u8  gMidiLFODepth  = 0;
// moved to src/global.c upstream: struct LFO gMidiLFO = {0};
// moved to src/global.c upstream: u8  gMidiLFOMode   = 0; // LFO_MODE_DISABLED
// moved to src/global.c upstream: struct SoundPlayer *gMidiLFOPlayer = NULL;

// ─── COMM VARS ───────────────────────────────────────────────────────────────
// moved to src/global.c upstream: u8  *gMidiCommVars      = NULL;
// moved to src/global.c upstream: u16  gMidiCommVarCurrent = 0;
// moved to src/global.c upstream: u16  gMidiCommVarTotal   = 0;

// ─── Late-init function called from pc_main before agb_main ─────────────────
// Sets pointer fields that depend on runtime addresses of static buffers.
extern void midi_dsp_init_sample_table(void);
// RTPC_MIX_MULT raises the MIDI mixer's internal rate above the GBA's 13379 Hz.
// Instrument samples are stored at higher rates than that, so everything in
// them above 6.69 kHz currently folds back down as aliasing *inside* the mixer,
// where no amount of output-side filtering can remove it.  Pitch is preserved
// automatically: midi_sampler_set_sample derives the per-stream step from
// gMidiDMASampleRate, so doubling the rate halves the step.
u32 gPcMixMult = 1;

void midi_globals_init(void)
{
    {
        const char *e = getenv("RTPC_MIX_MULT");
        int m = e ? atoi(e) : 1;
        if (m == 2 || m == 3 || m == 4) gPcMixMult = (u32)m;
    }
    gMidiDMASampleRate   = 13379 * gPcMixMult;
    gMidiSamplesPerFrame = gMidiDMASampleRate / 60;
    gMidiPCMBufSize32    = (DMA_SAMPLE_BUFFER_SIZE * gPcMixMult) / 4;

    // Upstream moved these into src/global.c, which zero-initialises them.  On
    // GBA that is fine: its linker script gives the section its initial image.
    // Here they have to be set before the mixer first runs, or it works with a
    // zero channel count, no pools and a zero-length scratch area.
    gMidiSoundChannelCount = DIRECTSOUND_CHANNEL_COUNT;
    gMidiSamplerCount      = DIRECTSOUND_CHANNEL_COUNT;
    gMidiScratchSize       = SAMPLE_SCRATCHPAD_SIZE;
    gMidiSamplerGain       = 0x100;
    gMidiSoundChannelPool  = sSoundChannelArea;
    gMidiSamplerPool       = sSamplerArea;
    gMidiReverb3Decay      = 2;
    gMidiReverb4LowCut     = 4;

    gMidiPCMBufR      = (volatile u32 *)sPCMBufferArea[0];
    gMidiPCMBufL      = (volatile u32 *)sPCMBufferArea[1];
    gMidiSampleScratch = sPCMScratchArea;
    // Initialise the 10-bit→8-bit saturating lookup table used by
    // midi_asm_update_buffer.  On GBA this is done inside
    // midi_directsound_init (excluded from PC builds).
    midi_dsp_init_sample_table();
    // Master sound enable.  On GBA this is set by midi_directsound_init, which
    // is excluded from the PC build, so the PSG channels stayed gated off and
    // psg_pc_render bailed out on every call.
    *(volatile u16 *)(gba_io + 0x84) |= 0x80;   // REG_SOUNDCNT_X
    // PSG output ratio.  Bits 0-1 of SOUNDCNT_H are 0 = 25 %, 1 = 50 %,
    // 2 = 100 %, and they too are left at 0 without the GBA init — so every
    // PSG note played at a quarter of its intended level.
    *(volatile u16 *)(gba_io + 0x82) =
        (u16)((*(volatile u16 *)(gba_io + 0x82) & ~0x3) | 2);
    // midi_directsound_init (excluded on PC) sets this to TRUE after
    // copying the ARM/Thumb DSP blobs to IWRAM.  On PC we skip that
    // step entirely, so enable DirectSound output explicitly here.
    gMidiDirectSoundEnabled = TRUE;
}

#endif // PLATFORM_PC
