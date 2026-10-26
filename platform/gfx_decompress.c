#ifdef PLATFORM_PC
/* ── Rhythm Tengoku's own graphics decompressor ─────────────────────────────
 *
 * C translation of the ARM routine `decompress_gfx_rom`
 * (asm/code_08000a00.s:353-502), which the game copies into IWRAM and calls
 * through the function pointer set up by decompress_gfx_init/_resume.
 *
 * It is NOT a GBA BIOS decompressor.  The PC build previously answered this
 * symbol with a hand-written LZ77/RLE routine taking (src, dest) — a different
 * algorithm AND a different signature from the one decompress_gfx_init calls
 * it with, so on PC every call read a bogus header out of the progress struct
 * and wrote through whatever happened to be in the second argument register.
 *
 * ── The format ────────────────────────────────────────────────────────────
 * Output is 4bpp pixels, four per 16-bit halfword.  The stream is driven by
 * two independent bit windows, refilled 32 bits at a time from the `window1`
 * and `window2` arrays that sit alongside the pixel data:
 *
 *   window1 bit = 0   raw: copy one halfword (4 pixels) straight through.
 *   window1 bit = 1   a run, whose kind the next window2 bit selects:
 *     window2 bit = 0   1bpp run.  One halfword holds two 4-bit colours in
 *                       its low byte and a repeat count in its high byte;
 *                       each following halfword is 16 one-bit pixel
 *                       selectors -> 16 pixels (4 halfwords out).
 *     window2 bit = 1   2bpp run.  One halfword holds four 4-bit colours;
 *                       the next holds four 2-bit selectors in its high byte
 *                       (one halfword out) and the repeat count in its low
 *                       byte; each following halfword is eight 2-bit
 *                       selectors -> 8 pixels (2 halfwords out).
 *
 * A "unit" is one window1 symbol — one raw halfword or one whole run — and
 * `count`'s high half counts them down.  `lim1` is a work budget in
 * cycle-ish units: when it runs out mid-stream the routine writes its state
 * back and returns 0, so the caller can resume on a later frame.  Returning 1
 * means the last unit was consumed and the state is NOT written back.
 *
 * ── progress layout (struct GFXDecompressProgress) ─────────────────────────
 *   +0x00 data     source halfword cursor
 *   +0x04 size     DESTINATION cursor — misnamed by the decompilation; the
 *                  ARM code stores through it (`strh r0, [r2], #2`)
 *   +0x08 count    bits 31..16 units left, bits 12..8 window2 bit counter,
 *                  bits 4..0 window1 bit counter (both start at 0x20)
 *   +0x0C curwin1  +0x10 curwin2   the live 32-bit windows
 *   +0x14 win1     +0x18 win2      refill cursors
 *   +0x1C lim1     +0x20 lim2      work budget (lim1 is not saved back)
 */

#include "global.h"
#include "graphics.h"
#include "src/graphics_table.h"

u32 decompress_gfx_rom(struct GFXDecompressProgress *p)
{
    const u16 *src    = p->data;
    u16       *dest   = (u16 *)p->size;
    u32        count  = p->count;
    u32        win1   = p->curwin1;
    u32        win2   = p->curwin2;
    const u32 *win1p  = p->win1;
    const u32 *win2p  = p->win2;
    s32        budget = (s32)p->lim1;

    for (;;) {
        u32 raw = win1 & 1;
        win1 = (win1 >> 1) | (win1 << 31);

        if (!raw) {
            *dest++ = *src++;
            count -= 0x10000;
            if ((s32)count < 0) return TRUE;
            budget -= 8;
        } else {
            u32 wide = win2 & 1;
            win2 = (win2 >> 1) | (win2 << 31);

            if (!wide) {
                /* 1bpp run: low byte of `pal` = two colours, high byte = the
                 * repeat count.  Subtracting 0x100 never disturbs the colours,
                 * which is why the ARM code keeps both in one register. */
                u32 pal = *src++;
                do {
                    u32 sel = *src++;
                    u32 i;
                    for (i = 0; i < 16; i += 4) {
                        u32 out = 0, j;
                        for (j = 0; j < 4; j++) {
                            u32 shift = ((sel >> (i + j)) & 1) * 4;
                            out |= ((pal >> shift) & 0xF) << (j * 4);
                        }
                        *dest++ = out;
                    }
                    budget -= 12;
                    pal -= 0x100;
                } while ((s32)pal >= 0);
            } else {
                /* 2bpp run: `pal` is four colours, and the first selector
                 * halfword doubles as the run length in its low byte. */
                u32 pal = *src++;
                u32 sel = *src++;
                u32 out = 0, j;
                s32 remaining;

                for (j = 0; j < 4; j++) {
                    u32 shift = ((sel >> (8 + j * 2)) & 3) * 4;
                    out |= ((pal >> shift) & 0xF) << (j * 4);
                }
                *dest++ = out;

                remaining = sel & 0xFF;
                do {
                    u32 i;
                    sel = *src++;
                    for (i = 0; i < 16; i += 8) {
                        u32 word = 0;
                        for (j = 0; j < 4; j++) {
                            u32 shift = ((sel >> (i + j * 2)) & 3) * 4;
                            word |= ((pal >> shift) & 0xF) << (j * 4);
                        }
                        *dest++ = word;
                    }
                    budget -= 16;
                } while (--remaining >= 0);
            }

            count -= 0x10000;
            if ((s32)count < 0) return TRUE;

            count -= 0x100;
            if ((count & 0x1F00) == 0) {
                count |= 0x2000;
                win2 = *win2p++;
            }
        }

        count -= 1;
        if ((count & 0x1F) == 0) {
            count |= 0x20;
            win1 = *win1p++;
        }

        if (budget < 0) {
            /* Out of budget: save everything except lim1, exactly as the ARM
             * `stm r0, {r1-r7}` does — decompress_gfx_resume re-seeds it. */
            p->data    = src;
            p->size    = (uintptr_t)dest;
            p->count   = count;
            p->curwin1 = win1;
            p->curwin2 = win2;
            p->win1    = win1p;
            p->win2    = win2p;
            return FALSE;
        }
    }
}

#endif /* PLATFORM_PC */
