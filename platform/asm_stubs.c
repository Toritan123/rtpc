/*
 * PC-side replacements for GBA BIOS calls, hand-coded ARM routines, and
 * not-yet-decompiled assembly stubs.
 *
 * All symbols here have the same signature as the originals so the existing
 * C code links and runs.  Where the original implemented real hardware logic
 * (DMA, register writes, etc.) the PC version uses standard-library calls or
 * simply no-ops.
 */
#ifdef PLATFORM_PC

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
// global.h provides u8/u16/u32/s8/s16/s32 typedefs needed by types.h
#include "global.h"
#include "include/types.h"
#include "src/code_0800b778.h"
#include "src/task_pool.h"
#include "src/code_08007468.h"
#include "src/graphics_table.h"
#include "src/midi/midi.h"
#include "save_pc.h"

// ─── GBA BIOS replacements ───────────────────────────────────────────────────

// GBA BIOS Div: returns quotient; original also stores remainder in R1
// For PC callers that only use the return value this is sufficient.
s32 Div(s32 num, s32 den)
{
    if (den == 0) return 0;
    return num / den;
}

// GBA BIOS CpuFastSet: bit 24 selects fill vs. copy; lower 21 bits = word count.
void CpuFastSet(const void *src, void *dest, u32 mode)
{
    u32 words = mode & 0x1FFFFF;
    if (mode & (1u << 24)) {
        // Fill mode: replicate the first source word
        u32 fill;
        memcpy(&fill, src, 4);
        u32 *d = (u32 *)dest;
        for (u32 i = 0; i < words; i++) d[i] = fill;
    } else {
        memcpy(dest, src, words * 4);
    }
}

// ─── Fast math (GBA copies ARM Thumb code into IWRAM at runtime) ─────────────



// math_sqrt: game calls this via a function pointer set by init_math_sqrt().
s32 pc_math_sqrt(s32 value)
{
    if (value <= 0) return 0;
    return (s32)sqrtf((float)value);
}

// math_sqrt: the game installs a pointer to this function at runtime.
// moved to src/global.c upstream: s32 (*math_sqrt)(s32 value) = pc_math_sqrt;

// ROM function blob end markers (the game copies these from ROM to IWRAM)
const u8 math_sqrt_rom_end[1]             = {0};
const u8 mem_heap_alloc_block_rom_end[1]  = {0};
const u8 write_int_sram_fast_rom_end[1]   = {0};

// ─── Interrupt handler ROM stubs ─────────────────────────────────────────────
// On GBA these ROM blobs are copied to IWRAM and executed.  On PC the DmaCopy
// becomes memcpy (no-op in practice) and the handler is never called via
// hardware, so we just need valid, non-NULL symbols to satisfy the linker.
const u8 interrupt_handler_rom[0x200]     = {0};
const u8 interrupt_handler_jtbl_rom[0x38] = {0};

// Destination buffers in "IWRAM" (just regular globals on PC).
u8 interrupt_handler[0x200];
u8 interrupt_handler_jtbl[0x38];

// ─── MIDI library assembly stubs ─────────────────────────────────────────────
// The MIDI library uses a run-time function-pointer table populated by
// midi_asm_init_mode / midi_asm_init_table (lib_midi.s).
// On PC we replace the whole table with C no-ops so the audio system at
// least does not crash.  Real PCM mixing is handled by audio_pc.c which
// drains gMidiPCMBufR/L after each midi_sound_main() call.

// ─── Beatscript command processor ────────────────────────────
// func_0800cb28 lives in platform/beatscript_vm.c — a full translation of
// asm/code_0800b778/asm_0800cb28.s.  Do not add a stub for it here.

// ─── MIDI DSP functions ───────────────────────────────────────────────────────
// Implemented in platform/midi_dsp.c (full C replacements for ARM assembly).
// Do NOT define them here — midi_dsp.c provides the real implementations.

