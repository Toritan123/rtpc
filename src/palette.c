#include "palette.h"
#include "task_pool.h"
#include "memory_heap.h"


#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/code_08001360.h"
#endif

/* PALETTE INTERPOLATOR */


enum PaletteInterpolatorSourceTypesEnum {
    /* 00 */ SOURCE_TYPE_PAL_PAL,
    /* 01 */ SOURCE_TYPE_PAL_PAL_2,
    /* 02 */ SOURCE_TYPE_COL_PAL,
    /* 03 */ SOURCE_TYPE_PAL_COL,
};


#ifndef PLATFORM_PC
#define FAST_BLEND_PAL_TO_PAL_SIZE ((u32)fast_blend_pal_to_pal_end - (u32)fast_blend_pal_to_pal)
#define FAST_BLEND_COL_TO_PAL_SIZE ((u32)fast_blend_col_to_pal_end - (u32)fast_blend_col_to_pal)

extern void fast_blend_pal_to_pal(void *args);
extern void *fast_blend_pal_to_pal_end;
extern void fast_blend_col_to_pal(void *args);
extern void *fast_blend_col_to_pal_end;

static s32 fast_blend_pal_code[32]; // Palette Interpolation Function
#endif // !PLATFORM_PC

#ifdef PLATFORM_PC
// PC implementations of the GBA ARM Thumb palette blend routines.
// BGR555 format: bits [4:0]=R, [9:5]=G, [14:10]=B
// prog is Q24.8: 0=all color/srcA, 256=all palette/srcB
static void pc_fast_blend_col_to_pal(u16 color, const u16 *src, u16 *dst, u32 count, s32 prog) {
    s32 rc = color & 0x1F;
    s32 gc = (color >> 5) & 0x1F;
    s32 bc = (color >> 10) & 0x1F;
    u32 i;
    for (i = 0; i < count; i++) {
        s32 rp = src[i] & 0x1F;
        s32 gp = (src[i] >> 5) & 0x1F;
        s32 bp = (src[i] >> 10) & 0x1F;
        s32 r  = rc + FIXED_POINT_MUL(rp - rc, prog);
        s32 g  = gc + FIXED_POINT_MUL(gp - gc, prog);
        s32 b  = bc + FIXED_POINT_MUL(bp - bc, prog);
        dst[i] = (u16)(r | (g << 5) | (b << 10));
    }
}

static void pc_fast_blend_pal_to_pal(const u16 *srcA, const u16 *srcB, u16 *dst, u32 count, s32 prog) {
    u32 i;
    for (i = 0; i < count; i++) {
        s32 ra = srcA[i] & 0x1F, ga = (srcA[i] >> 5) & 0x1F, ba = (srcA[i] >> 10) & 0x1F;
        s32 rb = srcB[i] & 0x1F, gb = (srcB[i] >> 5) & 0x1F, bb = (srcB[i] >> 10) & 0x1F;
        s32 r  = ra + FIXED_POINT_MUL(rb - ra, prog);
        s32 g  = ga + FIXED_POINT_MUL(gb - ga, prog);
        s32 b  = ba + FIXED_POINT_MUL(bb - ba, prog);
        dst[i] = (u16)(r | (g << 5) | (b << 10));
    }
}
#endif // PLATFORM_PC


// Stub
void func_08001a24_stub(void) {
}


// Immediately Blend Palette (Array->Color)
void fast_blend_pal_to_col(const u16 *sourceA, u32 valueB, u16 *outputDest, u32 totalColors, u24_8 progress) {
#ifdef PLATFORM_PC
    pc_fast_blend_col_to_pal((u16)valueB, sourceA, outputDest, totalColors, (s32)(INT_TO_FIXED(1.0) - progress));
#else
    {
        void (*interpolatePalette)() = (void *)(fast_blend_pal_code);
        u32 args[5];
        args[0] = (u32)(valueB);
        args[1] = (u32)(sourceA);
        args[2] = (u32)(outputDest);
        args[3] = (u32)(totalColors);
        args[4] = (u32)(INT_TO_FIXED(1.0) - progress);
        dma3_set(fast_blend_col_to_pal, interpolatePalette, sizeof(fast_blend_pal_code), 0x20, 0x100);
        interpolatePalette(args);
    }
#endif
}


