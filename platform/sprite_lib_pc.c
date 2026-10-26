/*
 * PC reimplementations of the GBA sprite-rendering assembly routines.
 *
 *   func_0804e418 – OAM OBJ attribute writer (replaces asm/lib_0804e418.s)
 *   func_0804e1c8 – Sprite-to-OAM flush      (replaces asm/lib_0804ca80/asm_0804e1c8.s)
 *
 * On GBA both functions are hand-coded ARM assembly for performance.
 * On PC we provide plain C implementations that produce identical output.
 *
 * Data-flow summary:
 *   func_0804e1c8(gSpriteHandler)
 *     → for each visible Sprite in z-order:
 *         fills struct struct_0804cb88 (drawData)
 *         calls func_0804cb88(&drawData)  [already in lib_0804ca80.c]
 *           → sets objTotal from src[0], advances src past count
 *           → calls func_0804e418(&drawData)
 *               → writes (attr0, attr1, attr2) per OBJ to D_03004b10.oam shadow
 *   func_08006e88 (called at frame start)
 *     → copies D_03004b10.oam → gba_oam  (via dma3_set / memcpy on PC)
 *   PPU reads gba_oam each frame.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "src/lib_0804ca80.h"
#include <string.h>
#include <stdio.h>

/* ─── OBJ dimension table ────────────────────────────────────────────────────
 * GBA hardware table: D_08bd0cae[shape][size] = {width, height} in pixels.
 * On GBA the 12 pairs live in ROM at consecutive addresses.
 * On PC the individual globals in gameplay_unused_warioware_graphics.c are not
 * guaranteed to be contiguous, so we use our own local copy instead.
 *
 * Layout: [shape 0=square, 1=wide, 2=tall][size 0..3][0=width, 1=height]
 */
static const u8 pc_obj_dim[3][4][2] = {
    /* Square */  { {8,8},  {16,16}, {32,32}, {64,64} },
    /* Wide   */  { {16,8}, {32,8},  {32,16}, {64,32} },
    /* Tall   */  { {8,16}, {8,32},  {16,32}, {32,64} },
};


/* ═══════════════════════════════════════════════════════════════════════════
 * func_0804e418 – write one animation cel's OBJ entries to the OAM buffer
 *
 * struct struct_0804cb88 *drawData  (filled by func_0804cb88 / func_0804e1c8)
 *   .src      – pointer to first OBJ entry in cel data (3 u16 per entry)
 *   .dest     – pointer to current OAM slot (u32 *, each slot = 8 bytes)
 *   .objTotal – number of OBJ entries in this cel
 *   .srcInc   – bytes to advance src per OBJ (+6=forward, -6=backward)
 *   .destInc  – bytes to advance dest per OBJ (+8=forward, -8=backward)
 *   .xPos     – sprite screen X (signed, stored as u16)
 *   .yPos     – sprite screen Y (signed, stored as u16)
 *   .attr10   – packed sprite OAM flags (see oamAttributes format below)
 *   .attr2    – (basePalette<<12)|(u16)baseTile  base offset for attr2
 *   .affine   – 2×2 rotation matrix [pa, pb, pc, pd] (Q8.8 fixed-point)
 *   .objDim   – copy of OBJ dimension table (from D_08bd0cae / pc_obj_dim)
 *
 * oamAttributes / attr10 encoding (matches GBA hand-coded assembly):
 *   bits  7:0  → GBA OAM attr0 bits 15:8  (rotation/scaling, double-size, …)
 *   bits 15:0  → XOR'd into GBA OAM attr1 (affine group index in bits 13:9)
 *   bits 23:16 → shifted to attr2 bits 15:8 (priority in bits 11:10)
 *   bit    25  → affine-path trigger (set when sprite uses rotation/scaling)
 *   bit    28  → H-flip
 *   bit    29  → V-flip
 *
 * Returns the number of OBJ entries actually written (off-screen ones skipped).
 * ═══════════════════════════════════════════════════════════════════════════ */