// ─── MIDI PCM buffer area (lives in IWRAM on GBA) ────────────────────────────
// Declared as extern in audio/sound_players.inc.c – provide the definition here.
// 1568 samples = 7 frames at the GBA's 13379 Hz.  Sized for the maximum
// RTPC_MIX_MULT (4x) so the ring still spans the same number of frames when
// the mixer runs above hardware rate.
u8 sPCMBufferArea[2][PC_DMA_SAMPLE_BUFFER_SIZE];
s32 sPCMScratchArea[0x80 * 2];

// ─── GFX decompression ───────────────────────────────────────────────────────
// decompress_gfx_rom lives in platform/gfx_decompress.c — a translation of the
// game's own ARM routine.  It used to be a hand-written GBA-BIOS LZ77/RLE
// decompressor here, which was both the wrong algorithm and the wrong
// signature for the way decompress_gfx_init calls it.

// ─── SRAM (save) I/O stubs ───────────────────────────────────────────────────

// Function pointer types declared in lib_sram.h
extern void (*read_sram_fast)(const u8 *src, u8 *dest, u32 size);
extern void (*write_int_sram_fast)(const u8 *src, u8 *dest, u32 size);
extern void (*verify_sram_fast)(const u8 *src, u8 *dest, u32 size);

static void pc_read_sram_fast(const u8 *src, u8 *dest, u32 size)
{
    memcpy(dest, src, size);
}
static void pc_write_int_sram_fast(const u8 *src, u8 *dest, u32 size)
{
    memcpy(dest, src, size);
    rtpc_sram_mark_dirty();
}
static void pc_verify_sram_fast(const u8 *src, u8 *dest, u32 size)
{
    (void)src; (void)dest; (void)size;
}

// Called by init_save_buffer() to install the SRAM fast-path function pointers.
void set_sram_fast_func(void)
{
    read_sram_fast      = pc_read_sram_fast;
    write_int_sram_fast = pc_write_int_sram_fast;
    verify_sram_fast    = pc_verify_sram_fast;
}

void write_sram_fast(const u8 *src, u8 *dest, u32 size)
{
    memcpy(dest, src, size);
    rtpc_sram_mark_dirty();
}

void read_sram(const u8 *src, u8 *dest, u32 size)
{
    memcpy(dest, src, size);
}

// ─── Misc. asm-only functions referenced by code_08003b28.c ─────────────────

// code_08009de4.s – text-line animation builder. Real PC implementation
// lives in src/text_printer.c (#ifdef PLATFORM_PC).

// lib_0804e418 – OAM OBJ attribute writer; real implementation is in sprite_lib_pc.c
// lib_0804e938 – compression helper
/* func_0804e418 intentionally omitted here – defined in platform/sprite_lib_pc.c */
void func_0804e938(void) {}

// ─── GFX decompression cache lookup ─────────────────────────────────────────
// func_0800869c used to be stubbed here as "return src unchanged", which was
// harmless only because nothing was ever buffered.  It is now translated from
// assembly in src/code_08007468.c along with the rest of the cache.

// ─── Synchronous GFX table loader (PC replacement) ───────────────────────────
// func_08002e78: synchronous version of func_08002ee0 – loads a GFX table all
// at once (large limit = 0x20000) before returning.  Used by gameplay_start_scene
// and the beatscript engine for "must-have-now" assets.
// The GBA version keeps the GfxTableLoader on the stack; we do the same.
void func_08002e78(const struct GraphicsTable *gfxTable)
{
    struct GfxTableLoader loader;
    func_08002a6c(&loader, gfxTable, 0x20000u);
    while (loader.active) {
        func_08002b10(&loader);
    }
}

// ─── Texture loader task ─────────────────────────────────────────────────
// The stand-in that used to live here returned "done" immediately without
// decompressing anything, on the reasoning that a PC has no VRAM.  It does
// have one (gba_vram, which the PPU draws from), so engines that load their
// graphics through this task got none.  The real loader is now translated
// from assembly in src/code_08007468.c.

#endif // PLATFORM_PC
