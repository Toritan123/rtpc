#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

#include "engines/showtime.h"

#include "src/code_08001360.h"
#include "src/bitmap_font.h"
#include "src/code_08007468.h"
#include "src/code_0800b778.h"
#include "src/scenes/gameplay.h"
#include "src/lib_0804ca80.h"
#include "src/audio.h"
#include "src/task_pool.h"

// For readability.
#define gShowtime ((struct ShowtimeEngineData *)gCurrentEngineData)


/* SHOWTIME */


void showtime_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}


void showtime_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), showtime_gfx_tables[gShowtime->version], 0x2000);
    run_func_after_task(task, showtime_init_gfx3, 0);
}


void showtime_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), showtime_buffered_textures);
    run_func_after_task(task, showtime_init_gfx2, 0);
}


void showtime_engine_start(u32 version) {
    struct PrintedTextAnim *textAnim;

    gShowtime->version = version;
    showtime_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 29, BG_PRIORITY_LOW);
    scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, BG_PRIORITY_HIGHEST);
    func_0802d96c();
    gShowtime->objFont = scene_create_obj_font_printer(0x340, 2);
    textAnim = bmp_font_obj_print_c(gShowtime->objFont, D_0805a3cc, 0, 0);
    gShowtime->textSprite = sprite_create(gSpriteHandler, textAnim->frames, 0, 120, 56, 0, 0, 0, 0);
    gameplay_set_input_buttons(A_BUTTON, 0);
    func_0802c23c();    
    func_0802d104();
    func_0802c40c();
    func_0802d394();
    func_0802da84();
    gShowtime->inputCooldown = 0;
    gShowtime->splashX[0] = 0;
    gShowtime->splashX[1] = 0;
    gShowtime->crowd = 0;
}


void showtime_engine_event_stub() {
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802bd44.s"
#else
// Game Engine Update
void showtime_engine_update(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (gShowtime->splashX[i] == 0) {
            continue;
        }
        if (i == 0) {
            sprite_create(gSpriteHandler, anim_showtime_splash_ball, 0, gShowtime->splashX[i] - 4, 156, 0, 1, 0, 3);
        } else {
            sprite_create(gSpriteHandler, anim_showtime_splash_penguin, 0, gShowtime->splashX[i] + 8, 136, 0, 1, 0, 3);
        }
        gShowtime->splashX[i] = 0;
    }

    func_0802c334();
    func_0802d43c();
    func_0802c5c8();
    func_0802d250();
    func_0802d9fc();
    func_0802db08();

    if (gShowtime->inputCooldown > 0) {
        gShowtime->inputCooldown--;
    }
}
#endif


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802be10.s"
#else
// Engine Event 0 (Set Crowd)
void func_0802be10(u32 crowd) {
    gShowtime->crowd = crowd;
}
#endif


void showtime_engine_stop() {
    D_03004b10.BLDMOD = BLDMOD_BLEND_MODE(BLEND_MODE_OFF);
    D_03004b10.DISPCNT &= ~DISPCNT_ENABLE_WINDOW0;
}


void showtime_cue_spawn_gray(struct Cue *cue, struct ShowtimeCue *info, u32 unused) {
    info->unk4 = func_0802ce70(0);
    func_0802d38c();
}


u32 showtime_cue_update_gray(struct Cue *cue, struct ShowtimeCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x78)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


void showtime_cue_despawn_gray(struct Cue *cue, struct ShowtimeCue *info) {
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802be78.s"
#else
// Cue - Spawn (Black)
void showtime_cue_spawn_black(struct Cue *cue, struct ShowtimeCue *info, u32 unused) {
    info->unk4 = func_0802ce70(1);
    func_0802d38c();
}
#endif


u32 showtime_cue_update_black(struct Cue *cue, struct ShowtimeCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x90)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


void showtime_cue_despawn_black(struct Cue *cue, struct ShowtimeCue *info) {
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802beb0.s"
#else
// Cue - Spawn (White, Fast)
void showtime_cue_spawn_white_fast(struct Cue *cue, struct ShowtimeCue *info, u32 unused) {
    info->unk4 = func_0802ce70(2);
    func_0802d38c();
}
#endif


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802bec8.s"
#else
// Cue - Update (White, Fast)
u32 showtime_cue_update_white_fast(struct Cue *cue, struct ShowtimeCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x78));
}
#endif