// Blend Palette with Interpolator
void pal_interp_blend(struct PaletteInterpolator *task, u32 startIndex) {
    s32 runningTime = task->runningTime;
    s32 duration = task->duration;
    s24_8 progress = INT_TO_FIXED(runningTime) / duration;

    switch (task->sourceType) {
        case SOURCE_TYPE_PAL_PAL:
        case SOURCE_TYPE_PAL_PAL_2:
#ifdef PLATFORM_PC
            pc_fast_blend_pal_to_pal(
                task->sourceA + startIndex,
                task->sourceB + startIndex,
                task->outputDest + startIndex,
                task->totalPalettes * 16,
                (s32)progress);
#else
            {
                void (*interpolatePalette)() = (void *)(fast_blend_pal_code);
                u32 args[5];
                args[0] = (u32)(task->sourceA + startIndex);
                args[1] = (u32)(task->sourceB + startIndex);
                args[2] = (u32)(task->outputDest + startIndex);
                args[3] = (u32)(task->totalPalettes * 16);
                args[4] = (u32)(progress);
                dma3_set(fast_blend_pal_to_pal, interpolatePalette, sizeof(fast_blend_pal_code), 0x20, 0x100);
                interpolatePalette(args);
            }
#endif
            break;

        case SOURCE_TYPE_COL_PAL:
#ifdef PLATFORM_PC
            pc_fast_blend_col_to_pal(
                (u16)(uintptr_t)task->sourceA,
                task->sourceB + startIndex,
                task->outputDest + startIndex,
                task->totalPalettes * 16,
                (s32)progress);
#else
            {
                void (*interpolatePalette)() = (void *)(fast_blend_pal_code);
                u32 args[5];
                args[0] = (u32)(task->sourceA);
                args[1] = (u32)(task->sourceB + startIndex);
                args[2] = (u32)(task->outputDest + startIndex);
                args[3] = (u32)(task->totalPalettes * 16);
                args[4] = (u32)(progress);
                dma3_set(fast_blend_col_to_pal, interpolatePalette, sizeof(fast_blend_pal_code), 0x20, 0x100);
                interpolatePalette(args);
            }
#endif
            break;

        case SOURCE_TYPE_PAL_COL:
#ifdef PLATFORM_PC
            pc_fast_blend_col_to_pal(
                (u16)(uintptr_t)task->sourceB,
                task->sourceA + startIndex,
                task->outputDest + startIndex,
                task->totalPalettes * 16,
                (s32)(INT_TO_FIXED(1.0) - progress));
#else
            {
                void (*interpolatePalette)() = (void *)(fast_blend_pal_code);
                u32 args[5];
                args[0] = (u32)(task->sourceB);
                args[1] = (u32)(task->sourceA + startIndex);
                args[2] = (u32)(task->outputDest + startIndex);
                args[3] = (u32)(task->totalPalettes * 16);
                args[4] = (u32)(INT_TO_FIXED(1.0) - progress);
                dma3_set(fast_blend_col_to_pal, interpolatePalette, sizeof(fast_blend_pal_code), 0x20, 0x100);
                interpolatePalette(args);
            }
#endif
            // break;
    }
}


// Update Palette Interpolation
void pal_interp_update(struct PaletteInterpolator *task) {
    if ((task == NULL) || !task->isActive) {
        return;
    }

    task->runningTime++;
    if (task->runningTime > task->duration) {
        task->isActive = FALSE;
        return;
    }

    pal_interp_blend(task, 0);
}


// Initialise Palette Output for Interpolation
void pal_interp_init_dest(struct PaletteInterpolator *task, u32 startIndex) {
    const u16 *src;
    u16 *dest;

    src = task->sourceA + startIndex;
    dest = task->outputDest + startIndex;

    switch (task->sourceType) {
        case SOURCE_TYPE_PAL_PAL:
        case SOURCE_TYPE_PAL_PAL_2:
        case SOURCE_TYPE_PAL_COL:
            dma3_set(src, dest, task->totalPalettes * 16 * 2, 0x10, 0x100);
            break;
        case SOURCE_TYPE_COL_PAL:
            dma3_fill((u32)src | ((u32)src << 16), dest, task->totalPalettes * 16 * 2, 0x10, 0x100);
            break;
    }
}


// Initialise Palette Interpolator (Array->Array)
void pal_interp_init_ptp(struct PaletteInterpolator *task, u32 duration, u32 totalPalettes, const u16 *sourceA, const u16 *sourceB, u16 *outputBackup, u16 *outputDest) {
    if (task == NULL) {
        return;
    }

    task->duration = duration;
    task->runningTime = 0;
    task->totalPalettes = totalPalettes;
    task->sourceA = sourceA;
    task->sourceB = sourceB;
    task->outputBackup = outputBackup;
    task->outputDest = outputDest;
    task->sourceType = SOURCE_TYPE_PAL_PAL;
    task->isActive = TRUE;
    pal_interp_init_dest(task, 0);
}


