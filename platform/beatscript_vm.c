/*
 * Beatscript command processor (PC).
 *
 * This is a C translation of the Thumb routine func_0800cb28 in
 * asm/code_0800b778/asm_0800cb28.s.  Every case below mirrors the jump-table
 * entry of the same opcode; the register names in the comments refer to that
 * listing so the two can be diffed:
 *
 *     R6 = cmd->param2   (an address *or* a small integer, per opcode)
 *     R7 = cmd->param1   (the upper 24 bits of the first script word)
 *     R8 = cmd->param3
 *     R9 = &D_030053c0.threads[threadID]
 *
 * Opcodes whose jump-table slot is branch_0800df9e (the function epilogue) are
 * no-ops on GBA as well, and are handled by `default:` here.
 */
#ifdef PLATFORM_PC

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__APPLE__) || defined(__linux__)
#  define RTPC_HAVE_DLADDR 1
#  include <dlfcn.h>
#endif

#include "global.h"
#include "include/types.h"
#include "include/scenes.h"
#include "src/code_0800b778.h"
#include "src/audio.h"
#include "src/graphics_table.h"
#include "src/code_08007468.h"
#include "src/code_080068f8.h"
#include "src/backdrop.h"
#include "src/scenes/gameplay.h"

// Called through cmd->param2 by opcodes 0x02/0x03/0x04.  The GBA calls these
// via _call_via_r6 with 0-2 arguments in r0/r1; on any modern ABI passing a
// full-width argument to a callee that only reads the low 32 bits is safe, so
// the widest prototype is used and every callee sees the arguments it expects.
// The return value feeds D_030053c0.globalVariable, which opcode 0xB0 then
// jumps to as a script address — so it has to be pointer-sized.  As s32 it
// clipped the top half off, and drum_studio_drumming jumped to 0x008d41e0
// instead of 0x1008d41e0 on its first call_result.
typedef intptr_t (*BsFunc0)(void);
typedef intptr_t (*BsFunc1)(uintptr_t);
typedef intptr_t (*BsFunc2)(uintptr_t, uintptr_t);