void showtime_cue_despawn_white_fast(struct Cue *cue, struct ShowtimeCue *info) {
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802bee8.s"
#else
// Cue - Spawn (White, Fast, Swing)
void showtime_cue_spawn_white_fast_swing(struct Cue *cue, struct ShowtimeCue *info, u32 unused) {
    info->unk4 = func_0802ce70(3);
    func_0802d38c();
}
#endif


u32 showtime_cue_update_white_fast_swing(struct Cue *cue, struct ShowtimeCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x78)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


void showtime_cue_despawn_white_fast_swing(struct Cue *cue, struct ShowtimeCue *info) {
}


void showtime_cue_spawn_white(struct Cue *cue, struct ShowtimeCue *info, u32 unused) {
    info->unk4 = func_0802ce70(4);
    func_0802d38c();
}


u32 showtime_cue_update_white(struct Cue *cue, struct ShowtimeCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x78)){
        return TRUE;
    } else {
        return FALSE;
    }
}


void showtime_cue_despawn_white(struct Cue *cue, struct ShowtimeCue *info) {
}


void showtime_cue_hit(struct Cue *cue, struct ShowtimeCue *info, u32 pressed, u32 released) {
    func_0802cf8c(info->unk4);
    func_0802d81c(info->unk4);
    func_0802d2bc();
    gShowtime->inputCooldown = ticks_to_frames(0x14);
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802bf88.s"
#else
// Cue - Barely
void showtime_cue_barely(struct Cue *cue, struct ShowtimeCue *info, u32 pressed, u32 released) {
    u16 speed;

    func_0802cfa4(info->unk4);
    func_0802d8bc(info->unk4);
    gShowtime->monkeyState = 2;
    sprite_set_anim(gSpriteHandler, gShowtime->monkey, anim_showtime_monkey_sad_swing, 0, 1, 0x7f, 4);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->monkey, 0);
    speed = (get_beatscript_tempo() << 8) / 140;
    sprite_set_anim_speed(gSpriteHandler, gShowtime->monkey, speed);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->launcher, 0);
    speed = (get_beatscript_tempo() << 8) / 140;
    sprite_set_anim_speed(gSpriteHandler, gShowtime->launcher, speed);
    gShowtime->monkeyTimer = ticks_to_frames(0x24);
    gShowtime->inputCooldown = ticks_to_frames(0x14);
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c078.s"
#else
// Cue - Miss
void showtime_cue_miss(struct Cue *cue, struct ShowtimeCue *info) {
    beatscript_enable_loops();
}
#endif


