#include "code_08003b28.h"
#include "syscall.h"
#include "src/code_080068f8.h"
#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/code_08007468.h"
#endif

#ifdef PLATFORM_PC
extern void ppu_set_hblank_palette(const u16 *source, u32 firstEntry, u32 count);
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\"");//Temporary
#endif


/* GRADIENT BACKDROP */


// VALUES
#define GRADIENT_BUFFER_SIZE (16u * 10u)

// VARIABLES
static struct {
    u32 enabled:1;
    u32 wordSize:1;
    u32 queuedWordSize:1;
    u32 hasQueuedChanges:1;
    u16 wordCount;
    u16 queuedWordCount;
    u16 *source;
    u16 *dest;
    u16 *queuedSource;
    u16 *queuedDest;
} sBackdropRenderer;
static u16 sGradientBuffer[GRADIENT_BUFFER_SIZE];
static u16 sColor1Index;
static u16 sColor2Index;
static u8 sGradientIsAllocated;
static u16 sColor1Value;
static u16 sColor2Value;
static s16 sBaseOffsetFromPaletteRAM;
static u8 sGradientStartIndex;
static u8 sGradientEndIndex;


// Reset Gradient Backdrop
void func_08003f28(void) {
    volatile u32 dummy;

    sBackdropRenderer.enabled = FALSE;
    sBackdropRenderer.hasQueuedChanges = FALSE;
    sGradientIsAllocated = FALSE;
}


// Flush Gradient to Palette RAM (DMA0)
void func_08003f50(void) {
    u32 controls;

#ifdef PLATFORM_PC
    // There is no HBlank DMA on PC. Hand the per-line table to the renderer
    // instead: platform/ppu.c reads line y's colours from
    // source[y * wordCount ...] for the palette entries at dest, which is
    // what the immediate copy plus the repeating HBlank transfer produce.
    // Only 16-bit transfers into palette RAM are used by this game.
    if (sBackdropRenderer.enabled && !sBackdropRenderer.wordSize
        && ((uintptr_t)sBackdropRenderer.dest >= PaletteRAMBase)
        && ((uintptr_t)sBackdropRenderer.dest < PaletteRAMBase + 0x400)) {
        ppu_set_hblank_palette(sBackdropRenderer.source,
                               ((uintptr_t)sBackdropRenderer.dest - PaletteRAMBase) / 2,
                               sBackdropRenderer.wordCount);
    } else {
        ppu_set_hblank_palette(NULL, 0, 0);
    }
    return;
#endif

    REG_DMA0CNT = 0;

    if (!sBackdropRenderer.enabled) {
        return;
    }

    REG_DMA0SAD = (u32)sBackdropRenderer.source;
    REG_DMA0DAD = (u32)sBackdropRenderer.dest;
    controls = sBackdropRenderer.wordCount | sBackdropRenderer.wordSize << 10 | DMACNT_ENABLE << 16;
    REG_DMA0CNT = controls;

    REG_DMA0SAD = (u32)sBackdropRenderer.source + (sBackdropRenderer.wordCount << (sBackdropRenderer.wordSize + 1));
    REG_DMA0DAD = (u32)sBackdropRenderer.dest;
    REG_DMA0CNT = controls | (DMACNT_START_MODE_HBLANK | DMACNT_REPEAT | DMACNT_DEST_INC_TYPE_INCREMENT_WITHOUT) << 16;
}


// Flush Gradient to Palette RAM (DMA3)
void func_08003fb4(void) {
    u32 controls;

    if (!sBackdropRenderer.enabled) {
        return;
    }

    REG_DMA3SAD = (u32)sBackdropRenderer.source;
    REG_DMA3DAD = (u32)sBackdropRenderer.dest;
    controls = sBackdropRenderer.wordCount | sBackdropRenderer.wordSize << 10 | DMACNT_ENABLE << 16;
    REG_DMA3CNT = controls;
}


// Update Gradient Backdrop
void func_08003ff0(void) {
    if (!sBackdropRenderer.hasQueuedChanges) {
        return;
    }

    sBackdropRenderer.source = sBackdropRenderer.queuedSource;
    sBackdropRenderer.dest = sBackdropRenderer.queuedDest;
    sBackdropRenderer.wordCount = sBackdropRenderer.queuedWordCount;
    sBackdropRenderer.wordSize = sBackdropRenderer.queuedWordSize;
    sBackdropRenderer.hasQueuedChanges = FALSE;
    sBackdropRenderer.enabled = TRUE;
}


// Set Buffers for Gradient Backdrop
void func_0800402c(u16 *source, u16 *dest, u32 wordCount, u32 wordSize) {
    sBackdropRenderer.queuedSource = source;
    sBackdropRenderer.queuedDest = dest;
    sBackdropRenderer.queuedWordCount = wordCount;
    sBackdropRenderer.queuedWordSize = wordSize;
    sBackdropRenderer.hasQueuedChanges = TRUE;
}


// Disable Gradient Backdrop
void func_08004058(void) {
    sBackdropRenderer.enabled = FALSE;
    sBackdropRenderer.hasQueuedChanges = FALSE;
}