// Initialise Palette Interpolator (Color->Array)
void pal_interp_init_ctp(struct PaletteInterpolator *task, u32 duration, u32 totalPalettes, u32 valueA, const u16 *sourceB, u16 *outputBackup, u16 *outputDest) {
    if (task == NULL) {
        return;
    }

    task->duration = duration;
    task->runningTime = 0;
    task->totalPalettes = totalPalettes;
    task->sourceA = (void *)valueA;
    task->sourceB = sourceB;
    task->outputBackup = outputBackup;
    task->outputDest = outputDest;
    task->sourceType = SOURCE_TYPE_COL_PAL;
    task->isActive = TRUE;
    pal_interp_init_dest(task, 0);
}


// Initialise Palette Interpolator (Array->Color)
void pal_interp_init_ptc(struct PaletteInterpolator *task, u32 duration, u32 totalPalettes, const u16 *sourceA, u32 valueB, u16 *outputBackup, u16 *outputDest) {
    if (task == NULL) {
        return;
    }

    task->duration = duration;
    task->runningTime = 0;
    task->totalPalettes = totalPalettes;
    task->sourceA = sourceA;
    task->sourceB = (void *)valueB;
    task->outputBackup = outputBackup;
    task->outputDest = outputDest;
    task->sourceType = SOURCE_TYPE_PAL_COL;
    task->isActive = TRUE;
    pal_interp_init_dest(task, 0);
}


// Copy Contents of OutputDest to OutputBackup
void pal_interp_write_backup(struct PaletteInterpolator *task) {
    if (task == NULL || !task->isActive || task->outputBackup == NULL) {
        return;
    }

    dma3_set(task->outputDest, task->outputBackup, 0x200, 0x20, 0x80);
}


// Update Palette Interpolator (using PaletteMask)
void pal_interp_update_masked(struct PaletteInterpolator *task) {
    s32 i;

    if (task == NULL || !task->isActive) {
        return;
    }

    task->runningTime++;
    if (task->runningTime > task->duration) {
        task->isActive = FALSE;
        return;
    }

    for (i = 0; i < 16; i++) {
        if (((task->paletteMask >> i) & 1) != 0) {
            pal_interp_blend(task, i * 16);
        }
    }
}


// Blend Palette (Array->Array)
void blend_pal_to_pal(u8 alpha, u8 totalPalettes, const u16 *sourceA, const u16 *sourceB, u16 *outputDest) {
    struct PaletteInterpolator task;

    task.duration = 32;
    task.runningTime = alpha;
    task.totalPalettes = totalPalettes;
    task.sourceA = sourceA;
    task.sourceB = sourceB;
    task.outputDest = outputDest;
    task.sourceType = SOURCE_TYPE_PAL_PAL;
    task.isActive = TRUE;

    pal_interp_blend(&task, 0);
}


// Blend Palette (Color->Array)
void blend_col_to_pal(u8 alpha, u8 totalPalettes, u32 valueA, const u16 *sourceB, u16 *outputDest) {
    struct PaletteInterpolator task;

    task.duration = 32;
    task.runningTime = alpha;
    task.totalPalettes = totalPalettes;
    task.sourceA = (void *)valueA;
    task.sourceB = sourceB;
    task.outputDest = outputDest;
    task.sourceType = SOURCE_TYPE_COL_PAL;
    task.isActive = TRUE;

    pal_interp_blend(&task, 0);
}


// Blend Palette (Array->Color)
void blend_pal_to_col(u8 alpha, u8 totalPalettes, const u16 *sourceA, u32 valueB, u16 *outputDest) {
    struct PaletteInterpolator task;

    task.duration = 32;
    task.runningTime = alpha;
    task.totalPalettes = totalPalettes;
    task.sourceA = sourceA;
    task.sourceB = (void *)valueB;
    task.outputDest = outputDest;
    task.sourceType = SOURCE_TYPE_PAL_COL;
    task.isActive = TRUE;

    pal_interp_blend(&task, 0);
}


// Start Palette Interpolator (Array->Array)
struct PaletteInterpolator *pal_interp_task_start_ptp(struct PaletteInterpolatorInputs *inputs) {
    struct PaletteInterpolator *task;