void showtime_input_event(u32 pressed, u32 released) {
    if (gShowtime->inputCooldown == 0) {
        func_0802d918(-1);
        func_0802d2bc();
        gShowtime->inputCooldown = ticks_to_frames(0x1E);
        play_sound(&s_block_hit_seqData);
    }
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c0c8.s"
#else
// Common Event 0 (Beat Animation)
void showtime_common_beat_animation(void) {
    if ((gShowtime->monkeyTimer == 0) && (gShowtime->monkeyState == 0)) {
        sprite_set_anim(gSpriteHandler, gShowtime->monkey, anim_showtime_monkey_beat2, 0, 1, 0x7f, 0);
        sprite_set_anim_cel(gSpriteHandler, gShowtime->monkey, 0);
        sprite_set_anim_speed(gSpriteHandler, gShowtime->monkey, (u16)((get_beatscript_tempo() << 8) / 140));
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c150.s"
#else
// Common Event 1 (Display Text)
void showtime_common_display_text(const char *text) {
    struct PrintedTextAnim *anim;

    if (text == NULL) {
        sprite_set_visible(gSpriteHandler, gShowtime->textSprite, FALSE);
        return;
    }

    anim = bmp_font_obj_print_c(gShowtime->objFont, text, 1, 12);
    delete_bmp_font_obj_text_anim(gShowtime->objFont, gShowtime->textSprite);
    sprite_set_anim(gSpriteHandler, gShowtime->textSprite, anim->frames, 0, 0, 0, 0);
    sprite_set_visible(gSpriteHandler, gShowtime->textSprite, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c1cc.s"
#else
// Common Event 2 (Init. Tutorial)
void showtime_common_init_tutorial(struct Scene *skipDestination) {
    if (skipDestination != NULL) {
        gameplay_enable_tutorial(TRUE);
        gameplay_set_skip_destination(skipDestination);
    } else {
        gameplay_enable_tutorial(FALSE);
    }
}
#endif


void func_0802c1f0(u32 unused, s16 sprite, u32 arg2) {
    switch (gShowtime->blocks[arg2].state) {
        case 0:
            break;
        case 1:
            gShowtime->blocks[arg2].state = 0;
        sprite_set_anim_cel(gSpriteHandler, sprite, 3);
        sprite_set_anim_speed(gSpriteHandler, sprite, 0);
    }
}


void func_0802c23c() {
    s32 i;

    for (i = 0; i < SHOWTIME_SUB_AMOUNT; i++) {
        gShowtime->blocks[i].state = 0;

        if (gShowtime->version != SHOWTIME_VER_REMIX_3) {
            gShowtime->blocks[i].sprite = sprite_create(gSpriteHandler, anim_showtime_block, 0, 64, 64, 0x4800, 1, 0, 4);
        } else {
            gShowtime->blocks[i].sprite = sprite_create(gSpriteHandler, anim_showtime_block_pink, 0, 64, 64, 0x4800, 1, 0, 4);
        }

        gShowtime->blocks[i].time = 0;

        sprite_set_callback(gSpriteHandler, gShowtime->blocks[i].sprite, &func_0802c1f0, i);
        sprite_set_anim_cel(gSpriteHandler, gShowtime->blocks[i].sprite, 3);
        sprite_set_anim_speed(gSpriteHandler, gShowtime->blocks[i].sprite, 0);
    }

    sprite_set_x_y(gSpriteHandler, gShowtime->blocks[0].sprite, 200, 128);
    sprite_set_x_y(gSpriteHandler, gShowtime->blocks[1].sprite, 184, 144);
}


void func_0802c334() {
    s32 i;

    for (i = 0; i < SHOWTIME_SUB_AMOUNT; i++) {
        if (gShowtime->blocks[i].state == 0) {
            continue;
        }

        if (gShowtime->blocks[i].state == 1) {
            gShowtime->blocks[i].time++;
        }
    }
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c36c.s"
#else
// Bounce a Block
void func_0802c36c(u32 block) {
    gShowtime->blocks[block].state = 1;
    gShowtime->blocks[block].time = 0;
    sprite_set_anim_cel(gSpriteHandler, gShowtime->blocks[block].sprite, 0);
    sprite_set_anim_speed(gSpriteHandler, gShowtime->blocks[block].sprite, (u16)((get_beatscript_tempo() << 8) / 140));
}
#endif


u32 func_0802c3d0(u32 arg) {
    switch (gShowtime->blocks[arg].time / 4) {
        case 1:
            return 3;
        case 0:
            return 0;
        case 2:
            return 2;
        default:
            return 0;
    }
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c40c.s"
#else
// Create Penguins
void func_0802c40c(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        gShowtime->penguins[i].state = 0;
        gShowtime->penguins[i].sprite = sprite_create(gSpriteHandler, anim_showtime_penguin_beat, 0, 64, 64, 0, 1, 0, 0);
        gShowtime->penguins[i].time = 0;
        gShowtime->penguins[i].landX = 1;
        gShowtime->penguins[i].type = 0;
        sprite_set_visible(gSpriteHandler, gShowtime->penguins[i].sprite, FALSE);
        sprite_set_x_y(gSpriteHandler, gShowtime->penguins[i].sprite, 256, 80);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c4b0.s"
#else
// (Unused) Play Stop Sound
void func_0802c4b0(void) {
    play_sound(&s_rat_stop_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c4c0.s"
#else
// Play Penguin Voice 1
void func_0802c4c0(u32 type) {
    switch ((s32)type) {
        case 0:
            play_sound(&s_esa_pengin1_1_seqData);
            break;
        case 1:
            play_sound(&s_esa_pengin2_1_seqData);
            break;
        case 2:
        case 3:
        case 4:
            play_sound(&s_esa_pengin3_1_seqData);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c4f4.s"
#else
// Play Penguin Voice 2
void func_0802c4f4(u32 type) {
    switch ((s32)type) {
        case 0:
            play_sound(&s_esa_pengin1_2_seqData);
            break;
        case 1:
            play_sound(&s_esa_pengin2_2_seqData);
            break;
        case 2:
        case 3:
        case 4:
            play_sound(&s_esa_pengin3_2_seqData);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c528.s"
#else
// Play Penguin Voice 3
void func_0802c528(u32 type) {
    switch ((s32)type) {
        case 0:
            play_sound(&s_esa_pengin1_3_seqData);
            break;
        case 1:
            play_sound(&s_esa_pengin2_3_seqData);
            break;
        case 2:
        case 3:
        case 4:
            play_sound(&s_esa_pengin3_3_seqData);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c55c.s"
#else
// Parabolic Hop from (x0, y0) to (x1, y1)
void func_0802c55c(s32 x0, s32 y0, s32 x1, s32 y1, s32 height, s32 time, s32 duration, s32 *outX, s32 *outY) {
    s32 x, mid, half, y;

    x = math_lerp(x0 * 256, x1 * 256, time, duration);
    mid = (((x0 + x1) * 256) >> 1) - x;
    half = ((x1 - x0) * 256) >> 1;
    mid = (mid * mid) >> 8;
    half = (half * half) >> 8;
    y = ((height * mid) / half) - height;
    y += math_lerp(y0, y1, time, duration);
    *outX = x >> 8;
    *outY = y;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802c5c8.s"
#else
// Update Penguins
void func_0802c5c8(void) {
    struct ShowtimePenguin *penguin;
    s32 x = 0, y = 0;
    s32 duration;
    s32 i, j;

    for (i = 0; i < 8; i++) {
        penguin = &gShowtime->penguins[i];
        if (penguin->state > 11) {
            goto store;
        }
        duration = D_089e3b14[penguin->state][penguin->type] << 8;

        switch (penguin->state) {
            case 0:
                x = 272;
                y = 188;
                break;

            case 1: // Launch onto the first block
                penguin->time += func_0800c398();
                func_0802c55c(256, 80, 200, 104, 32, penguin->time, duration, &x, &y);
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);
                if (penguin->time < duration) {
                    break;
                }
                func_0802c36c(0);
                penguin->time = 0;
                if (penguin->type == 4) {
                    penguin->state = 2;
                    func_0802c4f4(penguin->type);
                    sprite_set_anim_speed(gSpriteHandler, penguin->sprite, 0x100);
                } else {
                    penguin->state = 3;
                    func_0802c4c0(penguin->type);
                    sprite_set_anim_speed(gSpriteHandler, penguin->sprite, 0);
                }
                break;

            case 2: // Wait on the first block
                penguin->time += func_0800c398();
                x = 200;
                y = func_0802c3d0(0) + 104;
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);
                if (penguin->time < duration) {
                    break;
                }
                penguin->state = 3;
                penguin->time = 0;
                func_0802c4c0(penguin->type);
                sprite_set_anim_speed(gSpriteHandler, penguin->sprite, 0);
                break;

            case 3: // Hop to the second block
                penguin->time += func_0800c398();
                func_0802c55c(200, 104, 184, 120, 32, penguin->time, duration, &x, &y);
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);
                if (penguin->time < duration) {
                    break;
                }
                penguin->state = 4;
                penguin->time = 0;
                func_0802c36c(1);
                func_0802c4f4(penguin->type);
                sprite_set_anim_speed(gSpriteHandler, penguin->sprite, 0x100);
                break;

            case 4: // On the second block
            case 5:
                penguin->time += func_0800c398();
                x = 184;
                y = func_0802c3d0(1) + 120;
                sprite_set_x_y(gSpriteHandler, penguin->sprite, 184, y);
                if (penguin->state == 4) {
                    if (penguin->time >= (duration >> 1)) {
                        penguin->state = 5;
                        sprite_set_anim(gSpriteHandler, penguin->sprite, showtime_penguin_jump_prepare_anim[penguin->type], 0, 1, 0x7f, 0);
                        sprite_set_anim_cel(gSpriteHandler, penguin->sprite, 1);
                    }
                } else if (penguin->time >= duration) {
                    penguin->state = 6;
                    penguin->time = 0;
                    sprite_set_anim(gSpriteHandler, penguin->sprite, showtime_penguin_jump_anim[penguin->type], 0, 1, 0x7f, 0);
                    func_0802c528(penguin->type);
                }
                break;

            case 6: // Jump towards the ball (or the water)
                penguin->time += func_0800c398();
                if (penguin->landX == 0) {
                    func_0802c55c(184, 120, 96, 48, 32, penguin->time, duration, &x, &y);
                } else {
                    func_0802c55c(184, 120, 128, 48, 32, penguin->time, duration, &x, &y);
                }
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);

                if ((y > 120) && (penguin->landX == 0)) {
                    penguin->state = 8;
                    penguin->time = 0;
                    penguin->landX = x;
                    sprite_set_anim(gSpriteHandler, penguin->sprite, showtime_penguin_slide_anim[penguin->type], 0, 1, 0, 0);
                    y = 120;
                    sprite_set_x_y(gSpriteHandler, penguin->sprite, x, 120);
                    if (gShowtime->crowd) {
                        if (penguin->barely) {
                            play_sound(&s_warai_solo_seqData);
                        } else if ((s32)penguin->type <= 1) {
                            play_sound(&s_hakushu_solo_seqData);
                        } else {
                            play_sound(&s_kansei_solo_seqData);
                        }
                    }
                }

                if ((y > 172) && (penguin->landX == 1)) {
                    penguin->state = 0;
                    penguin->time = 0;
                    penguin->landX = 1;
                    sprite_set_visible(gSpriteHandler, penguin->sprite, FALSE);
                    sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_beat, 0, 1, 0, 0);
                    gShowtime->splashX[1] = x;
                    if (gShowtime->crowd) {
                        play_sound(&s_warai_solo_seqData);
                    }
                    play_sound(&s_f_esa_splash_penguin_seqData);
                    for (j = 7; j >= 0; j--) {
                        func_0802dc54(96, y);
                    }
                }
                break;

            case 9: // Floating away on the balloon
            case 10:
            case 11:
                penguin->time += func_0800c398();
                x = penguin->startX - ((penguin->time * 16) / duration);
                y = penguin->startY + ((penguin->time * 8) / duration);
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);
                if (penguin->time >= duration) {
                    if (penguin->state == 9) {
                        penguin->state = 10;
                        sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_catch_para, 0, 1, 0, 0);
                    } else if (penguin->state == 10) {
                        penguin->state = 11;
                        sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_float_para, 0, 1, 0, 0);
                    }
                }
                if (x <= -16) {
                    penguin->state = 0;
                    penguin->time = 0;
                    penguin->landX = 1;
                    sprite_set_visible(gSpriteHandler, penguin->sprite, FALSE);
                    sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_beat, 0, 1, 0, 0);
                }
                break;

            case 7: // Drop into the water
                penguin->time += func_0800c398();
                x = 96;
                y = ((((penguin->time >> 8) * 124) * (penguin->time >> 8)) / 24) / 24 + 48;
                sprite_set_x_y(gSpriteHandler, penguin->sprite, 96, y);
                if (penguin->time >= duration) {
                    penguin->state = 0;
                    penguin->time = 0;
                    penguin->landX = 1;
                    sprite_set_visible(gSpriteHandler, penguin->sprite, FALSE);
                    sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_beat, 0, 1, 0, 0);
                    sprite_create(gSpriteHandler, anim_showtime_splash_penguin, 0, 96, 148, 0, 1, 0, 3);
                }
                break;

            case 8: // Slide off with the ball
                penguin->time += func_0800c398();
                x = penguin->landX - ((penguin->time * 64) / duration);
                y = penguin->y;
                sprite_set_x_y(gSpriteHandler, penguin->sprite, x, y);
                if (x <= -16) {
                    penguin->state = 0;
                    penguin->time = 0;
                    penguin->landX = 1;
                    sprite_set_visible(gSpriteHandler, penguin->sprite, FALSE);
                    sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_beat, 0, 1, 0, 0);
                }
                break;
        }

    store:
        penguin->x = x;
        penguin->y = y;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802ce70.s"
#else
// Spawn a Penguin (returns its index, or -1)
s32 func_0802ce70(s32 type) {
    struct ShowtimePenguin *penguin;
    s32 i;

    if (type > 4) {
        type = 0;
    }

    for (i = 0; i < 8; i++) {
        penguin = &gShowtime->penguins[i];
        if (penguin->state != 0) {
            continue;
        }

        penguin->state = 1;
        sprite_set_anim(gSpriteHandler, penguin->sprite, showtime_penguin_beat_anim[type], 0, 1, 0, 0);
        sprite_set_visible(gSpriteHandler, penguin->sprite, TRUE);
        penguin->type = type;
        sprite_set_base_palette(gSpriteHandler, penguin->sprite, 0);
        switch (type) {
            case 2:
                sprite_set_base_palette(gSpriteHandler, penguin->sprite, 4);
                break;
            case 4:
                func_0802c4c0(4);
                // fallthrough
            case 3:
                sprite_set_base_palette(gSpriteHandler, penguin->sprite, 4);
                break;
        }
        sprite_set_anim_speed(gSpriteHandler, penguin->sprite, 0);
        penguin->barely = FALSE;
        return i;
    }

    return -1;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802cf8c.s"
#else
// Penguin Will Land
void func_0802cf8c(s32 penguin) {
    gShowtime->penguins[penguin].landX = 0;
}
#endif


void func_0802cfa4(u32 arg) {
    gShowtime->penguins[arg].landX = 0;
    gShowtime->penguins[arg].barely = 1;
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802cfc8.s"
#else
// Penguin Will Fall In
void func_0802cfc8(s32 penguin) {
    gShowtime->penguins[penguin].landX = 1;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802cfe0.s"
#else
// Penguin Catches the Ball Mid-Jump (parachute)
void func_0802cfe0(s32 index) {
    struct ShowtimePenguin *penguin = &gShowtime->penguins[index];

    if (penguin->state == 6) {
        penguin->state = 9;
        penguin->time = 0;
        penguin->startX = func_0802d068(index);
        penguin->startY = func_0802d080(index);
        sprite_set_anim(gSpriteHandler, penguin->sprite, anim_showtime_penguin_catch, 0, 1, 0, 0);
        sprite_set_anim_cel(gSpriteHandler, penguin->sprite, 1);
    }
}
#endif


u32 func_0802d068(u32 arg) {
    return gShowtime->penguins[arg].x;
}


u32 func_0802d080(u32 arg) {
    if (gShowtime->penguins[arg].state == 8) {
        return gShowtime->penguins[arg].y + 13;
    } else {
        return gShowtime->penguins[arg].y;
    }
}


void func_0802d0b8() {
    if (gShowtime->monkeyState != 0) {
        if (gShowtime->monkeyState == 2) {
            gShowtime->monkeyState = 0;
            }
    }
}


void func_0802d0dc(u32 arg0, s16 sprite) {
    sprite_set_anim_cel(gSpriteHandler, sprite, 8);
    sprite_set_anim_speed(gSpriteHandler, sprite, 0);
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d104.s"
#else
// Create Monkey, Launcher and Ball
void func_0802d104(void) {
    gShowtime->monkeyState = 0;
    gShowtime->monkey = sprite_create(gSpriteHandler, anim_showtime_monkey_beat2, 0, 23, 136, 0x4800, 1, 0, 4);
    gShowtime->unk16C = 0;
    sprite_set_callback(gSpriteHandler, gShowtime->monkey, func_0802d0b8, 0);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->monkey, 3);
    sprite_set_anim_speed(gSpriteHandler, gShowtime->monkey, 0);

    gShowtime->launcher = sprite_create(gSpriteHandler, anim_showtime_launcher, 0, 23, 136, 0x4800, 1, 0, 4);
    sprite_set_callback(gSpriteHandler, gShowtime->launcher, func_0802d0dc, 0);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->launcher, 8);
    sprite_set_anim_speed(gSpriteHandler, gShowtime->launcher, 0);

    gShowtime->ball = sprite_create(gSpriteHandler, anim_showtime_ball, 0, 64, 64, 0x4800, 1, 0, 0);
    sprite_set_x_y(gSpriteHandler, gShowtime->ball, 64, 120);
    gShowtime->monkeyTimer = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d250.s"
#else
// Update Monkey's Ball
void func_0802d250(void) {
    switch (gShowtime->monkeyState) {
        case 0:
            sprite_set_visible(gSpriteHandler, gShowtime->ball, TRUE);
            break;
        case 2:
            sprite_set_visible(gSpriteHandler, gShowtime->ball, FALSE);
            break;
    }

    if (gShowtime->monkeyTimer != 0) {
        gShowtime->monkeyTimer--;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d2bc.s"
#else
// Monkey Swings
void func_0802d2bc(void) {
    u16 speed;

    gShowtime->monkeyState = 2;
    sprite_set_anim(gSpriteHandler, gShowtime->monkey, anim_showtime_monkey_swing, 0, 1, 0x7f, 4);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->monkey, 0);
    speed = (get_beatscript_tempo() << 8) / 140;
    sprite_set_anim_speed(gSpriteHandler, gShowtime->monkey, speed);
    sprite_set_anim_cel(gSpriteHandler, gShowtime->launcher, 0);
    speed = (get_beatscript_tempo() << 8) / 140;
    sprite_set_anim_speed(gSpriteHandler, gShowtime->launcher, speed);
    gShowtime->monkeyTimer = ticks_to_frames(0xc);
}
#endif


// stack pointer moment
void func_0802d38c(void) {
    u8 unused_temp[0xC]; // has to be a data type 0xC bytes long
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d394.s"
#else
// Create Balls
void func_0802d394(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        gShowtime->balls[i].state = 0;
        gShowtime->balls[i].sprite = sprite_create(gSpriteHandler, anim_showtime_ball, 0, 64, 64, 0, 1, 0, 0);
        gShowtime->balls[i].time = 0;
        gShowtime->balls[i].target = -1;
        sprite_set_visible(gSpriteHandler, gShowtime->balls[i].sprite, FALSE);
        sprite_set_x_y(gSpriteHandler, gShowtime->balls[i].sprite, 64, 116);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d43c.s"
#else
// Update Balls
void func_0802d43c(void) {
    struct ShowtimeBall *ball;
    s32 i, t, x, y;

    for (i = 0; i < 8; i++) {
        ball = &gShowtime->balls[i];

        switch (ball->state) {
            case 1: // Stray throw: wind up, then fly
                ball->time += func_0800c398();
                if ((u32)ball->time >= func_0800c398() * 2) {
                    ball->state = 4;
                    sprite_set_visible(gSpriteHandler, ball->sprite, TRUE);
                }
                break;

            case 2: // Thrown at a penguin
                ball->time += func_0800c398();
                t = ball->time >> 8;
                x = ((t * 16) / 24) + 64;
                y = ((((72 - x) * 32) * (72 - x)) / -8) / -8 + 84;
                y += -(t * 72) / 24;
                sprite_set_x_y(gSpriteHandler, ball->sprite, x, y);
                if (ball->time <= 0x17ff) {
                    break;
                }
                if ((agb_random(2) != 0) || !gShowtime->crowd || (gShowtime->penguins[ball->target].type == 1)) {
                    ball->state = 3;
                    ball->time = 0;
                    play_sound(&s_esa_catch_seqData);
                } else {
                    func_0802cfe0(ball->target);
                    ball->state = 0;
                    ball->time = 0;
                    sprite_set_visible(gSpriteHandler, ball->sprite, FALSE);
                    sprite_set_x_y(gSpriteHandler, ball->sprite, 64, 116);
                    play_sound(&s_esa_catch_seqData);
                }
                switch ((s32)gShowtime->penguins[ball->target].type) {
                    case 0:
                        play_sound(&s_esa_pengin1_1_seqData);
                        break;
                    case 1:
                        play_sound(&s_esa_pengin2_1_seqData);
                        break;
                    case 2:
                    case 3:
                    case 4:
                        play_sound(&s_esa_pengin3_1_seqData);
                        break;
                }
                break;

            case 3: // Carried by the penguin
                x = (s32)func_0802d068(ball->target) - 18;
                y = (s32)func_0802d080(ball->target) - 4;
                sprite_set_x_y(gSpriteHandler, ball->sprite, x, y);
                if (x < -8) {
                    ball->state = 0;
                    ball->time = 0;
                    sprite_set_visible(gSpriteHandler, ball->sprite, FALSE);
                    sprite_set_x_y(gSpriteHandler, ball->sprite, 64, 116);
                }
                break;

            case 4: // Into the water
                ball->time += func_0800c398();
                t = ball->time >> 8;
                x = ((t * 16) / 24) + 64;
                y = ((((72 - x) * 32) * (72 - x)) / -8) / -8 + 84;
                y += -(t * 72) / 24;
                sprite_set_x_y(gSpriteHandler, ball->sprite, x, y);
                if (y > 199) {
                    ball->state = 0;
                    ball->time = 0;
                    sprite_set_visible(gSpriteHandler, ball->sprite, FALSE);
                    sprite_set_x_y(gSpriteHandler, ball->sprite, 64, 116);
                    gShowtime->splashX[0] = x;
                    play_sound(&s_f_esa_splash_ball_seqData);
                }
                break;
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d81c.s"
#else
// Throw a Ball at a Penguin
void func_0802d81c(s32 penguin) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (gShowtime->balls[i].state == 1) {
            gShowtime->balls[i].state = 2;
            gShowtime->balls[i].target = penguin;
            gShowtime->balls[i].time = 0;
            sprite_set_visible(gSpriteHandler, gShowtime->balls[i].sprite, TRUE);
            return;
        }
    }
    for (i = 0; i < 8; i++) {
        if (gShowtime->balls[i].state == 0) {
            gShowtime->balls[i].state = 2;
            gShowtime->balls[i].target = penguin;
            gShowtime->balls[i].time = 0;
            sprite_set_visible(gSpriteHandler, gShowtime->balls[i].sprite, TRUE);
            return;
        }
    }
}
#endif


void func_0802d8bc(u32 arg) {
    s32 i;

    for (i = 0; i < SHOWTIME_SUB2_AMOUNT; i++) {
        if (gShowtime->balls[i].state == 0) {
            gShowtime->balls[i].state = 4;
            gShowtime->balls[i].target = arg;
            gShowtime->balls[i].time = 0;
            sprite_set_visible(gSpriteHandler, gShowtime->balls[i].sprite, TRUE);
            return;
        }
    }
}


#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d918.s"
#else
// Stray Throw
void func_0802d918(s32 target) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (gShowtime->balls[i].state == 0) {
            gShowtime->balls[i].state = 1;
            gShowtime->balls[i].target = target;
            gShowtime->balls[i].time = 0;
            return;
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d96c.s"
#else
// Init. Water Window and Blend
void func_0802d96c(void) {
    gShowtime->blendLevel = 8;
    D_03004b10.BLDMOD = 0x3a44;
    D_03004b10.COLEV = (gShowtime->blendLevel << 8) | (0x10 - gShowtime->blendLevel);
    D_03004b10.WIN0H = 0xf0;
    D_03004b10.WIN1H = 0xf0;
    D_03004b10.WIN0V = 0x80a0;
    D_03004b10.WIN1V = 0xa0;
    D_03004b10.WININ = 0x3337;
    D_03004b10.WINOUT = 0x2033;
    D_03004b10.DISPCNT |= 0x2000;
    gShowtime->waveIndex = 0;
    gShowtime->waveTimer = 0;
    gShowtime->waveScroll = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802d9fc.s"
#else
// Update Water Waves
void func_0802d9fc(void) {
    s32 height;

    if (++gShowtime->waveTimer > 9) {
        gShowtime->waveIndex = (gShowtime->waveIndex + 1) % 10;
        gShowtime->waveTimer = 0;
        gShowtime->waveScroll += 8;
    }

    height = (gShowtime->waveIndex > 5) ? (10 - gShowtime->waveIndex) : gShowtime->waveIndex;
    gShowtime->waveScroll %= 256;
    scene_set_bg_layer_pos(BG_LAYER_2, (s16)gShowtime->waveScroll, (s16)((1 - height) * 32));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802da84.s"
#else
// Create Bubbles
void func_0802da84(void) {
    s32 i;

    for (i = 0; i < 16; i++) {
        gShowtime->bubbles[i].active = FALSE;
        gShowtime->bubbles[i].sprite = sprite_create(gSpriteHandler, anim_showtime_bubble, 0, 64, 64, 0, 1, 0, 0);
        gShowtime->bubbles[i].time = 0;
        sprite_set_visible(gSpriteHandler, gShowtime->bubbles[i].sprite, FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802db08.s"
#else
// Update Bubbles
void func_0802db08(void) {
    struct ShowtimeBubble *bubble;
    s32 i;

    for (i = 0; i < 16; i++) {
        bubble = &gShowtime->bubbles[i];
        if (bubble->active != 1) {
            continue;
        }

        bubble->time++;
        bubble->x += bubble->velX;
        bubble->y += bubble->velY;
        sprite_set_x_y(gSpriteHandler, bubble->sprite, (s16)(bubble->x >> 8), (s16)(bubble->y >> 8));

        if (bubble->velX > 0) {
            bubble->velX -= 2;
        }
        if (bubble->velX < 0) {
            bubble->velX += 2;
        }
        if (bubble->velY > -0x80) {
            bubble->velY -= 4;
        }
        if (bubble->velY < -0x80) {
            bubble->velY += 2;
        }

        if ((bubble->time > 120) || (bubble->y <= 0x8fff)) {
            bubble->active = FALSE;
            bubble->time = 0;
            sprite_set_visible(gSpriteHandler, bubble->sprite, FALSE);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/showtime/asm_0802dc54.s"
#else
// Spawn a Bubble
void func_0802dc54(s32 x, s32 y) {
    struct ShowtimeBubble *bubble;
    s32 i;

    for (i = 0; i < 16; i++) {
        bubble = &gShowtime->bubbles[i];
        if (bubble->active) {
            continue;
        }
        bubble->active = TRUE;
        bubble->velX = 0x60 - agb_random(0x120);
        bubble->velY = agb_random(0x100) - 0x80;
        bubble->x = x * 256;
        bubble->y = y * 256;
        sprite_set_visible(gSpriteHandler, bubble->sprite, TRUE);
        bubble->time = 0;
        return;
    }
}
#endif
