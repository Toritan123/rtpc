#pragma once

#include "global.h"
#include "graphics.h"

extern void func_080020ec(u32 count, u32 *oamBuffer); // Init
extern s32  func_08002150(void);        // Find + Claim Free Group
extern s32  func_08002194(u32 owner);   // Allocate New
extern void func_080021b8(s32 idx);     // Delete
extern void func_0800222c(u32 owner);   // Delete by Mem. ID
extern void func_08002260(s16, u32); // Update Horizontal & Vertical Flip (and probably more)
// extern ? func_08002280(?);
extern void func_080022bc(s16); // ?? (Reduced Angle Precision)
extern void func_080022d8(s16); // ?? (Fine Angle Precision)
extern void func_080022f4(s32 idx);     // Maintain Renderer Matrix (set flag 0x10)
extern void func_08002310(s32 idx);     // Stop Maintaining Renderer Matrix
extern void func_0800232c(s32 idx, s16 xScale, s16 yScale, s32 rotation, u32 invertOam); // Set Scale + Rotation
extern void func_080024dc(s32 idx, s16 scale, s16 rotation); // Set Uniform Scale + Rotation
// extern ? func_08002500(?);
extern s16 *func_08002520(s32 idx);     // Get Renderer Matrix
// extern ? func_0800253c(?);
extern void func_08002584(void);        // Flush Params to OAM Buffer
// extern ? func_080025bc(?);
// extern ? func_080025d8(?);
// extern ? func_080025fc(?);