    task = mem_heap_alloc(sizeof(struct PaletteInterpolator));
    pal_interp_init_ptp(task, inputs->duration, inputs->totalPalettes, inputs->sourceA, inputs->sourceB, NULL, inputs->outputDest);

    return task;
}


// Start Palette Interpolator (Color->Array)
struct PaletteInterpolator *pal_interp_task_start_ctp(struct PaletteInterpolatorInputs *inputs) {
    struct PaletteInterpolator *task;

    task = mem_heap_alloc(sizeof(struct PaletteInterpolator));
    pal_interp_init_ctp(task, inputs->duration, inputs->totalPalettes, (u32)inputs->sourceA, inputs->sourceB, NULL, inputs->outputDest);

    return task;
}


// Start Palette Interpolator (Array->Color)
struct PaletteInterpolator *pal_interp_task_start_ptc(struct PaletteInterpolatorInputs *inputs) {
    struct PaletteInterpolator *task;

    task = mem_heap_alloc(sizeof(struct PaletteInterpolator));
    pal_interp_init_ptc(task, inputs->duration, inputs->totalPalettes, inputs->sourceA, (u32)inputs->sourceB, NULL, inputs->outputDest);

    return task;
}


// Update Palette Interpolator
u32 pal_interp_task_update(struct PaletteInterpolator *task) {
    pal_interp_update(task);
    return !task->isActive;
}


// Interpolate Palettes (Array->Array)
static struct TaskMethods pal_interp_task_ptp = {
    (TaskStartFunc)pal_interp_task_start_ptp,
    (TaskUpdateFunc)pal_interp_task_update,
    NULL,
    NULL
};

s32 palette_fade_to(u16 memID, u8 duration, u8 totalPalettes, const u16 *sourceA, const u16 *sourceB, u16 *outputDest) {
    struct PaletteInterpolatorInputs info;

    info.duration = duration;
    info.totalPalettes = totalPalettes;
    info.sourceA = sourceA;
    info.sourceB = sourceB;
    info.outputDest = outputDest;

    return start_new_task(memID, &pal_interp_task_ptp, &info, NULL, 0);
}


// Interpolate Palettes (Color->Array)
static struct TaskMethods pal_interp_task_ctp = {
    (TaskStartFunc)pal_interp_task_start_ctp,
    (TaskUpdateFunc)pal_interp_task_update,
    NULL,
    NULL
};

s32 palette_fade_in(u16 memID, u8 duration, u8 totalPalettes, u32 valueA, const u16 *sourceB, u16 *outputDest) {
    struct PaletteInterpolatorInputs info;

    info.duration = duration;
    info.totalPalettes = totalPalettes;
    info.sourceA = (void *)valueA;
    info.sourceB = sourceB;
    info.outputDest = outputDest;

    return start_new_task(memID, &pal_interp_task_ctp, &info, NULL, 0);
}


// Interpolate Palettes (Array->Color)
static struct TaskMethods pal_interp_task_ptc = {
    (TaskStartFunc)pal_interp_task_start_ptc,
    (TaskUpdateFunc)pal_interp_task_update,
    NULL,
    NULL
};

s32 palette_fade_out(u16 memID, u8 duration, u8 totalPalettes, const u16 *sourceA, u32 valueB, u16 *outputDest) {
    struct PaletteInterpolatorInputs info;

    info.duration = duration;
    info.totalPalettes = totalPalettes;
    info.sourceA = sourceA;
    info.sourceB = (void *)valueB;
    info.outputDest = outputDest;

    return start_new_task(memID, &pal_interp_task_ptc, &info, NULL, 0);
}


// Get Blend of Two Colors
u16 get_blended_color(u16 col1, u16 col2, u16 blendAlpha) {
    s32 r1, g1, b1;
    s32 r2, g2, b2;

    r1 = col1 & 0x1F;
    g1 = (col1 >> 5) & 0x1F;
    b1 = (col1 >> 10) & 0x1F;

    r2 = col2 & 0x1F;
    g2 = (col2 >> 5) & 0x1F;
    b2 = (col2 >> 10) & 0x1F;

    r1 += FIXED_POINT_MUL(r2 - r1, blendAlpha);
    g1 += FIXED_POINT_MUL(g2 - g1, blendAlpha);
    b1 += FIXED_POINT_MUL(b2 - b1, blendAlpha);

    return (r1) | (g1 << 5) | (b1 << 10);
}