void func_0800cb28(u32 threadID) {
    struct BeatscriptThread *thread = &D_030053c0.threads[threadID];
    const struct Beatscript *cmd = thread->currentCmd;
    uintptr_t p2, p3;
    u32 op, p1;

    if (!thread->active) return;

    // A thread can be left with a NULL script (e.g. a SubScene slot with no
    // script of its own); the GBA never reaches this state, so guard instead
    // of faulting at address 0.
    if (cmd == NULL) { thread->active = 0; return; }

    // /* 0800cb74 */ the stream pointer advances before the command runs, so
    // call/goto/loop can overwrite it afterwards.
    thread->currentCmd = cmd + 1;

    op = cmd->command;
    p1 = cmd->param1;
    p2 = (uintptr_t)cmd->param2;
    p3 = cmd->param3;

    if (getenv("RTPC_BS")) {
        // Resolve param2 to a symbol name where it is a function pointer, so
        // the trace reads as script source rather than as raw addresses.
        const char *sym = "";
#ifdef RTPC_HAVE_DLADDR
        Dl_info info;
        if ((op == 0x02 || op == 0x03 || op == 0x04) && p2 &&
            dladdr((void *)p2, &info) && info.dli_sname)
            sym = info.dli_sname;
#endif
        extern int gPcFrameNo;
        fprintf(stderr, "[BS] f=%d th=%u cmd=%p op=0x%02x p1=%u p2=%p p3=%llu %s\n",
                gPcFrameNo, threadID, (const void *)cmd, op, (unsigned)p1,
                (const void *)p2, (unsigned long long)p3, sym);
        fflush(stderr);
    }

    switch (op) {

    /* ---- flow ---------------------------------------------------------- */

    case 0x00: // rest — jump_0800ce78
        thread->timeUntilNext += (s32)p3 << 8;
        break;

    case 0x01: // stop — jump_0800ce88 (clears bit 0 = active)
        thread->active = 0;
        thread->currentCmd = cmd;   // stay on STOP so re-entry is harmless
        break;

    case 0x0D: // call — jump_0800ce9a
        if (thread->stackCounter < 8)
            thread->jumpStack[thread->stackCounter] = thread->currentCmd;
        thread->stackCounter = (thread->stackCounter + 1) & 0xF;
        thread->currentCmd = (const struct Beatscript *)p2;
        break;

    case 0xB0: // call_result — jump_0800cece: call GLOBAL_VARIABLE as a script
        if (D_030053c0.globalVariable != 0) {
            if (thread->stackCounter < 8)
                thread->jumpStack[thread->stackCounter] = thread->currentCmd;
            thread->stackCounter = (thread->stackCounter + 1) & 0xF;
            thread->currentCmd =
                (const struct Beatscript *)D_030053c0.globalVariable;
        }
        break;

    case 0x0E: // return — jump_0800cf10
        thread->stackCounter = (thread->stackCounter - 1) & 0xF;
        if (thread->stackCounter < 8)
            thread->currentCmd = thread->jumpStack[thread->stackCounter];
        break;

    case 0x0F: // goto — jump_0800cf3e
        thread->currentCmd = (const struct Beatscript *)p2;
        break;

    case 0x10: // loop_start — jump_0800cf50
        thread->loopStart = thread->currentCmd;
        break;

    case 0x11: // loop_end — jump_0800cf5a
        // bypassLoops leaves the loop by simply falling through.
        if (!D_030053c0.bypassLoops)
            thread->currentCmd = thread->loopStart;
        break;

    case 0xB1: // rest_reset — jump_0800cf46
        thread->timeUntilNext = 0;
        break;

    /* ---- function calls ------------------------------------------------ */

    case 0x02: { // scene_run — jump_0800d1ae: f(&localVariables[thread], p3)
        BsFunc2 f = (BsFunc2)p2;
        if (f)
            D_030053c0.globalVariable =
                f((uintptr_t)&D_030053c0.localVariables[threadID], p3);
        break;
    }

    case 0x03: { // run — jump_0800d1d0: f(p3)
        BsFunc1 f = (BsFunc1)p2;
        if (f) D_030053c0.globalVariable = f(p3);
        break;
    }

    case 0x04: { // run2 — jump_0800d1d8: f(p3, p1)
        BsFunc2 f = (BsFunc2)p2;
        if (f) D_030053c0.globalVariable = f(p3, p1);
        break;
    }

    /* ---- absolute-address variable ops --------------------------------- */
    // p1 selects the width: 0 = 32-bit, 1 = 16-bit, 2 = 8-bit.

    case 0x05: // set_var — jump_0800d1ec
        if (p2) {
            if      (p1 == 1) *(u16 *)p2 = (u16)p3;
            else if (p1 == 2) *(u8  *)p2 = (u8)p3;
            else              *(u32 *)p2 = (u32)p3;
        }
        break;

    case 0x06: // add_var — jump_0800d214
        if (p2) {
            if      (p1 == 1) *(u16 *)p2 += (u16)p3;
            else if (p1 == 2) *(u8  *)p2 += (u8)p3;
            else              *(u32 *)p2 += (u32)p3;
        }
        break;

    case 0x07: // set_bit — jump_0800d242
        if (p2) {
            if      (p1 == 1) *(u16 *)p2 |= (u16)(p1 << p3); // GBA: r7 << r8
            else if (p1 == 2) *(u8  *)p2 |= (u8)(1u << p3);
            else              *(u32 *)p2 |= (1u << p3);
        }
        break;

    case 0x08: // clear_bit — jump_0800d280
        if (p2) {
            if      (p1 == 1) *(u16 *)p2 &= (u16)~(p1 << p3);
            else if (p1 == 2) *(u8  *)p2 &= (u8)~(1u << p3);
            else              *(u32 *)p2 &= ~(1u << p3);
        }
        break;

    /* ---- scene-data variable ops --------------------------------------- */
    // p2 is a *byte* offset into gCurrentSceneData for every width.

    case 0x09: // scene_set_var — jump_0800d2be
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p2;
            if      (p1 == 1) *(u16 *)a = (u16)p3;
            else if (p1 == 2) *a        = (u8)p3;
            else              *(u32 *)a = (u32)p3;
        }
        break;

    case 0x0A: // scene_add_var — jump_0800d2f0
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p2;
            if      (p1 == 1) *(u16 *)a += (u16)p3;
            else if (p1 == 2) *a        += (u8)p3;
            else              *(u32 *)a += (u32)p3;
        }
        break;

    case 0x0B: // scene_set_bit — jump_0800d32a
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p2;
            if      (p1 == 1) *(u16 *)a |= (u16)(1u << p3);
            else if (p1 == 2) *a        |= (u8)(1u << p3);
            else              *(u32 *)a |= (1u << p3);
        }
        break;

    case 0x0C: // scene_clear_bit — jump_0800d372
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p2;
            if      (p1 == 1) *(u16 *)a &= (u16)~(1u << p3);
            else if (p1 == 2) *a        &= (u8)~(1u << p3);
            else              *(u32 *)a &= ~(1u << p3);
        }
        break;

    /* ---- conditionals -------------------------------------------------- */

    case 0x12:   // if_eq  — jump_0800cf78
    case 0x13: { // if_neq — same handler, result inverted for 0x13
        u32 differs = 1;
        if (p2) {
            if      (p1 == 1) differs = ((u16)*(u16 *)p2 != (u16)p3);
            else if (p1 == 2) differs = ((u8) *(u8  *)p2 != (u8)p3);
            else              differs = (*(u32 *)p2 != (u32)p3);
        }
        if (op == 0x13) differs = !differs;
        // differs != 0 → the body must be skipped.
        if (differs)
            thread->currentCmd = beatscript_stream_jump_cond_if(
                (struct Beatscript *)thread->currentCmd);
        break;
    }

    case 0x14: // else — jump_0800d008
        thread->currentCmd = beatscript_stream_jump_cond_else(
            (struct Beatscript *)thread->currentCmd);
        break;

    case 0x16: // if_set — jump_0800cfd0 (p3 is a mask, not a bit index)
        if (!p2 || (*(u32 *)p2 & (u32)p3) == 0)
            thread->currentCmd = beatscript_stream_jump_cond_if(
                (struct Beatscript *)thread->currentCmd);
        break;

    case 0x17: // if_clear — jump_0800cfec
        if (!p2 || (*(u32 *)p2 & (u32)p3) != 0)
            thread->currentCmd = beatscript_stream_jump_cond_if(
                (struct Beatscript *)thread->currentCmd);
        break;

    case 0x21:   // scene_if_eq  — jump_0800d0bc
    case 0x22: { // scene_if_neq
        u32 differs = 1;
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p2;
            if      (p1 == 1) differs = ((u16)*(u16 *)a != (u16)p3);
            else if (p1 == 2) differs = (*a != (u8)p3);
            else              differs = (*(u32 *)a != (u32)p3);
        }
        if (op == 0x22) differs = !differs;
        if (differs)
            thread->currentCmd = beatscript_stream_jump_cond_if(
                (struct Beatscript *)thread->currentCmd);
        break;
    }

    case 0x1A: { // switch — jump_0800d012
        s32 value = 0;
        if (p1 == 3) {                       // p3 is a function to call
            BsFunc0 f = (BsFunc0)p3;
            if (f) value = f();
        } else if (p3) {
            if      (p1 == 1) value = *(u16 *)p3;
            else if (p1 == 2) value = *(u8  *)p3;
            else              value = *(s32 *)p3;
        }
        thread->currentCmd = beatscript_stream_jump_cond_switch(
            (struct Beatscript *)thread->currentCmd, value);
        break;
    }

    case 0x23: { // scene_switch — jump_0800d116 (offset is in p3 here)
        s32 value = 0;
        if (gCurrentSceneData) {
            u8 *a = (u8 *)gCurrentSceneData + p3;
            if      (p1 == 1) value = *(u16 *)a;
            else if (p1 == 2) value = *a;
            else              value = *(s32 *)a;
        }
        thread->currentCmd = beatscript_stream_jump_cond_switch(
            (struct Beatscript *)thread->currentCmd, value);
        break;
    }

    case 0x1D: // break — jump_0800d048
        thread->currentCmd = beatscript_stream_jump_cond_break(
            (struct Beatscript *)thread->currentCmd);
        break;

    case 0x20: // end_while — jump_0800d0ac
        thread->currentCmd = beatscript_stream_jump_cond_end_while(
            (struct Beatscript *)(thread->currentCmd - 1));
        break;

    /* ---- music / sound ------------------------------------------------- */

    case 0x28: // play_music_in — jump_0800d3ba (p2 <= 0xFE selects a player)
        if (p2 <= 0xFE)
            scene_set_music_with_soundplayer((struct SongHeader *)p3, (s32)p2);
        else
            scene_set_music((struct SongHeader *)p3);
        break;

    case 0x3A: // add_music — jump_0800d3d4
        if (p2 <= 0xFE)
            scene_play_music_with_soundplayer((struct SongHeader *)p3, (s32)p2);
        else
            scene_play_music((struct SongHeader *)p3);
        break;

    case 0x29: // play_sfx — jump_0800d3ee
        play_sound((struct SongHeader *)p3);
        break;

    case 0xAE: // play_sfx_vol_pitch — jump_0800d3f8
        play_sound_w_pitch_volume((struct SongHeader *)p3, (s32)p1, (s32)p2);
        break;

    case 0xB2: { // play_sfx_synced_pitch — jump_0800d406
        struct SoundPlayer *sp = play_sound_w_pitch_volume(
            (struct SongHeader *)p3, (s32)(p2 & 0xFFFF), (s32)((s32)p2 >> 16));
        if (p1) {
            u32 speed = ((u32)get_beatscript_tempo() << 8) / p1;
            set_soundplayer_speed(sp, (u16)speed);
        }
        break;
    }

    case 0x35: // fade_music_in — jump_0800d434 (p1 != 0 → duration in ticks)
        scene_fade_music_in(p1 ? (u32)ticks_to_frames((u32)p3) : (u32)p3);
        break;

    case 0x36: // fade_music_out — jump_0800d44a
        scene_fade_music_out(p1 ? (u32)ticks_to_frames((u32)p3) : (u32)p3);
        break;

    case 0x38: // fade_sfx_out — jump_0800d460
        fade_out_sound((struct SoundPlayer *)p2,
                       (u16)ticks_to_frames((u16)p3));
        break;

    case 0x3C: // set_speed — jump_0800d4ee
        set_beatscript_speed((u16)p3);
        break;

    case 0x3D: // set_music_pitch_s — jump_0800d514
        scene_set_music_pitch_env((s16)p3);
        break;

    case 0x3E: // set_music_pitch — jump_0800d522
        scene_set_music_pitch((s16)p3);
        break;

    case 0x4C: // mod_tempo — jump_0800d800: interpolate to p2 over p3
        scene_interpolate_tempo((u32)p2, (u32)p3);
        break;

    case 0xAF: // increase_speed — jump_0800d804: multiply the current tempo
        scene_interpolate_tempo(((u32)get_beatscript_tempo() * (u32)p2) >> 8,
                                (u32)p3);
        break;

    case 0x4E: // mod_music_pitch — jump_0800d816
        scene_interpolate_music_pitch((s32)p2, (u32)p3);
        break;

    case 0x50: // mod_music_volume — jump_0800d820
        scene_interpolate_music_volume((u32)p2, (u32)ticks_to_frames((u32)p3));
        break;

    case 0xAC: // set_music_track_volume — jump_0800d830
        scene_set_music_track_volume((u16)p1, (u16)p2);
        break;

    case 0xAD: // mod_music_track_volume — jump_0800d83e
        scene_set_music_track_volume((u16)p1, D_030053c0.musicTrkVolume);
        scene_interpolate_music_track_volume((u32)p2,
                                             (u32)ticks_to_frames((u32)p3));
        break;

    /* ---- graphics / screen --------------------------------------------- */

    case 0x39: // load_graphics — jump_0800d4d0
        func_08002e78((void *)p2);
        break;

    case 0x47: // task_load_graphics — jump_0800d4da
        func_08002ee0((u16)get_current_mem_id(), (void *)p2, (s32)p3);
        break;

    case 0x3F: { // mod_lcd_blend — jump_0800d530
        u32 time = (u32)p3 & 0x7FFFFFFF;
        if ((s32)p3 < 0) time = (u32)ticks_to_frames(time);
        interp_lcd_blend_mode((u16)get_current_mem_id(), (u32)p2, time, (u32)p1);
        break;
    }

    case 0x48: // set_backdrop — jump_0800d79c
        func_080041d0((u16)p1, (u16)p2, (u16)p3);
        break;

    case 0x49: // set_video_mode — jump_0800d7b2
        D_03004b10.DISPCNT = (u16)((D_03004b10.DISPCNT & 0xFFF8) | (u16)p2);
        break;

    case 0x4A: // fade_screen — jump_0800d7cc (p1 != 0 → fade in)
        if (p1)
            func_080070c4((u16)ticks_to_frames((u32)p2), (u16)p3);
        else
            func_0800703c((u16)ticks_to_frames((u32)p2), (u16)p3);
        break;

    default:
        // Opcodes 0x15 (end_if), 0x1B (end_switch), 0x1C (case), 0x4F
        // (default) and every reserved slot jump straight to the function
        // epilogue on GBA, so doing nothing is the faithful behaviour.  The
        // stream pointer has already advanced, so the caller's loop still
        // makes progress.  Commands 0x80+ (sprite/motion) are not translated
        // yet and land here as well — they are visual, not timing, so
        // skipping them keeps the beat correct.
        break;
    }
}

#endif // PLATFORM_PC
