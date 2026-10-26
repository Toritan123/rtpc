/*
 * Correct definitions for global variables that auto_stubs.c gets wrong.
 * These variables have complex types (structs, function pointers) that need
 * the real game headers to define properly.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "graphics.h"
#include "src/memory_heap.h"
#include "src/lib_0804ca80.h"
#include "src/lib_sram.h"
#include "src/lib_0804e564.h"
#include "include/types.h"
#include "src/code_0800b778.h"

// Graphics double-buffer (holds BG/OBJ palette, OAM, display control registers)
struct GraphicsBuffer D_03004b10;

// Memory heap tracking struct
struct struct_03004ad0 D_03004ad0;

// Save-data pointer (set by init_ewram / init_save_buffer)
// Declared in memory.h as a pointer to an anonymous struct — start NULL.
// The game sets this during save buffer init.
// moved to src/global.c upstream: void *D_030046a8 = NULL;

// RLE decompressor function pointer (set by func_08003e64 on both GBA and PC)
// moved to src/global.c upstream: s32 (*D_03004af0)(const u16 *src, u16 *dest, const u8 *rleData, u32 sizeData) = NULL;

// Gyro/tilt read function pointer (optional peripheral — NULL on PC)
// moved to src/global.c upstream: s32 (*D_030064d4)(void) = NULL;

// SRAM fast-path function pointers (installed by lib_sram init)
// On PC we fall back to plain memcpy-based helpers in asm_stubs.c.
// These are only called after an explicit install step that we skip on PC.
// moved to src/global.c upstream: void (*read_sram_fast)(const u8 *src, u8 *dest, u32 size)     = NULL;
// moved to src/global.c upstream: void (*write_int_sram_fast)(const u8 *src, u8 *dest, u32 size) = NULL;
// moved to src/global.c upstream: void (*verify_sram_fast)(const u8 *src, u8 *dest, u32 size)   = NULL;

// Sprite handler global (set by sprite_handler_create in func_080073b8)
// moved to src/global.c upstream: struct SpriteHandler *gSpriteHandler = NULL;

// Sprite allocator function pointers (set by sprite_lib_set_mem_alloc)
void *(*sSpriteMemAlloc)(u32)        = NULL;
void  (*sSpriteMemDealloc)(void *)   = NULL;
void *(*sSpriteMemAllocId)(u32, u32) = NULL;

// Scene/engine data pointers — dynamically allocated per scene
// moved to src/global.c upstream: void *gCurrentSceneData  = NULL;
// moved to src/global.c upstream: void *gCurrentEngineData = NULL;

// Beatscript engine state (declared as extern struct in types.h)
struct BeatscriptScene D_030053c0;

// Beatscript thread-local state pointers (set in set_beatscript_subscenes)
// moved to src/global.c upstream: uintptr_t *D_03005588 = NULL;
// moved to src/global.c upstream: s16 *D_0300558c = NULL;

#endif // PLATFORM_PC

// Cartridge GPIO (rumble/gyro pak) stand-in — see src/lib_0804e564.h.
volatile u16 gPcGpioDummy[3];
