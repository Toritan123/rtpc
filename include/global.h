#pragma once

#include <stdint.h>
#include <stdio.h>

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t    s8;
typedef int16_t  s16;
typedef int32_t  s32;

typedef enum Boolean {
    FALSE,
    TRUE
} boolean;

#define ABS(x)  ((x) < 0 ? -(x) : (x))

#define sins(x) gSineTable[((u32)(x))&0x7FF]
#define coss(x) gSineTable[(((u32)(x))+0x200)&0x7FF]

#define sins2(x) D_08935fcc[((u32)(x))&0xFF]
#define coss2(x) D_089361cc[((u32)(x))&0xFF]

#define lerp(start, end, t, total) ((start) + fast_divsi3(((end) - (start)) * (t), (total)))

// Q24.8 fixed-point types.
typedef u16 u8_8;
typedef u32 u24_8;
typedef s16 s8_8;
typedef s32 s24_8;
// Convert integer to fixed-point value.
#define INT_TO_FIXED(x) ((s32)((x) * 256))
// Convert fixed-point value to integer.
#define FIXED_TO_INT(x) ((s32)((x) >> 8))
// Multiply two fixed-point values.
#define FIXED_POINT_MUL(a, b) (((a) * (b)) >> 8)
// Divide numbers, where 'd' is always fixed-point.
#define FIXED_POINT_DIV(n, d) (((n) << 8) / (d))

#define ARRAY_COUNT(a) (s32)(sizeof(a))/sizeof((a)[0])

// Struct sizes baked into the game's own data tables — Scene.requiredMemory,
// GameEngine.gameDataSize, CueDefinition.cueInfoSize — were computed for the
// GBA, where a pointer is four bytes.  Every one of those structs is larger on
// a 64-bit host, so allocating the recorded size overflows the block by however
// many pointers it holds.  struct DataRoomSceneData is 0x20 on GBA and 0x28
// here, and those eight bytes were landing on the next heap block's function
// pointer.
//
// Each 4-byte slot can at most become 8, and alignment padding grows with it,
// so twice the recorded size is an upper bound; the constant absorbs trailing
// padding.  Regenerating the tables from the real sizes would be tidier, but
// they are decompiled data and this is the layer that knows about the host.
#ifdef PLATFORM_PC
#define GBA_STRUCT_BYTES(n) ((u32)(n) * 2u + 64u)
#else
#define GBA_STRUCT_BYTES(n) (n)
#endif
#ifdef PLATFORM_PC
// The GBA build gathers these into a section its linker script places by hand.
// There is no such section on a host target — Mach-O rejects the name outright
// — and nothing on PC depends on where they land.
#define COMMON_DATA
#else
#define COMMON_DATA __attribute__((section(".common_data"), aligned(4)))
#endif


#include "gba/gba.h"
#include "types.h"
#include "sequence_data.h"