u32 func_0804e418(struct struct_0804cb88 *drawData)
{
    const u16 *src  = drawData->src;
    u32       *dest = drawData->dest;
    u8  objTotal    = drawData->objTotal;
    u8  objCount    = 0;
    s32 xPos        = (s16)drawData->xPos;   /* sign-extend */
    s32 yPos        = (s16)drawData->yPos;
    s8  srcInc      = drawData->srcInc;
    s8  destInc     = drawData->destInc;
    u32 attr10      = drawData->attr10;
    u32 attr2base   = drawData->attr2;

    /* ror(attr10, 16) – swaps the two 16-bit halves.
     * Bit mapping after rotation: r6[k] = attr10[(k+16) % 32]
     *   r6 & 0x000000FF = attr10[23:16]   (used for attr2 priority)
     *   r6 & 0x0000FF00 >> 8 = attr10[7:0] (used for attr0 flags)
     *   r6 & 0x0000FFFF = attr10[15:0]    (used for attr1 XOR)
     *   r6 bit  9 = attr10 bit 25           (affine trigger)
     *   r6 bit 28 = attr10 bit 12           (H-flip)
     *   r6 bit 29 = attr10 bit 13           (V-flip)
     */
    u32 r6 = (attr10 >> 16) | (u32)(attr10 << 16);

    for (u8 i = 0; i < objTotal; i++) {
        /* ── Read cel OBJ entry (3 u16: attr0, attr1, attr2) ── */
        u16 cel_attr0 = src[0];
        u16 cel_attr1 = src[1];
        u16 cel_attr2 = src[2];

        /* ── Merge sprite-level flags ── */
        /* attr0: OR in sprite's flag byte (rotation/scaling, double-size, …) */
        u16 out_attr0 = (u16)(cel_attr0 | (u16)((attr10 & 0xFFu) << 8));
        /* attr1: XOR in sprite's low-16 (affine group index) */
        u16 out_attr1 = (u16)(cel_attr1 ^ (u16)(attr10 & 0xFFFFu));
        /* attr2: OR in priority from attr10[23:16], then add base tile/palette */
        u16 out_attr2 = (u16)(cel_attr2 | (u16)(((attr10 >> 16) & 0xFFu) << 8));
        out_attr2     = (u16)((u32)out_attr2 + attr2base);

        /* ── OBJ-relative position (within the sprite's cel) ── */
        /* attr1 bits[8:0] = 9-bit signed X offset */
        s32 xObjOff = (s32)((s32)(cel_attr1 << 23) >> 23);
        /* attr0 bits[7:0] = 8-bit signed Y offset */
        s32 yObjOff = (s32)((s32)(cel_attr0 << 24) >> 24);

        /* ── OBJ size from shape/size fields ── */
        u32 shape  = (u32)(cel_attr0 >> 14);   /* attr0[15:14] */
        u32 size   = (u32)(cel_attr1 >> 14);   /* attr1[15:14] */
        u32 dimIdx = (shape << 2) | size;       /* 0..11 */
        u32 width  = drawData->objDim[dimIdx * 2];
        u32 height = drawData->objDim[dimIdx * 2 + 1];

        /* Double-size is read from the merged attr0 (the sprite's own flags
         * set it, not the cel), and doubles the OBJ's box for everything
         * below: the centring, the culling (asm: and r1, r7, #0x300;
         * lsleq r0, r0, #1). */
        int dbl = (out_attr0 & 0x300u) == 0x300u;
        if (dbl) {
            width  <<= 1;
            height <<= 1;
        }

        /* ── Affine or non-affine path ── */
        if (r6 & 0x200u) {
            /* Affine path (bit 25 of attr10 / bit 9 of r6) */
            s32 quarterW = (s32)width  >> 2;
            s32 quarterH = (s32)height >> 2;
            s32 halfW    = (s32)width  >> 1;
            s32 halfH    = (s32)height >> 1;

            /* Adjust OBJ offset toward sprite centre.
             *   double-size → add quarter of the doubled box once
             *   otherwise   → add quarter-dim twice (= half-dim) */
            xObjOff += quarterW;
            yObjOff += quarterH;
            if (!dbl) {
                xObjOff += quarterW;
                yObjOff += quarterH;
            }

            /* Apply 2×2 matrix (Q8.8 fixed-point): new = M * offset - half_dim */
            s32 pa = drawData->affine[0];
            s32 pb = drawData->affine[1];
            s32 pc = drawData->affine[2];
            s32 pd = drawData->affine[3];

            s32 newX = (xObjOff * pa + yObjOff * pb) >> 8;
            s32 newY = (xObjOff * pc + yObjOff * pd) >> 8;
            xObjOff = newX - halfW;
            yObjOff = newY - halfH;
        } else {
            /* Non-affine: handle H/V flip */
            if (r6 & 0x10000000u) { /* H-flip (bit 12 of attr10 / bit 28 of r6) */
                xObjOff += (s32)width;
                xObjOff  = -xObjOff;
            }
            if (r6 & 0x20000000u) { /* V-flip (bit 13 of attr10 / bit 29 of r6) */
                yObjOff += (s32)height;
                yObjOff  = -yObjOff;
            }
        }

        /* ── Final screen position ── */
        s32 finalX = xObjOff + xPos;
        s32 finalY = yObjOff + yPos;

        /* ── Off-screen culling (unsigned arithmetic, matching GBA behaviour) ──
         * GBA screen: 240×160.  An OBJ is visible if its right edge is
         * within [0, width+240) and its bottom edge within [0, height+160).
         * The unsigned comparison naturally catches negative positions that
         * wrap to large values.                                                  */
        u32 xEnd   = (u32)(finalX + (s32)width);
        u32 xBound = (u32)width + 240u;
        u32 yEnd   = (u32)(finalY + (s32)height);
        u32 yBound = (u32)height + 160u;

        if (xEnd < xBound && yEnd < yBound) {
            /* ── Write merged OAM entry ── */
            /* attr0: clear Y field [7:0], insert finalY */
            out_attr0 = (u16)((out_attr0 & 0xFF00u) | ((u32)finalY & 0xFFu));
            /* attr1: clear X field [8:0], insert finalX */
            out_attr1 = (u16)((out_attr1 & 0xFE00u) | ((u32)finalX & 0x1FFu));

            u16 *oam16 = (u16 *)dest;
            oam16[0] = out_attr0;
            oam16[1] = out_attr1;
            oam16[2] = out_attr2;
            /* oam16[3] = affine param slot – leave as-is */

            dest = (u32 *)((u8 *)dest + destInc);
            objCount++;
        }

        /* Advance source pointer (6 bytes per OBJ entry: 3 × u16) */
        src = (const u16 *)((const u8 *)src + srcInc);
    }

    return objCount;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * func_0804e1c8 – flush all active sprites to the OAM shadow buffer
 *
 * Iterates the SpriteHandler's z-ordered linked list (forward on even cycles,
 * backward on odd cycles, alternating each frame).  For each visible sprite
 * it builds a struct_0804cb88 and calls func_0804cb88 → func_0804e418.
 * Also advances each sprite's animation timer.
 *
 * This overrides the __attribute__((weak)) no-op stub in auto_stubs.c.
 * ═══════════════════════════════════════════════════════════════════════════ */
