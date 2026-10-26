/*
 * PC reimplementation of the GBA affine-parameter manager
 * (asm/code_08001360/asm_080020ec.s and neighbours).
 *
 * The GBA maintains 32 affine "groups".  Each group owns 4 consecutive OAM
 * entries' affine slots (the u16 at byte offset 6 of each 8-byte entry) —
 * the hardware convention for rotation/scaling parameter storage.
 *
 * Two matrices are kept per group:
 *   sParamsOam[g]  (GBA: D_03000138) – flushed into the OAM shadow each frame
 *                                      by func_08002584 (texture-step form).
 *   sParamsDraw[g] (GBA: D_03000238) – handed to the sprite renderer via
 *                                      func_08002520 for multi-OBJ cel layout.
 *
 * Per-group flags (GBA: D_03000340):
 *   0x01  allocated
 *   0x02  flip X   (negate pa,pc after compute)
 *   0x04  flip Y   (negate pb,pd after compute)
 *   0x08  use 11-bit gSineTable angle (else 8-bit D_08935fcc/D_089361cc)
 *   0x10  also maintain the second matrix (set by default on alloc: 0x11)
 */
#ifdef PLATFORM_PC

#include "global.h"

extern s32 fast_divsi3(s32 dividend, s32 divisor);
extern s16 gSineTable[];   // 0x800 entries, Q8.8
extern s16 D_08935fcc[];   // 256-entry sine, Q8.8
extern s16 D_089361cc[];   // 256-entry cosine, Q8.8

#define AFFINE_MAX_GROUPS 32

static u32 *sOamBuffer;                       /* D_03000338 */
static u8   sGroupCount;                      /* D_03000360 */
static s16  sParamsOam[AFFINE_MAX_GROUPS][4]; /* D_03000138 */
static s16  sParamsDraw[AFFINE_MAX_GROUPS][4];/* D_03000238 */
static u8   sFlags[AFFINE_MAX_GROUPS];        /* D_03000340 */
static u32  sOwner[AFFINE_MAX_GROUPS];        /* D_03000368 */


// Init: set group count + OAM buffer, reset all groups to identity.
void func_080020ec(u32 count, u32 *oamBuffer)
{
    u32 i;

    sOamBuffer = oamBuffer;
    if (count > AFFINE_MAX_GROUPS) count = AFFINE_MAX_GROUPS;
    sGroupCount = count;

    for (i = 0; i < count; i++) {
        sParamsOam[i][0] = 0x100; sParamsOam[i][1] = 0;
        sParamsOam[i][2] = 0;     sParamsOam[i][3] = 0x100;
        sParamsDraw[i][0] = 0x100; sParamsDraw[i][1] = 0;
        sParamsDraw[i][2] = 0;     sParamsDraw[i][3] = 0x100;
        sFlags[i] = 0;
    }
}


// Find + claim a free group (flags -> 0x11).  Returns -1 when full.
s32 func_08002150(void)
{
    s32 i;

    for (i = 0; i < sGroupCount; i++) {
        if (sFlags[i] == 0) {
            sFlags[i] = 0x11;
            sOwner[i] = 0;
            return i;
        }
    }
    return -1;
}


// Allocate a group tagged with an owner (mem ID).
s32 func_08002194(u32 owner)
{
    s32 idx = func_08002150();

    if (idx >= 0) {
        sOwner[idx] = owner;
    }
    return idx;
}


// Reset a group to identity and mark it free.
void func_080021b8(s32 idx)
{
    if (idx < 0 || idx >= AFFINE_MAX_GROUPS) return;

    sParamsOam[idx][0] = 0x100; sParamsOam[idx][1] = 0;
    sParamsOam[idx][2] = 0;     sParamsOam[idx][3] = 0x100;
    sParamsDraw[idx][0] = 0x100; sParamsDraw[idx][1] = 0;
    sParamsDraw[idx][2] = 0;     sParamsDraw[idx][3] = 0x100;
    sFlags[idx] = 0;
}


// Free every group owned by 'owner'.
void func_0800222c(u32 owner)
{
    s32 i;

    for (i = 0; i < AFFINE_MAX_GROUPS; i++) {
        if ((sFlags[i] != 0) && (sOwner[i] == owner)) {
            func_080021b8(i);
        }
    }
}


// Set the flip flags (bits 1-2), preserving the rest.
void func_08002260(s32 idx, u32 flipFlags)
{
    if (idx < 0) return;
    sFlags[idx] = (sFlags[idx] & 0xF9) | flipFlags;
}


// Apply the group's flip flags to a computed parameter block.
static void func_08002280(s32 idx, s16 *params)
{
    u8 flags = sFlags[idx];

    if (flags & 0x02) {
        params[0] = -params[0];
        params[2] = -params[2];
    }
    if (flags & 0x04) {
        params[1] = -params[1];
        params[3] = -params[3];
    }
}