// Generate Gradient (https://decomp.me/scratch/2u6Wo)
#ifndef PLATFORM_PC
#include "asm/code_08003980/asm_08004070.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08004070] Generate the gradient
//
// Interpolates scanlines [start, end) from colour 1 to colour 2 of the BG
// palette, and fills the lines above with colour 1 and below with colour 2.
// Unless forced, nothing happens while both colours are unchanged. While a
// palette fade is running (any of bits 1 and 4-15 of the word at
// D_03004b10 + 0x854) the colours come from the fade's source palettes.
void func_08004070(u32 force) {
    const u16 *palette;
    u16 color1, color2;
    s32 r1, g1, b1, dr, dg, db;
    s32 lines, step, t, i;
    u32 progress;
    u16 *dst;

    if (D_03004b10.unk854_1 || D_03004b10.unk854_4) {
        palette = (const u16 *)D_030046c0;
    } else {
        palette = (const u16 *)D_03004b10.bgPalette;
    }

    color1 = palette[sColor1Index];
    color2 = palette[sColor2Index];

    if (!force && (color1 == sColor1Value) && (color2 == sColor2Value)) return;

    sColor1Value = color1;
    sColor2Value = color2;

    r1 = color1 & 0x1f;
    g1 = (color1 >> 5) & 0x1f;
    b1 = (color1 >> 10) & 0x1f;
    dr = (color2 & 0x1f) - r1;
    dg = ((color2 >> 5) & 0x1f) - g1;
    db = ((color2 >> 10) & 0x1f) - b1;

    lines = sGradientEndIndex - sGradientStartIndex;
    dst = &sGradientBuffer[sGradientStartIndex];
    step = Div(0x1000000, lines);
    progress = 0;

    for (i = lines; i > 0; i--) {
        t = progress >> 16;
        *dst++ = (r1 + ((dr * t) >> 8))
               | ((g1 + ((dg * t) >> 8)) << 5)
               | ((b1 + ((db * t) >> 8)) << 10);
        progress += step;
    }

    for (i = 0; i < sGradientStartIndex; i++) {
        sGradientBuffer[i] = color1;
    }
    for (i = sGradientEndIndex; i <= 0x9f; i++) {
        sGradientBuffer[i] = color2;
    }
}
#endif


// Set Backdrop
void func_080041d0(u16 color1Index, u16 color2Index, u16 baseOffset) {
    sColor1Value = -1;
    sColor2Value = -1;
    sColor1Index = color1Index;
    sColor2Index = color2Index;
    sGradientStartIndex = 0;
    sGradientEndIndex = GRADIENT_BUFFER_SIZE;
    func_08004070(TRUE);
    sGradientIsAllocated = TRUE;
    func_0800402c(sGradientBuffer, ((u16 *)PaletteRAMBase) + baseOffset, 1, 0);
    sBaseOffsetFromPaletteRAM = baseOffset;
}


// Disable Gradient Backdrop
void func_08004248(void) {
    sGradientIsAllocated = FALSE;
    func_08004058();
}


// Set Gradient Bounds
void func_0800425c(u32 startIndex, u32 endIndex) {
    sGradientStartIndex = startIndex;
    sGradientEndIndex = endIndex;
}


// Get Target Base Offset
s32 func_08004270(void) {
    if (!sBackdropRenderer.enabled || !sGradientIsAllocated) {
        return -1;
    }

    return sBaseOffsetFromPaletteRAM;
}


// Update Generated Gradient
void func_080042a4(void) {
    if (!sGradientIsAllocated) {
        return;
    }

    func_08004070(FALSE);
}


// ?
void func_080042bc(s32 arg0, s32 arg1, s32 arg2, u16 *buffer, u32 shift) {
    s32 i;
    s32 r5;
    s32 r7;
    s32 r6;
    s32 value;

    for (i = 0; i < arg1 - arg2; i++) {
        buffer[i << shift] = -1;
    }

    for (i = 160u - 1; i > arg1 + arg2; i--) {
        buffer[i << shift] = -1;
    }

    r5 = arg2;
    r7 = 0;
    r6 = -(arg2 * 2) + 3;

    while (r5 >= r7) {
        value = clamp_int32(arg0 - r5, 0, 239) << 8 | clamp_int32(arg0 + r5, 0, 239);
        if (arg1 + r7 < 160u) {
            buffer[(arg1 + r7) << shift] = value;
        }
        if (arg1 - r7 < 160u) {
            buffer[(arg1 - r7) << shift] = value;
        }

        value = clamp_int32(arg0 - r7, 0, 239) << 8 | clamp_int32(arg0 + r7, 0, 239);
        if (arg1 + r5 < 160u) {
            buffer[(arg1 + r5) << shift] = value;
        }
        if (arg1 - r5 < 160u) {
            buffer[(arg1 - r5) << shift] = value;
        }

        if (r6 >= 0) {
            r5--;
            r6 -= (r5 * 4);
        }
        r7++;
        r6 += 2 + (r7 * 4);
    }
}