void func_0804e1c8(struct SpriteHandler *handler)
{
    if (!handler) return;

    struct struct_0804cb88 drawData;

    /* Install the correct OBJ dimension table in drawData.
     * (The global D_08bd0cae is only 2 bytes on PC; use our local copy.) */
    memcpy(drawData.objDim, pc_obj_dim, sizeof(drawData.objDim));

    u16 totalCycles  = handler->totalCycles;
    u16 remainingOAM = handler->objAmount;   /* tracks available OAM slots */

    u32 *oamPos;
    s8   srcInc, destInc;
    s16  currentID;

    if (totalCycles & 1u) {
        /* Odd cycle → backward: start from highest-z sprite, use last OAM slot */
        currentID = handler->zLinkEnd;
        oamPos    = handler->oamBuffer + (u32)(handler->objAmount - 1u) * 2u;
        srcInc    = (s8)(-6);
        destInc   = (s8)(-8);
    } else {
        /* Even cycle → forward: start from lowest-z sprite, use first OAM slot */
        currentID = handler->zLinkStart;
        oamPos    = handler->oamBuffer;
        srcInc    = 6;
        destInc   = 8;
    }

    drawData.srcInc  = srcInc;
    drawData.destInc = destInc;

    while (currentID >= 0) {
        struct Sprite *sp = &handler->sprites[currentID];

        /* Pre-fetch next ID before we potentially modify the sprite */
        s16 nextID = (totalCycles & 1u) ? sp->zLinkPrev : sp->zLinkNext;

        /* ── Render visible sprite ── */
        if (sp->visible) {
            s8          cel     = sp->currentCel;
            const u16  *celData = (const u16 *)sp->animation[cel].cel;

            if (celData) {
                u16 celObjCount = celData[0];
                if (remainingOAM >= celObjCount) {
                    /* World-space anchor: sprite pos – (origin + handler offset) */
                    s16 xOrig = (s16)((u16)(sp->xOrigin ? (u16)*sp->xOrigin : 0u)
                                      + handler->xPos);
                    s16 yOrig = (s16)((u16)(sp->yOrigin ? (u16)*sp->yOrigin : 0u)
                                      + handler->yPos);

                    /* attr10: sprite flags + priority from zDepth bits[15:14] */
                    u32 attr10 = sp->oamAttributes
                               | ((u32)((sp->zDepth >> 14) & 3u) << 18);

                    /* attr2 base: (basePalette << 12) | (u16)baseTile */
                    u32 attr2 = ((u32)(u8)sp->basePalette << 12)
                              | (u16)(s16)sp->baseTile;

                    /* Affine matrix (negate middle two entries per assembly).
                     * Gate on the affine attribute bit (25) like the GBA asm —
                     * affineParams may hold stale garbage on recycled slots. */
                    if ((sp->oamAttributes & 0x02000000u) && sp->affineParams) {
                        drawData.affine[0] =  sp->affineParams[0];
                        drawData.affine[1] = (s16)(-sp->affineParams[1]);
                        drawData.affine[2] = (s16)(-sp->affineParams[2]);
                        drawData.affine[3] =  sp->affineParams[3];
                    } else {
                        drawData.affine[0] = drawData.affine[1] = 0;
                        drawData.affine[2] = drawData.affine[3] = 0;
                    }

                    drawData.src      = celData;
                    drawData.dest     = oamPos;
                    drawData.objCount = 0;
                    drawData.objTotal = 0;
                    drawData.xPos     = (u16)((s16)sp->xPos - xOrig);
                    drawData.yPos     = (u16)((s16)sp->yPos - yOrig);
                    drawData.attr10   = attr10;
                    drawData.attr2    = attr2;
                    drawData.objLimit = remainingOAM;

                    u16 used = (u16)func_0804cb88(&drawData);
                    remainingOAM -= used;

                    /* Advance OAM buffer pointer (each OAM entry = 2 u32 = 8 bytes) */
                    if (totalCycles & 1u) {
                        oamPos -= (u32)used * 2u;   /* backward */
                    } else {
                        oamPos += (u32)used * 2u;   /* forward  */
                    }
                }
            }
        }

        /* ── Animation update ──────────────────────────────────────────────
         * Skip if: sprite or handler is paused,
         *          OR sprite->update is set (caller is driving animation manually),
         *          OR sprite is invisible AND doesn't have the always-animate flag.
         * The update bit (sprite->update != 0) means "auto-update disabled".
         * The always-animate flag is bit 24 of oamAttributes (0x01000000).       */
        {
            u16 isPaused = sp->paused ? sp->paused : handler->paused;

            if (!isPaused && !sp->update) {
                int shouldAnimate = sp->visible;
                if (!shouldAnimate) {
                    shouldAnimate =
                        (sp->oamAttributes & 0x01000000u) ? 1 : 0;
                }

                if (shouldAnimate) {
                    /* Decrement cel timer by animationSpeed (u16 arithmetic) */
                    u16 celTime = (u16)sp->currentCelTime;
                    u16 speed   = (u16)sp->animationSpeed;
                    celTime -= speed;
                    sp->currentCelTime = (s8_8)celTime;

                    /* Advance to next cel when timer expires (signed <= 0) */
                    while ((s16)sp->currentCelTime <= 0) {
                        /* nextCel = (u8)celInc + (u8)currentCel reinterpreted as s8 */
                        s8 nextCel = (s8)((u8)sp->celInc + (u8)sp->currentCel);
                        sprite_update_anim_cel(handler, currentID, nextCel, 0);

                        /* Sprite may have been deleted by its callback */
                        if (!sp->allocated) break;
                    }
                }
            }
        }

        currentID = nextID;
    }

    /* Record number of OAM slots used this frame */
    handler->unk20      = handler->objAmount - remainingOAM;
    /* Increment cycle counter (toggles forward/backward direction each frame) */
    handler->totalCycles = totalCycles + 1u;
}

#endif /* PLATFORM_PC */