// Use the 8-bit angle tables (clear flag 0x08).
void func_080022bc(s32 idx)
{
    if (idx < 0) return;
    sFlags[idx] &= 0xF7;
}


// Use the 11-bit gSineTable angle (set flag 0x08).
void func_080022d8(s32 idx)
{
    if (idx < 0) return;
    sFlags[idx] |= 0x08;
}


// Maintain the second (renderer) matrix too (set flag 0x10).
void func_080022f4(s32 idx)
{
    if (idx < 0) return;
    sFlags[idx] |= 0x10;
}


// Stop maintaining the second matrix (clear flag 0x10).
void func_08002310(s32 idx)
{
    if (idx < 0) return;
    sFlags[idx] &= 0xEF;
}


static void affine_lookup_sincos(u8 flags, s32 angle, s32 *sinOut, s32 *cosOut)
{
    if (flags & 0x08) {
        *sinOut = gSineTable[angle & 0x7FF];
        *cosOut = gSineTable[(angle + 0x200) & 0x7FF];
    } else {
        *sinOut = D_08935fcc[angle & 0xFF];
        *cosOut = D_089361cc[angle & 0xFF];
    }
}


// Compute a group's matrices from scale + rotation.
//   invertOam: when true the OAM matrix gets the INVERTED scale
//   (65536/scale — the hardware texture-step form) and the renderer matrix
//   the raw scale; when false the roles swap.
void func_0800232c(s32 idx, s16 xScale, s16 yScale, s32 rotation, u32 invertOam)
{
    u8  flags;
    s32 invX = 0, invY = 0;
    s32 oamX, oamY, drawX, drawY;
    s32 rotOam;
    s32 sinV, cosV;
    s16 params[4];

    if (idx < 0 || idx >= AFFINE_MAX_GROUPS) return;
    flags = sFlags[idx];

    if (invertOam || (flags & 0x10)) {
        invX = fast_divsi3(0x10000, xScale);
        invY = (xScale == yScale) ? invX : fast_divsi3(0x10000, yScale);
    }

    if (invertOam) {
        oamX = invX;    oamY = invY;
        drawX = xScale; drawY = yScale;
    } else {
        oamX = xScale;  oamY = yScale;
        drawX = invX;   drawY = invY;
    }

    // OAM matrix: rotation sign-flipped once per flip flag.
    rotOam = rotation;
    if (flags & 0x02) rotOam = -rotOam;
    if (flags & 0x04) rotOam = -rotOam;

    affine_lookup_sincos(flags, rotOam, &sinV, &cosV);
    params[0] = (s16)((oamX * cosV) >> 8);
    params[1] = (s16)((oamX * -sinV) >> 8);
    params[2] = (s16)((oamY * sinV) >> 8);
    params[3] = (s16)((oamY * cosV) >> 8);
    func_08002280(idx, params);
    sParamsOam[idx][0] = params[0];
    sParamsOam[idx][1] = params[1];
    sParamsOam[idx][2] = params[2];
    sParamsOam[idx][3] = params[3];

    // Renderer matrix: original (unflipped) rotation.
    if (flags & 0x10) {
        affine_lookup_sincos(flags, rotation, &sinV, &cosV);
        params[0] = (s16)((drawX * cosV) >> 8);
        params[1] = (s16)((-sinV * drawY) >> 8);
        params[2] = (s16)((drawX * sinV) >> 8);
        params[3] = (s16)((drawY * cosV) >> 8);
        func_08002280(idx, params);
        sParamsDraw[idx][0] = params[0];
        sParamsDraw[idx][1] = params[1];
        sParamsDraw[idx][2] = params[2];
        sParamsDraw[idx][3] = params[3];
    }
}


// Uniform scale + rotation, raw scale into the OAM matrix.
void func_080024dc(s32 idx, s16 scale, s16 rotation)
{
    func_0800232c(idx, scale, scale, rotation, FALSE);
}


// Renderer-matrix accessor (assign_sprite_affine_param passes this to
// sprite_set_affine_params).
s16 *func_08002520(s32 idx)
{
    if (idx < 0 || idx >= AFFINE_MAX_GROUPS) return NULL;
    return sParamsDraw[idx];
}


// Flush every group's OAM matrix into the OAM shadow buffer:
// group g's params spread across entries g*4..g*4+3 at byte offset 6.
void func_08002584(void)
{
    u16 *src = (u16 *)sParamsOam;
    u8  *oam = (u8 *)sOamBuffer;
    u32 total = (u32)sGroupCount * 4;
    u32 i;

    if (oam == NULL) return;
    for (i = 0; i < total; i++) {
        *(u16 *)(oam + i * 8 + 6) = src[i];
    }
}

#endif /* PLATFORM_PC */
