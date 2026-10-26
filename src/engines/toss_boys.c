#include "engines/toss_boys.h"
#include "src/scenes/gameplay.h"
#include "src/code_08001360.h"
#include "src/code_08007468.h"
#include "src/code_0800b778.h"
#include "src/task_pool.h"
#include "src/lib_0804ca80.h"
#include "src/audio.h"
#include "src/affine_param.h"
#include "src/text_printer.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gTossBoys ((struct TossBoysEngineData *)gCurrentEngineData)


/* TOSS BOYS */


#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e824.s"
#else
// Create Remix 5 Guitarist
void func_0803e824(void) {
    if (gTossBoys->version == ENGINE_VER_TOSS_REMIX_5) {
        gTossBoys->soshi = sprite_create(gSpriteHandler, anim_toss_remix_5_soshi_strum_pop, 0x7f, 116, 56, 0x4c00, 1, 0x7f, 0x8000);
    } else {
        gTossBoys->soshi = -1;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e884.s"
#else
// Engine Event 0x08 (Show Remix 5 Guitarist)
void func_0803e884(void) {
    if (gTossBoys->soshi >= 0) {
        sprite_set_visible(gSpriteHandler, gTossBoys->soshi, TRUE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e8b4.s"
#else
// Guitarist Beat Strum
void func_0803e8b4(void) {
    if ((gTossBoys->soshi >= 0) && (gTossBoys->soshiTimer == 0)) {
        sprite_set_anim(gSpriteHandler, gTossBoys->soshi, anim_toss_remix_5_soshi_strum_pop, 0, 1, 0x7f, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e908.s"
#else
// Guitarist Launch Strum
void func_0803e908(void) {
    if (gTossBoys->soshi >= 0) {
        sprite_set_anim(gSpriteHandler, gTossBoys->soshi, anim_toss_remix_5_soshi_strum_launch, 0, 1, 0x7f, 0);
        gTossBoys->soshiTimer = ticks_to_frames(0x3c);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e960.s"
#else
// Update Guitarist Timer
void func_0803e960(void) {
    if ((gTossBoys->soshi >= 0) && (gTossBoys->soshiTimer != 0)) {
        gTossBoys->soshiTimer--;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e990.s"
#else
// Get Animation
struct Animation *toss_boys_get_anim(u32 anim) {
    return toss_boys_anim_table[anim][gTossBoys->version];
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803e9b0.s"
#else
// Engine Event 0x03 (Play Drum Note at Tempo)
void func_0803e9b0(u32 drumID) {
    struct SoundPlayer *player;

    if (gTossBoys->ballActive && (gTossBoys->popTimer == 0) && !gTossBoys->popRecovered) {
        player = play_drumtech_note(drumID, 0x100, 0);
        set_soundplayer_speed(player, (u16)((s32)(get_beatscript_tempo() << 8) / 120));
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ea08.s"
#else
// Engine Event 0x04 (Set Next Cue's Drum Note)
void func_0803ea08(u32 drumNote) {
    gTossBoys->nextDrumNote = drumNote;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ea1c.s"
#else
// Graphics Init. 3
void toss_boys_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ea2c.s"
#else
// Graphics Init. 2
void toss_boys_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), toss_boys_gfx_tables[gTossBoys->version], 0x2000);
    run_func_after_task(task, toss_boys_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ea6c.s"
#else
// Graphics Init. 1
void toss_boys_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), toss_boys_buffered_textures);
    run_func_after_task(task, toss_boys_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ea98.s"
#else
// Game Engine Start
void toss_boys_engine_start(u32 version) {
    struct TextPrinter *textPrinter;
    u32 i;

    gTossBoys->version = version;
    toss_boys_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_0, TRUE, 0, 0, 2, 28, 0x4000);
    scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, 1);

    gTossBoys->boys[0] = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_BEAT_RED), 0, 185, 115, 0x4810, 1, 0x7f, 0);
    gTossBoys->boys[1] = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_BEAT_BLUE), 0, 161, 140, 0x4800, 1, 0x7f, 0);
    gTossBoys->boys[2] = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_BEAT_YELLOW), 0, 58, 124, 0x4800, 1, 0x7f, 0);
    for (i = 0; i < 3; i++) {
        gTossBoys->animTimer[i] = 0;
        gTossBoys->boyAction[i] = 0;
        gTossBoys->barelyTimer[i] = 0;
    }

    gTossBoys->dispenser = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_DISPENSER), 0x7f, 120, 110, 0x4900, 1, 0x7f, 0);
    gTossBoys->ball = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_BALL1), 0, 120, 120, 0x700, 0, 0, 0x8000);
    gTossBoys->ballAffine = scene_affine_group_alloc();
    assign_sprite_affine_param(gTossBoys->ball, gTossBoys->ballAffine);
    func_080022d8(gTossBoys->ballAffine);
    set_affine_scale_rotation(gTossBoys->ballAffine, 0x100, 0);
    gTossBoys->ballMotionTask = -1;
    gTossBoys->ballRotation = 0;
    gTossBoys->ballRotationSpeed = 0;
    gTossBoys->ballSquash = 0;
    gTossBoys->ballActive = FALSE;
    gTossBoys->ballWobbling = FALSE;
    gTossBoys->popTimer = 0;
    gTossBoys->popRecovered = FALSE;

    gTossBoys->arrow = sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_ARROW_RED), 0, 120, 80, 0x4f00, 1, 0, 0x8002);
    init_drumtech(&gTossBoys->drumTech);
    set_drumtech_bank(toss_boys_drumtech_bank);
    gTossBoys->nextDrumNote = 0xffff;
    gTossBoys->pitchBend = 0;
    gTossBoys->inputButtons = 0xf3;
    gameplay_set_input_buttons(0xf3, 0);
    for (i = 0; i < 3; i++) {
        gTossBoys->inputLockTimers[i] = 0;
    }

    textPrinter = text_printer_create_new(get_current_mem_id(), 2, 240, 30);
    if (gTossBoys->version == ENGINE_VER_TOSS_REMIX_5) {
        text_printer_set_x_y(textPrinter, 0, 8);
        text_printer_set_layer(textPrinter, 0x4c00);
        text_printer_center_by_content(textPrinter, FALSE);
    } else {
        text_printer_set_x_y(textPrinter, 0, 20);
        text_printer_set_layer(textPrinter, 0x4c00);
        text_printer_set_alignment(textPrinter, 2);
        text_printer_center_by_content(textPrinter, TRUE);
    }
    text_printer_set_palette(textPrinter, 0);
    text_printer_set_colors(textPrinter, 0);
    text_printer_set_line_spacing(textPrinter, 20);
    gameplay_set_text_printer(textPrinter);

    D_03004b10.BLDMOD = 0x3f41;
    D_03004b10.COLEV = 0x1000;
    gTossBoys->flashLevel = 0;
    func_0803e824();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ee14.s"
#else
// Engine Event 0x09 (STUB)
void toss_boys_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ee18.s"
#else
// Update Boy Timers
void func_0803ee18(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        if (gTossBoys->animTimer[i] != 0) {
            gTossBoys->animTimer[i]--;
        }
        if (gTossBoys->barelyTimer[i] != 0) {
            gTossBoys->barelyTimer[i]--;
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ee58.s"
#else
// Lock a Boy's Button for a While
void func_0803ee58(u32 boy, u32 frames) {
    if (gTossBoys->inputLockTimers[boy] <= frames) {
        gTossBoys->inputLockTimers[boy] = frames;
        gTossBoys->inputButtons &= ~toss_boys_button_masks[boy];
        gameplay_set_input_buttons(gTossBoys->inputButtons, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803eea0.s"
#else
// Update Button Locks
void func_0803eea0(void) {
    struct Animation *anim;
    u32 i, action;

    for (i = 0; i < 3; i++) {
        if (gTossBoys->inputLockTimers[i] == 0) {
            continue;
        }
        if (--gTossBoys->inputLockTimers[i] != 0) {
            continue;
        }

        gTossBoys->inputButtons |= toss_boys_button_masks[i];
        gameplay_set_input_buttons(gTossBoys->inputButtons, 0);
        action = gTossBoys->boyAction[i];
        anim = D_089e8660[action][i];
        if (anim != NULL) {
            sprite_set_anim(gSpriteHandler, gTossBoys->boys[i], anim, D_089e8690[action][i], 1, 0x7f, 0);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803ef64.s"
#else
// Update Ball Spin
void func_0803ef64(void) {
    s32 stretch;

    gTossBoys->ballSquash = (gTossBoys->ballSquash * 0xdc) >> 8;

    if (gTossBoys->ballWobbling) {
        gTossBoys->ballRotation += clamp_int32(gTossBoys->ballRotationSpeed, -0x18, 0x18);
        stretch = (D_08935fcc[gTossBoys->wobblePhase] * 100) >> 8;
        set_affine_stretch_rotation(gTossBoys->ballAffine, 0x140 + stretch, 0x140 - stretch, (s16)gTossBoys->ballRotation);
        gTossBoys->wobblePhase += 0x18;
    } else {
        gTossBoys->ballRotation += gTossBoys->ballRotationSpeed;
        set_affine_scale_rotation(gTossBoys->ballAffine, (s16)(0x100 + gTossBoys->ballSquash), (s16)gTossBoys->ballRotation);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f038.s"
#else
// Update Pitch Wobble (after a miss)
void func_0803f038(void) {
    s32 sign = (gTossBoys->pitchBend >= 0) ? 1 : -1;
    s32 bend, wobble;

    bend = (((sign * gTossBoys->pitchBend) * 0xf5) >> 8) * sign;
    gTossBoys->pitchBend = bend;
    wobble = ((gTossBoys->pitchBend >> 2) * gSineTable[gTossBoys->pitchPhase & 0x7ff]) >> 8;
    gTossBoys->pitchPhase += 0x100;
    set_soundplayer_pitch(get_soundplayer_from_id(5), (s16)((u16)gTossBoys->pitchBend + wobble));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f0b8.s"
#else
// Engine Event 0x06 (Flash Behind a Boy)
void func_0803f0b8(u32 boy) {
    if (gTossBoys->ballActive) {
        D_03004b10.BG_OFS[BG_LAYER_0].x = 0x100 - D_089e869c[boy].x;
        D_03004b10.BG_OFS[BG_LAYER_0].y = 0x80 - D_089e869c[boy].y;
        scene_start_integer_interp(1, ticks_to_frames(0x10), &gTossBoys->flashLevel, 0, 10);
        play_sound_w_pitch_volume(&s_ninja_wind_seqData, 0xa0, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f12c.s"
#else
// Engine Event 0x07 (Clear Flash)
void func_0803f12c(void) {
    gTossBoys->flashLevel = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f144.s"
#else
// Game Engine Update
void toss_boys_engine_update(void) {
    u32 blend;

    func_0803eea0();
    func_0803ee18();
    func_0803ef64();
    update_drumtech();
    func_0803f038();

    if (gTossBoys->popTimer != 0) {
        if (--gTossBoys->popTimer == 0) {
            gTossBoys->popRecovered = TRUE;
        }
    }

    blend = gTossBoys->ballActive ? gTossBoys->flashLevel : 0;
    D_03004b10.COLEV = blend | ((0x10 - blend) << 8);
    func_0803e960();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f1b8.s"
#else
// Game Engine Stop
void toss_boys_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f1bc.s"
#else
// Engine Event 0x00 (Hide Ball)
void func_0803f1bc(void) {
    sprite_set_visible(gSpriteHandler, gTossBoys->ball, FALSE);
    gTossBoys->ballTarget = 3;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f1f4.s"
#else
// Launch the Ball from the Dispenser
void func_0803f1f4(u32 target, u32 arcHeight, u32 travelTicks, s32 timingOffset) {
    struct Vector2 *from, *to;
    s32 task, duration;

    if (gTossBoys->ballActive) {
        return;
    }

    gTossBoys->ballActive = TRUE;
    gTossBoys->popTimer = 0;
    gTossBoys->popRecovered = FALSE;
    gTossBoys->ballSource = 3;
    gTossBoys->ballTarget = target;
    gTossBoys->ballArcHeight = arcHeight;
    gTossBoys->ballTravelTicks = travelTicks;
    gTossBoys->ballWobbling = FALSE;

    from = &D_089e86a8[gTossBoys->ballSource];
    to = &D_089e86a8[gTossBoys->ballTarget];
    task = gTossBoys->ballMotionTask;
    if ((task >= 0) && get_task_state(task)) {
        force_cancel_task(task);
    }

    duration = ticks_to_frames(gTossBoys->ballTravelTicks) - (timingOffset - 5);
    gTossBoys->ballMotionTask = scene_set_sprite_motion_sine_wave(gTossBoys->ball, from->x, from->y, to->x, to->y,
                                                                  (s16)gTossBoys->ballArcHeight, (u16)duration);
    sprite_set_visible(gSpriteHandler, gTossBoys->ball, TRUE);
    sprite_set_anim_cel(gSpriteHandler, gTossBoys->dispenser, 0);
    play_sound(&s_f_toss_ball_seqData);
    sprite_set_anim(gSpriteHandler, gTossBoys->arrow, toss_boys_get_anim(toss_boys_arrow_anim_ids[target]), 0, 1, 0, 2);
    sprite_set_visible(gSpriteHandler, gTossBoys->arrow, TRUE);
    func_0803e908();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f390.s"
#else
// Engine Event 0x01 (Launch: target | height << 2 | ticks << 12)
void func_0803f390(u32 arg) {
    func_0803f1f4(arg & 3, (arg >> 2) & 0x3ff, arg >> 12, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f3b0.s"
#else
// Engine Event 0x02 (Next Pass: target | height << 2 | ticks << 12)
void func_0803f3b0(u32 arg) {
    gTossBoys->ballSource = gTossBoys->ballTarget;
    gTossBoys->ballTarget = arg & 3;
    gTossBoys->ballArcHeight = (arg >> 2) & 0x3ff;
    gTossBoys->ballTravelTicks = arg >> 12;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f400.s"
#else
// Pass the Ball
void func_0803f400(struct TossBoysCue *info, s32 timingOffset) {
    struct Vector2 *from = &D_089e86a8[info->from];
    struct Vector2 *to = &D_089e86a8[info->to];
    s32 task = gTossBoys->ballMotionTask;
    s32 duration;

    if ((task >= 0) && get_task_state(task)) {
        force_cancel_task(task);
    }

    duration = ticks_to_frames(info->travelTicks) - (timingOffset - 5);
    gTossBoys->ballMotionTask = scene_set_sprite_motion_sine_wave(gTossBoys->ball, from->x, from->y, to->x, to->y,
                                                                  (s16)info->arcHeight, (u16)duration);
    sprite_set_visible(gSpriteHandler, gTossBoys->ball, TRUE);
    gTossBoys->ballRotationSpeed = D_089e86bc[info->from][info->to] * 4;

    if (info->drumNote >= 0) {
        play_drumtech_note(info->drumNote, 0x100, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f4ec.s"
#else
// Cue - Spawn
void toss_boys_cue_spawn(struct Cue *cue, struct TossBoysCue *info, u32 param) {
    info->boy = param & 0xf;
    info->action = param >> 4;
    info->arcHeight = gTossBoys->ballArcHeight;
    info->travelTicks = gTossBoys->ballTravelTicks;
    info->from = gTossBoys->ballSource;
    info->to = gTossBoys->ballTarget;
    info->relaunch = FALSE;
    info->drumNote = gTossBoys->nextDrumNote;
    gTossBoys->nextDrumNote = 0xffff;

    // After a dropped ball, the next pass in mid-air relaunches it from the
    // dispenser instead (see the miss handler).
    if (gTossBoys->popRecovered && (info->travelTicks != 0) && (info->travelTicks != 0x18)) {
        gameplay_set_cue_input_buttons(cue, 0);
        info->relaunch = TRUE;
        gTossBoys->popRecovered = FALSE;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f578.s"
#else
// Cue - Update
u32 toss_boys_cue_update(struct Cue *cue, struct TossBoysCue *info, u32 runningTime, u32 duration) {
    return (runningTime > duration + ticks_to_frames(0xc));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f598.s"
#else
// Cue - Despawn
void toss_boys_cue_despawn(struct Cue *cue, struct TossBoysCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f59c.s"
#else
// Cue - Hit/Barely
void func_0803f59c(struct Cue *cue, struct TossBoysCue *info, u32 barely) {
    struct Vector2 *pos;

    if (!gTossBoys->ballActive || (gTossBoys->popTimer != 0) || gTossBoys->popRecovered) {
        gameplay_ignore_this_cue_result();
        return;
    }

    if (barely) {
        gTossBoys->ballWobbling = TRUE;
        gTossBoys->barelyTimer[info->boy] = ticks_to_frames(0x24);
    } else {
        gTossBoys->ballWobbling = FALSE;
    }

    switch (info->action) {
        case TOSS_BOY_ACTION_PASS:
            func_0803f400(info, gameplay_get_last_hit_offset());
            func_0803f9a0(info->boy, 0);
            func_0803ee58(info->boy, ticks_to_frames(8));
            play_sound_w_pitch_volume(toss_boys_ball_bounce_sfx[info->boy], 0x100, 0);
            sprite_set_anim(gSpriteHandler, gTossBoys->ball, toss_boys_get_anim(TOSS_BOYS_ANIM_BALL1), 0, 0, 0, 0);
            gTossBoys->ballSquash = 0x80;
            break;
        case TOSS_BOY_ACTION_SUPER_PASS:
            func_0803fa64(info->boy, TOSS_BOY_ACTION_CATCH);
            func_0803f9a0(info->boy, 0);
            sprite_set_visible(gSpriteHandler, gTossBoys->ball, FALSE);
            func_0803ee58(info->boy, (u32)-1);
            play_sound_w_pitch_volume(toss_boys_ball_bounce_sfx[info->boy], 0x100, 0);
            break;
        case TOSS_BOY_ACTION_CATCH:
            func_0803f9a0(info->boy, 0);
            func_0803fa64(info->boy, TOSS_BOY_ACTION_PASS);
            sprite_set_visible(gSpriteHandler, gTossBoys->ball, FALSE);
            func_0803ee58(info->boy, ticks_to_frames(0x24));
            play_sound_w_pitch_volume(toss_boys_ball_pop_sfx[info->boy], 0x100, 0);
            func_0803e9b0(info->boy + 0x1f);
            pos = &D_089e86a8[info->boy];
            sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_POP_EFFECT), 0, pos->x, pos->y, 0x4200, 1, 0, 3);
            gTossBoys->ballActive = FALSE;
            break;
    }

    gTossBoys->pitchBend = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f7b4.s"
#else
// Cue - Hit
void toss_boys_cue_hit(struct Cue *cue, struct TossBoysCue *info, u32 pressed, u32 released) {
    func_0803f59c(cue, info, FALSE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f7c0.s"
#else
// Cue - Barely
void toss_boys_cue_barely(struct Cue *cue, struct TossBoysCue *info, u32 pressed, u32 released) {
    func_0803f59c(cue, info, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f7cc.s"
#else
// Cue - Miss
void toss_boys_cue_miss(struct Cue *cue, struct TossBoysCue *info) {
    struct Vector2 *pos;

    if (gTossBoys->popTimer != 0) {
        gameplay_ignore_this_cue_result();
        return;
    }

    if (!gTossBoys->ballActive) {
        gameplay_ignore_this_cue_result();
        if (info->relaunch) {
            gameplay_ignore_this_cue_result();
            func_0803f1f4(info->to, (info->travelTicks == 0x30) ? 60 : 100, info->travelTicks, 5);
        }
        return;
    }

    info->arcHeight = D_089e8704[info->from][info->to];
    info->arcHeight = (info->arcHeight * info->travelTicks) / 24;

    sprite_set_anim(gSpriteHandler, gTossBoys->boys[info->boy], toss_boys_get_anim(toss_boys_miss_anim_ids[info->boy]), 0, 1, 0x7f, 0);
    gTossBoys->animTimer[info->boy] = ticks_to_frames(0xc);
    gTossBoys->boyAction[info->boy] = TOSS_BOY_ACTION_PASS;
    func_0803ee58(info->boy, ticks_to_frames(0xc));
    play_sound_w_pitch_volume(toss_boys_ball_miss_sfx[info->boy], 0x100, 0);

    sprite_set_anim(gSpriteHandler, gTossBoys->ball, toss_boys_get_anim(TOSS_BOYS_ANIM_BALL1), 0, 0, 0, 0);
    sprite_set_visible(gSpriteHandler, gTossBoys->ball, FALSE);
    gTossBoys->ballSquash = 0;

    pos = &D_089e86a8[info->boy];
    sprite_create(gSpriteHandler, toss_boys_get_anim(TOSS_BOYS_ANIM_POP_EFFECT), 0, pos->x, (s16)(pos->y + 4), 0x4200, 1, 0, 3);

    gTossBoys->popTimer = ticks_to_frames(0x24);
    gTossBoys->pitchBend = 0xc00;
    gTossBoys->ballActive = FALSE;
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803f9a0.s"
#else
// Play a Boy's Hit (or Barely) Animation
void func_0803f9a0(u32 boy, s32 startCel) {
    u32 action = gTossBoys->boyAction[boy];
    s8 (*table)[3];

    if (!gTossBoys->ballActive || (gTossBoys->popTimer != 0) || gTossBoys->popRecovered) {
        action = 0;
    }

    table = gTossBoys->barelyTimer[boy] ? toss_boys_barely_anim_ids : toss_boys_hit_anim_ids;
    sprite_set_anim(gSpriteHandler, gTossBoys->boys[boy], toss_boys_get_anim(table[action][boy]), (s8)startCel, 1, 0x7f, 0);
    gTossBoys->animTimer[boy] = ticks_to_frames(0xc);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fa64.s"
#else
// Set a Boy's Next Action (and ready pose)
void func_0803fa64(u32 boy, u32 action) {
    s8 anim;

    gTossBoys->boyAction[boy] = action;
    if (!gTossBoys->ballActive || (gTossBoys->popTimer != 0) || gTossBoys->popRecovered) {
        action = 0;
    }

    anim = toss_boys_ready_anim_ids[action][boy];
    if (anim >= 0) {
        sprite_set_anim(gSpriteHandler, gTossBoys->boys[boy], toss_boys_get_anim(anim), toss_boys_ready_anim_playback[action][boy], 1, 0x7f, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fb00.s"
#else
// Engine Event 0x05 (Set Boy Action: boy | action << 4)
void func_0803fb00(u32 arg) {
    func_0803fa64(arg & 0xf, (arg >> 4) & 0xf);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fb14.s"
#else
// Update Ball Spin (without wobble)
void func_0803fb14(void) {
    gTossBoys->ballRotation += gTossBoys->ballRotationSpeed;
    gTossBoys->ballSquash = (gTossBoys->ballSquash * 0xdc) >> 8;
    set_affine_scale_rotation(gTossBoys->ballAffine, (s16)(gTossBoys->ballSquash + 0x100), (s16)gTossBoys->ballRotation);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fb68.s"
#else
// Input Event
void toss_boys_input_event(u32 pressed, u32 released) {
    if (pressed & A_BUTTON) {
        func_0803f9a0(0, 1);
        func_0803ee58(0, ticks_to_frames(0xc));
    }
    if (pressed & B_BUTTON) {
        func_0803f9a0(1, 1);
        func_0803ee58(1, ticks_to_frames(0xc));
    }
    if (pressed & (DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN)) {
        func_0803f9a0(2, 1);
        func_0803ee58(2, ticks_to_frames(0xc));
    }
    play_sound(&s_f_toss_swing_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fbd8.s"
#else
// Common Event 0 (Beat Animation)
void toss_boys_common_beat_animation(void) {
    u32 i, action;
    s8 anim;

    for (i = 0; i < 3; i++) {
        if (gTossBoys->animTimer[i] != 0) {
            continue;
        }

        action = gTossBoys->boyAction[i];
        if (!gTossBoys->ballActive || (gTossBoys->popTimer != 0) || gTossBoys->popRecovered) {
            action = 0;
        }

        anim = toss_boys_beat_anim_ids[action][i];
        if (anim >= 0) {
            sprite_set_anim(gSpriteHandler, gTossBoys->boys[i], toss_boys_get_anim(anim), toss_boys_beat_anim_playback[action][i], 1, 0x7f, 0);
        }
    }
    func_0803e8b4();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fc8c.s"
#else
// Common Event 1 (Display Text, Unimplemented)
void toss_boys_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/toss_boys/asm_0803fc90.s"
#else
// Common Event 2 (Init. Tutorial, Unimplemented)
void toss_boys_common_init_tutorial(void) {
}
#endif
