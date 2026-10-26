#include "engines/mechanical_horse.h"

#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/scenes/gameplay.h"
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gMechanicalHorse ((struct MechanicalHorseEngineData *)gCurrentEngineData)


/* MECHANICAL HORSE */


void func_08040c2c() {
    gMechanicalHorse->unk2fe = 0;
    gMechanicalHorse->unk306 = -1;
    gMechanicalHorse->unk300 = 0;
}

void func_08040c58() {
    u32* temp;
    switch (gMechanicalHorse->unk2fe) {
        case 0:
        case 1:
        case 2:
            gMechanicalHorse->unk30c += gMechanicalHorse->unk2d0 / 2;
            break;
        case 3:
            gMechanicalHorse->unk30c += INT_TO_FIXED(13);
            break;
        case 4:
            gMechanicalHorse->unk30c = 0;
            break;
        case 5:
            gMechanicalHorse->unk30c = 0;
            break;
    }
    scene_set_bg_layer_pos(0, FIXED_TO_INT(gMechanicalHorse->unk30c), 0);
}


void func_08040cfc() {
    gMechanicalHorse->unk300 = 0;
}

void func_08040d10() {
    u16 a = get_current_mem_id();
    s32 task = palette_fade_in(a, gMechanicalHorse->unk304, 2, gMechanicalHorse->unk302, &mechanical_horse_backgrounds[gMechanicalHorse->unk2ff].palette[0][0], D_03004b10.bgPalette[0]);
    run_func_after_task(task, func_08040cfc, 0);
    gMechanicalHorse->unk2fe = gMechanicalHorse->unk2ff;
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08040d90.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040d90] Load the queued background, then fade it in (func_08040d10)
void func_08040d90(void) {
    s32 task;

    task = func_08002ee0(get_current_mem_id(), mechanical_horse_backgrounds[gMechanicalHorse->unk2ff].gfxTable, 0x2000);
    run_func_after_task(task, func_08040d10, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08040dd8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040dd8] Start the queued background change: fade the current one out
void func_08040dd8(void) {
    s32 task;

    gMechanicalHorse->unk300 = 1;
    gMechanicalHorse->unk2ff = gMechanicalHorse->unk306;
    gMechanicalHorse->unk302 = gMechanicalHorse->unk308;
    gMechanicalHorse->unk304 = gMechanicalHorse->unk30a;
    gMechanicalHorse->unk306 = -1;

    task = palette_fade_out(get_current_mem_id(), (u8)gMechanicalHorse->unk304, 2,
                            &mechanical_horse_backgrounds[gMechanicalHorse->unk2fe].palette[0][0],
                            gMechanicalHorse->unk302, D_03004b10.bgPalette[0]);
    run_func_after_task(task, func_08040d90, 0);
}
#endif

void func_08040e80() {
    if (gMechanicalHorse->unk300 == 0 && gMechanicalHorse->unk306 >= 0) {
        func_08040dd8();
    }
    func_08040c58();
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08040eb0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040eb0] Queue a background change (cancelled if it is already showing)
void func_08040eb0(s32 bg, u16 color, u16 frames) {
    if (gMechanicalHorse->unk2fe == bg) {
        gMechanicalHorse->unk306 = -1;
    } else {
        gMechanicalHorse->unk306 = bg;
        gMechanicalHorse->unk308 = color;
        gMechanicalHorse->unk30a = frames;
    }
}
#endif

void mechanical_horse_init_gfx3() {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}

void mechanical_horse_init_gfx2() {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), gfx_table_mechanical_horse, 0x2000);
    run_func_after_task(task, mechanical_horse_init_gfx3, 0);
}

void mechanical_horse_init_gfx1() {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), mechanical_horse_buffered_textures);
    run_func_after_task(task, mechanical_horse_init_gfx2, 0);
}

void mechanical_horse_engine_start(u32 version) {
    u8 i;
    gMechanicalHorse->version = version;
    mechanical_horse_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(0, 1, 0, 0, 2, 28, 3);
    scene_set_bg_layer_display(1, 1, 0, 0, 0, 29, 2);
    scene_set_bg_layer_display(2, 1, 0, 0, 0, 30, 1);
    scene_set_bg_layer_display(3, 0, 0, 0, 0, 31, 0);
    gMechanicalHorse->unk2e4 = 0x1000;
    gMechanicalHorse->unk2e8 = 0;
    D_03004b10.BLDMOD = 0x142;
    D_03004b10.COLEV = 0x1000;
    gMechanicalHorse->text_font = scene_create_obj_font_printer(0x380, 1);
    gMechanicalHorse->text_sprite =
        sprite_create(gSpriteHandler, bmp_font_obj_print_c(gMechanicalHorse->text_font, D_0805a9fc, 1, 0xf)->frames, 0, 0x80, 0x90, 0, 0, 0, 0);
    for (i = 0; i < 2; i++) {
        gMechanicalHorse->horse[i].unk2 = 0;
        gMechanicalHorse->horse[i].cel = 0;
        gMechanicalHorse->horse[i].unk4 = 0;
        if (i == 0) {
            gMechanicalHorse->horse[i].pos_x = INT_TO_FIXED(88);
            gMechanicalHorse->horse[i].pos_y = INT_TO_FIXED(100);
        } else {
            gMechanicalHorse->horse[i].pos_x = INT_TO_FIXED(184);
            gMechanicalHorse->horse[i].pos_y = INT_TO_FIXED(100);
        }
        gMechanicalHorse->horse[i].unk10 = gMechanicalHorse->horse[i].unk14 = 0;
        if (i == 0) {
            gMechanicalHorse->horse[i].sprite =
                sprite_create(gSpriteHandler, anim_horse_still, 0, FIXED_TO_INT(gMechanicalHorse->horse[i].pos_x), FIXED_TO_INT(gMechanicalHorse->horse[i].pos_y), 0x8004, 0, 0, 0);
        } else {
            gMechanicalHorse->horse[i].sprite =
                sprite_create(gSpriteHandler, anim_horse_still, 0, FIXED_TO_INT(gMechanicalHorse->horse[i].pos_x), FIXED_TO_INT(gMechanicalHorse->horse[i].pos_y), 0x8007, 0, 0, 0);
            sprite_set_base_palette(gSpriteHandler, gMechanicalHorse->horse[i].sprite, -1);
        }
        gMechanicalHorse->jockey[i].unk2 = 0;
        gMechanicalHorse->jockey[i].cel = 0;
        if (i == 0) {
            gMechanicalHorse->jockey[i].sprite = 
                sprite_create(gSpriteHandler, anim_horse_walk_jockey, 0, FIXED_TO_INT(gMechanicalHorse->horse[i].pos_x), FIXED_TO_INT(gMechanicalHorse->horse[i].pos_y), 0x8003, 0, 0, 0);
        } else {
            gMechanicalHorse->jockey[i].sprite = 
                sprite_create(gSpriteHandler, anim_horse_walk_jockey, 0, FIXED_TO_INT(gMechanicalHorse->horse[i].pos_x), FIXED_TO_INT(gMechanicalHorse->horse[i].pos_y), 0x8006, 0, 0, 0);
            sprite_set_base_palette(gSpriteHandler, gMechanicalHorse->jockey[i].sprite, -1);
        }
    }
    for (i = 0; i < 20; i++) {
        gMechanicalHorse->unk3c[i].sprite = sprite_create(gSpriteHandler, anim_horse_text_pak, 0, -0x40, -0x40, 0x8002, 0, 0, 0);
        gMechanicalHorse->unk3c[i].unk2 = 0;
        sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, FALSE);
    }
    for (i = 0; i < 4; i++) {
        gMechanicalHorse->unk26c[i].pos_x = INT_TO_FIXED(-64);
        gMechanicalHorse->unk26c[i].pos_y = INT_TO_FIXED(144);
        gMechanicalHorse->unk26c[i].unk10 = gMechanicalHorse->unk26c[i].unk14 = 0;
        gMechanicalHorse->unk26c[i].sprite =
            sprite_create(gSpriteHandler, anim_horse_text_pak, 0, FIXED_TO_INT(gMechanicalHorse->unk26c[i].pos_x), FIXED_TO_INT(gMechanicalHorse->unk26c[i].pos_y), 1, 0, 0, 0);
        gMechanicalHorse->unk26c[i].unk2 = 0;
        gMechanicalHorse->unk26c[i].unk3 = 0;
        gMechanicalHorse->unk26c[i].unk4 = 0;
        sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk26c[i].sprite, FALSE);
    }
    gMechanicalHorse->tachometer_hand =
        create_affine_sprite(anim_horse_tachometer_hand, 0, 87, 144, 0x800, 0x100, 0, 0, 0, 0x8000, 0);
    gMechanicalHorse->speedometer_hand =
        create_affine_sprite(anim_horse_speedometer_hand, 0, 143, 144, 0x800, 0x100, 0, 0, 0, 0x8000, 0);
    gMechanicalHorse->high_speed_light_sprite =
        sprite_create(gSpriteHandler, anim_horse_high_speed_light, 0, 94, 130, 0x800, 1, 0, 0x8000);
    func_08040c2c();
    gMechanicalHorse->unk2cc = 0;
    gMechanicalHorse->unk2d0 = 0;
    gMechanicalHorse->unk2d4 = 0;
    gMechanicalHorse->unk2d8 = 0;
    gMechanicalHorse->unk2e9 = 0;
    gMechanicalHorse->unk2ea = 0;
    gMechanicalHorse->unk2eb = 1;
    gMechanicalHorse->music_volume = 0x40;
    gMechanicalHorse->unk2ee = 0;
    gMechanicalHorse->unk2f0 = 0x100;
    gameplay_set_input_buttons(DPAD_ANY | A_BUTTON | B_BUTTON, 0);
}

void func_08041444(int arg0) {
    u32 temp;
    u8 temp4;
    struct SoundPlayer* temp1;
    if (arg0 == 0) {
        temp1 = play_sound(mechanical_horse_player_horse_sfx[gMechanicalHorse->unk2cc * 4 + gMechanicalHorse->horse[0].cel]);
    } else {
        u24_8 temp2;
        s24_8 temp3;
        
        temp1 = play_sound(mechanical_horse_teacher_horse_sfx[gMechanicalHorse->unk2cc * 4 + gMechanicalHorse->horse[arg0].cel]);
        
        if (gMechanicalHorse->horse[0].unk2 == 1) {
            temp2 = INT_TO_FIXED(0.25);
        } else {
            temp2 = INT_TO_FIXED(1);
        }
        
        temp3 = FIXED_TO_INT(gMechanicalHorse->horse[1].pos_x) - 128;
        temp3 = ABS(temp3);
        if (temp3 >= 128) {
            temp2 = clamp_int32(128 - temp3 + temp2, INT_TO_FIXED(0.25), INT_TO_FIXED(1));
        }
        
        set_soundplayer_volume(temp1, temp2);
    }

    sprite_set_anim_cel(gSpriteHandler, gMechanicalHorse->horse[arg0].sprite, gMechanicalHorse->horse[arg0].cel);

    temp4 = gMechanicalHorse->horse[arg0].cel += 1;

    if (temp4 > D_0805aa00[gMechanicalHorse->unk2cc]) {
        gMechanicalHorse->horse[arg0].cel = 0;
    }

    sprite_set_anim_cel(gSpriteHandler, gMechanicalHorse->jockey[arg0].sprite, gMechanicalHorse->jockey[arg0].cel);


    if (((gMechanicalHorse->jockey[arg0].cel += 1) & 0xff) > D_0805aa10[gMechanicalHorse->unk2cc]) {
        gMechanicalHorse->jockey[arg0].cel = 0;
    }

    if (arg0 == 1) {
        gMechanicalHorse->horse[1].unk4 = 0;
        gMechanicalHorse->horse[1].unk10 += D_0805aa20[gMechanicalHorse->unk2cc];
    }

}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_080415c0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080415c0] Engine Event 0x01 (Set the Lesson)
//
// Gives the teacher's horse and jockey the lesson's animations and lays out
// its four step labels.
void func_080415c0(u32 lesson) {
    struct MechanicalHorseSub4 *label;
    u8 i;

    gMechanicalHorse->unk2cc = lesson;
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->horse[1].sprite, mechanical_horse_anim[lesson], 0, 0, 0, 0);
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[1].sprite, mechanical_horse_jockey_anim[lesson], 0, 0, 0, 0);

    for (i = 0; i < 4; i++) {
        label = &gMechanicalHorse->unk26c[i];
        label->pos_x = D_0805aab0[lesson][i] << 8;
        sprite_set_x_y(gSpriteHandler, label->sprite, (s16)(label->pos_x >> 8), (s16)(label->pos_y >> 8));
        sprite_set_anim(gSpriteHandler, label->sprite, mechanical_horse_lesson_text_anim[lesson][i], 1, 0, 0, 0);
    }
}
#endif

// prints specified text?
void func_080416cc(const char* string) {
    delete_bmp_font_obj_text_anim(gMechanicalHorse->text_font, gMechanicalHorse->text_sprite);
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->text_sprite, (struct Animation*)bmp_font_obj_print_c(gMechanicalHorse->text_font, string, 1, 0xc), 0, 1, 0, 0);
}

void func_08041730(u8 unk) {
    gMechanicalHorse->unk2e8 = unk;
}

void func_08041744(u32 arg0) {
    u8 i;
    gMechanicalHorse->unk2e9 = arg0;
    for (i = 0; i < 4; i++) {
        sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk26c[i].sprite, arg0);
    }
    if (arg0 == 0 && gMechanicalHorse->unk2cc == 3) {
        gMechanicalHorse->unk2e9 = 1;
    }
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_080417ac.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080417ac] Engine Event 0x05 (End the Lesson if the Teacher Is Through)
//
// While the teacher's horse is still left of x = 0x58 the lesson loops on.
// Otherwise the player's horse is stopped (unless this is lesson 3), the
// horse whinnies, the lesson music fades and the loop is let go.
void func_080417ac(void) {
    struct SongHeader *music;

    if (gMechanicalHorse->horse[1].pos_x > 0x5800) {
        beatscript_enable_loops();
        return;
    }

    if (gMechanicalHorse->unk2cc != 3) {
        gMechanicalHorse->horse[0].unk2 = 0;
        gMechanicalHorse->horse[0].cel = 0;
        gMechanicalHorse->horse[0].unk10 = 0;
        sprite_set_anim(gSpriteHandler, gMechanicalHorse->horse[0].sprite, anim_horse_still, 0, 0, 0, 0);
        sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[0].sprite, anim_horse_walk_jockey, 0, 0, 0, 0);
    }

    gMechanicalHorse->unk2ea = 0;
    play_sound(&s_uma_hihin_seqData);

    music = mechanical_horse_lesson_bgm[gMechanicalHorse->unk2cc];
    if (gMechanicalHorse->unk2cc <= 2) {
        fade_out_sound(music, ticks_to_frames(0x60));
    } else {
        fade_out_sound(music, ticks_to_frames(0xc0));
    }

    func_08041744(0);
    beatscript_disable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_0804188c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0804188c] Settle the jockey after a jump
void func_0804188c(void) {
    struct MechanicalHorseJockey *jockey = &gMechanicalHorse->jockey[0];
    s8 cel;

    if (jockey->unk2 == 1) {
        cel = sprite_get_anim_cel(gSpriteHandler, jockey->sprite);
        if (cel <= 2) return;
    } else if (jockey->unk2 == 2) {
        cel = sprite_get_anim_cel(gSpriteHandler, jockey->sprite);
        if (cel <= 1) return;
    } else {
        return;
    }

    gMechanicalHorse->jockey[0].unk2 = 0;
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[0].sprite,
                    mechanical_horse_jockey_anim[gMechanicalHorse->unk2cc], 0, 0, 0, 0);
}
#endif

u8 func_08041940(void) {
    u8 i;
    for (i = 0; i < 20; i++) {
        if (gMechanicalHorse->unk3c[i].unk2 == 0) {
            return i;
        }  
    }
    return 0;
}

void func_08041970(void) {
    u8 i;
    boolean isPlayer;
    u32 temp2;
    for (i = 0; i < 20; i++) {
        switch (gMechanicalHorse->unk3c[i].unk2) {
            case 2:
            case 3:
                isPlayer = FALSE;
                if (gMechanicalHorse->unk3c[i].unk2 == 2) {
                    isPlayer = TRUE;
                }

                if (isPlayer) {
                    gMechanicalHorse->unk3c[i].pos_x = gMechanicalHorse->horse[0].pos_x;
                    gMechanicalHorse->unk3c[i].pos_z = INT_TO_FIXED(128.0078125);
                } else {
                    gMechanicalHorse->unk3c[i].pos_x = gMechanicalHorse->horse[1].pos_x;
                    gMechanicalHorse->unk3c[i].pos_z = INT_TO_FIXED(128.01953125);
                }
                gMechanicalHorse->unk3c[i].pos_y = INT_TO_FIXED(96);
                gMechanicalHorse->unk3c[i].unk14[0] = D_0805aa40[gMechanicalHorse->unk2cc][0];
                gMechanicalHorse->unk3c[i].unk14[1] = D_0805aa40[gMechanicalHorse->unk2cc][1];
                gMechanicalHorse->unk3c[i].unk4 = 0;
                if (gMechanicalHorse->unk2cc == 0) {
                    if (gMechanicalHorse->unk3c[i].unk3 == 0 || gMechanicalHorse->unk3c[i].unk3 == 2) {
                        sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_pak, 0, 0, 0, 0);
                    } else {
                        sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_ka, 0, 0, 0, 0);
                    }
                } else {
                    if (gMechanicalHorse->unk2cc == 1) {                    
                        sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_tot, 0, 0, 0, 0);
                    } else if (gMechanicalHorse->unk2cc == 2) {
                        if (gMechanicalHorse->unk3c[i].unk3 == 0) {
                            sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_pa, 0, 0, 0, 0);
                        } else if (gMechanicalHorse->unk3c[i].unk3 == 1) {
                            sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_ka, 0, 0, 0, 0); 
                        } else {
                            sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_rap, 0, 0, 0, 0);
                        }
                    } else {
                        if (gMechanicalHorse->unk3c[i].unk3 <= 2) {
                             sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_do, 0, 0, 0, 0);
                        } else {
                             sprite_set_anim(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, anim_horse_text_dod, 0, 0, 0, 0);
                        }
                    }
                }
                sprite_set_x_y_z(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, FIXED_TO_INT(gMechanicalHorse->unk3c[i].pos_x), FIXED_TO_INT(gMechanicalHorse->unk3c[i].pos_y), gMechanicalHorse->unk3c[i].pos_z);
                sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, 1);
                gMechanicalHorse->unk3c[i].unk2 = 4;
                break;
            case 4:
                gMechanicalHorse->unk3c[i].pos_x += gMechanicalHorse->unk3c[i].unk14[0];
                gMechanicalHorse->unk3c[i].pos_y += gMechanicalHorse->unk3c[i].unk14[1];
                gMechanicalHorse->unk3c[i].unk14[1] += INT_TO_FIXED(0.125); 
                sprite_set_x_y(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, FIXED_TO_INT(gMechanicalHorse->unk3c[i].pos_x), FIXED_TO_INT(gMechanicalHorse->unk3c[i].pos_y));
                if ((gMechanicalHorse->unk3c[i].pos_x < INT_TO_FIXED(-16)) || (gMechanicalHorse->unk3c[i].pos_y > INT_TO_FIXED(112))) {
                    gMechanicalHorse->unk3c[i].unk2 = 0;
                    sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk3c[i].sprite, 0);
                }
                break;
            case 1:
                if (gMechanicalHorse->unk3c[i].unk4-- > 0) {
                    break;
                }
                gMechanicalHorse->unk3c[i].unk2 = 3;
                break;
            case 0:
                break;
        }
    }
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08041c98.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08041c98] Player's horse starts
void func_08041c98(void) {
    struct MechanicalHorseHorse *horse = &gMechanicalHorse->horse[0];
    u8 hoof;

    horse->unk2 = 1;
    horse->unk4 = 0;
    horse->cel = 0;
    sprite_set_anim(gSpriteHandler, horse->sprite, mechanical_horse_anim[gMechanicalHorse->unk2cc], 0, 0, 0, 0);
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[0].sprite,
                    mechanical_horse_jockey_anim[gMechanicalHorse->unk2cc], 0, 0, 0, 0);

    hoof = func_08041940();
    gMechanicalHorse->unk3c[hoof].unk3 = 0;
    gMechanicalHorse->unk3c[hoof].unk2 = 2;
    func_08041444(0);

    horse->unk10 += D_0805aa20[gMechanicalHorse->unk2cc];
    gMechanicalHorse->unk2eb = 1;

    // Bounce the first step label, and arm the second.
    gMechanicalHorse->unk26c[0].pos_y = 0x9000;
    gMechanicalHorse->unk26c[0].unk14 = (u32)-0x200;
    gMechanicalHorse->unk26c[0].unk2 = 1;
    sprite_set_y(gSpriteHandler, gMechanicalHorse->unk26c[0].sprite, (s16)(gMechanicalHorse->unk26c[0].pos_y >> 8));
    gMechanicalHorse->unk26c[1].unk2 = 2;
    gMechanicalHorse->unk26c[1].unk3 = ticks_to_frames(D_0805aa60[gMechanicalHorse->unk2cc][horse->cel]);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08041ddc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08041ddc] Player's horse keeps pace
void func_08041ddc(void) {
    struct MechanicalHorseHorse *horse = &gMechanicalHorse->horse[0];
    u8 hoof = func_08041940();
    u8 step, next;

    gMechanicalHorse->unk3c[hoof].unk3 = horse->cel;
    gMechanicalHorse->unk3c[hoof].unk2 = 2;

    // Bounce the label for this step...
    step = horse->cel;
    gMechanicalHorse->unk26c[step].pos_y = 0x9000;
    gMechanicalHorse->unk26c[step].unk14 = (u32)-0x200;
    gMechanicalHorse->unk26c[step].unk2 = 1;
    sprite_set_y(gSpriteHandler, gMechanicalHorse->unk26c[step].sprite,
                 (s16)(gMechanicalHorse->unk26c[step].pos_y >> 8));

    // ...and arm the next one.
    next = (u8)(horse->cel + 1);
    if (next > D_0805aa00[gMechanicalHorse->unk2cc]) next = 0;
    gMechanicalHorse->unk26c[next].unk2 = 2;
    gMechanicalHorse->unk26c[next].unk3 = ticks_to_frames(D_0805aa60[gMechanicalHorse->unk2cc][next]);

    func_08041444(0);
    horse->unk4 = 0;

    if (gMechanicalHorse->unk2ea == 1) {
        gMechanicalHorse->unk2ea = 2;
    } else {
        gMechanicalHorse->unk2ea = 1;
    }

    horse->unk10 += D_0805aa20[gMechanicalHorse->unk2cc] * 2;
    gMechanicalHorse->unk2eb = 0;

    gMechanicalHorse->music_volume += 0xc;
    if (gMechanicalHorse->music_volume > 0x100) gMechanicalHorse->music_volume = 0x100;

    if (gMechanicalHorse->unk2ee != 0) {
        gMechanicalHorse->unk2f0 = clamp_int32(gMechanicalHorse->unk2f0 + 1, 0x100, 0x800);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08041f80.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08041f80] Player's horse stops (stray input)
void func_08041f80(void) {
    struct MechanicalHorseHorse *horse = &gMechanicalHorse->horse[0];

    horse->unk2 = 0;
    horse->cel = 0;
    sprite_set_anim(gSpriteHandler, horse->sprite, anim_horse_still, 0, 0, 0, 0);
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[0].sprite, anim_horse_walk_jockey, 0, 0, 0, 0);
    gMechanicalHorse->unk2ea = 0;

    // Lose an eighth of the speed, rounded toward zero.
    horse->unk10 = (s32)horse->unk10 - ((s32)horse->unk10 / 8);

    if (gMechanicalHorse->unk2ee != 0) {
        gMechanicalHorse->unk2f0 = (((s32)gMechanicalHorse->unk2f0 - 0x100) * 0xdc >> 8) + 0x100;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08042020.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08042020] Player's horse stops (miss) -- the same as func_08041f80
void func_08042020(void) {
    struct MechanicalHorseHorse *horse = &gMechanicalHorse->horse[0];

    horse->unk2 = 0;
    horse->cel = 0;
    sprite_set_anim(gSpriteHandler, horse->sprite, anim_horse_still, 0, 0, 0, 0);
    sprite_set_anim(gSpriteHandler, gMechanicalHorse->jockey[0].sprite, anim_horse_walk_jockey, 0, 0, 0, 0);
    gMechanicalHorse->unk2ea = 0;

    horse->unk10 = (s32)horse->unk10 - ((s32)horse->unk10 / 8);

    if (gMechanicalHorse->unk2ee != 0) {
        gMechanicalHorse->unk2f0 = (((s32)gMechanicalHorse->unk2f0 - 0x100) * 0xdc >> 8) + 0x100;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_080420c0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080420c0] Update the horses' speed and the teacher's position
void func_080420c0(void) {
    struct MechanicalHorseHorse *player = &gMechanicalHorse->horse[0];
    struct MechanicalHorseHorse *teacher = &gMechanicalHorse->horse[1];
    s32 top = D_0805aa20[gMechanicalHorse->unk2cc + 4];
    s32 speed;

    // The player's speed decays by a 24th a frame, capped at twice the top.
    speed = (s32)player->unk10;
    speed -= speed / 0x18;
    player->unk10 = speed;
    if (speed > top * 2) player->unk10 = top * 2;
    if ((s32)player->unk10 <= 0x1f) player->unk10 = 0;

    // The teacher decays by a 32nd and drifts back by the player's speed.
    speed = (s32)teacher->unk10;
    speed -= speed / 0x20;
    teacher->unk10 = speed;
    teacher->pos_x += speed - ((s32)(gMechanicalHorse->unk2f0 * (s32)player->unk10) >> 8);
    if (speed > top) teacher->unk10 = top;
    if ((s32)teacher->unk10 <= 0x1f) teacher->unk10 = 0;

    if (teacher->pos_x > D_0805aaa0[gMechanicalHorse->unk2cc]) teacher->pos_x = D_0805aaa0[gMechanicalHorse->unk2cc];
    if (teacher->pos_x < -0x6400) teacher->pos_x = -0x6400;

    sprite_set_x(gSpriteHandler, teacher->sprite, (s16)(teacher->pos_x >> 8));
    sprite_set_x(gSpriteHandler, gMechanicalHorse->jockey[1].sprite, (s16)(teacher->pos_x >> 8));

    // Lessons 2 and 3 hold the gallop pose a little before landing.
    teacher->unk4++;
    if ((gMechanicalHorse->unk2cc == 2) && (teacher->cel == 0)
        && (sprite_get_anim_cel(gSpriteHandler, teacher->sprite) == 2)
        && ((s32)teacher->unk4 >= (s32)ticks_to_frames(6))) {
        sprite_set_anim_cel(gSpriteHandler, teacher->sprite, 3);
    }
    if ((gMechanicalHorse->unk2cc == 3) && (teacher->cel == 0)
        && (sprite_get_anim_cel(gSpriteHandler, teacher->sprite) == 3)
        && ((s32)teacher->unk4 >= (s32)ticks_to_frames(6))) {
        sprite_set_anim_cel(gSpriteHandler, teacher->sprite, 4);
    }

    if (!gMechanicalHorse->unk2e9) return;

    if (player->unk2 == 0) {
        // Standing still lets the music fade, down to a quarter.
        if (--gMechanicalHorse->music_volume <= 0x3f) gMechanicalHorse->music_volume = 0x40;
        return;
    }

    player->unk4++;
    if ((gMechanicalHorse->unk2cc == 2) && (player->cel == 0)
        && (sprite_get_anim_cel(gSpriteHandler, player->sprite) == 2)
        && ((s32)player->unk4 >= (s32)ticks_to_frames(6))) {
        sprite_set_anim_cel(gSpriteHandler, player->sprite, 3);
    }
    if ((gMechanicalHorse->unk2cc == 3) && (player->cel == 0)
        && (sprite_get_anim_cel(gSpriteHandler, player->sprite) == 3)
        && ((s32)player->unk4 >= (s32)ticks_to_frames(6))) {
        sprite_set_anim_cel(gSpriteHandler, player->sprite, 4);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_0804231c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0804231c] Bounce the lesson step labels
//
// State 2 counts unk3 down, then pops the label up (state 1), which falls
// back under gravity until it lands at y = 0x90.
void func_0804231c(void) {
    struct MechanicalHorseSub4 *label;
    u8 i;

    for (i = 0; i < 4; i++) {
        label = &gMechanicalHorse->unk26c[i];
        if (label->unk2 == 0) continue;

        if (label->unk2 == 2) {
            if (--label->unk3 != 0) continue;
            label->pos_y = 0x9000;
            label->unk14 = (u32)-0x100;
            label->unk2 = 1;
            label->unk3 = 0;
            sprite_set_y(gSpriteHandler, label->sprite, (s16)(label->pos_y >> 8));
            continue;
        }

        label->pos_y += (s32)label->unk14;
        label->unk14 += 0x40;
        if (label->pos_y > 0x9000) {
            label->pos_y = 0x9000;
            label->unk14 = 0;
            label->unk2 = 0;
        }
        sprite_set_y(gSpriteHandler, label->sprite, (s16)(label->pos_y >> 8));
    }
}
#endif

// https://decomp.me/scratch/58myn
#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08042438.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08042438] Scroll the ground and the trees with the player's speed
void func_08042438(void) {
    s32 product = (s32)gMechanicalHorse->horse[0].unk10 * gMechanicalHorse->unk2f0;

    gMechanicalHorse->unk2d0 = product >> 8;
    // Half of unk2d0, rounded toward zero the way the assembly does it
    // (adding the product's sign bit before an arithmetic shift).
    gMechanicalHorse->unk2d4 += (s32)(gMechanicalHorse->unk2d0 + (s32)((u32)product >> 31)) >> 1;
    gMechanicalHorse->unk2d8 += gMechanicalHorse->unk2d0;
    scene_set_bg_layer_pos(BG_LAYER_1, (s16)((s32)gMechanicalHorse->unk2d4 >> 8), 0);
    scene_set_bg_layer_pos(BG_LAYER_2, (s16)((s32)gMechanicalHorse->unk2d8 >> 8), 0);
}
#endif

void func_0804249c(void) {
    u32 temp = gMechanicalHorse->unk2e4;
    u32 temp1;
    u16 temp2;
    u8 temp3 = gMechanicalHorse->unk2e8;
    temp1 = temp + 0x40;
    if (temp3) {
        temp1 -= 0x80;
    }
    gMechanicalHorse->unk2e4 = temp1;
    gMechanicalHorse->unk2e4 = clamp_int32(gMechanicalHorse->unk2e4, 0, 0x1000);
    temp2 = FIXED_TO_INT(gMechanicalHorse->unk2e4);
    D_03004b10.COLEV = temp2 | INT_TO_FIXED(16 - temp2);
}

void func_080424f0(u16 unk) {
    gMechanicalHorse->unk2ee = unk;
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08042504.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08042504] Engine Event 0x07 (Set the Tempo from the Speed)
void func_08042504(void) {
    s32 tempo;

    if (gMechanicalHorse->unk2ee == 0) return;

    tempo = (((s32)gMechanicalHorse->unk2f0 - 0x100) / 2) + 0x100;
    tempo *= gMechanicalHorse->unk2ee;
    set_beatscript_tempo((u16)((u32)(tempo << 8) >> 16));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08042548.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08042548] Update the dashboard
//
// Above speed 0x140 the speedometer panel (BG3, two needles, the
// high-speed light) replaces the step labels. The needles jitter by a
// random 0-63 at their stop, and the speedometer picks the background.
void func_08042548(void) {
    u32 fast;
    s32 over, limit, angle;
    u32 i;

    if (!gMechanicalHorse->unk2e9) return;

    fast = (gMechanicalHorse->unk2f0 > 0x13f);
    affine_sprite_set_visible(gMechanicalHorse->tachometer_hand, fast);
    affine_sprite_set_visible(gMechanicalHorse->speedometer_hand, fast);
    sprite_set_visible(gSpriteHandler, gMechanicalHorse->high_speed_light_sprite, fast);
    if (fast) {
        scene_show_bg_layer(BG_LAYER_3);
    } else {
        scene_hide_bg_layer(BG_LAYER_3);
    }

    for (i = 0; i < 4; i++) {
        sprite_set_visible(gSpriteHandler, gMechanicalHorse->unk26c[i].sprite, (u16)(fast ^ 1));
    }

    if (!fast) return;

    over = (s32)gMechanicalHorse->unk2f0 - 0x100;

    limit = (u16)agb_random(0x40) + 0x352;
    angle = clamp_int32(((over * 7) * 0xaa) / 0x180 - 0x1fe, -0x2a8, limit);
    affine_sprite_set_rotation(gMechanicalHorse->tachometer_hand, (s16)angle);
    if (angle < 0x1fe) {
        sprite_set_visible(gSpriteHandler, gMechanicalHorse->high_speed_light_sprite, FALSE);
    }

    limit = (u16)agb_random(0x40) + 0x352;
    angle = clamp_int32(((over * 5) * 0xaa) / 0x180 - 0x1fe, -0x2a8, limit);
    affine_sprite_set_rotation(gMechanicalHorse->speedometer_hand, (s16)angle);
    gMechanicalHorse->unk2e8 = (angle >= -0xaa);

    func_08040eb0(clamp_int32((angle + 0xaa) / 0xaa, 0, 5), 0x7fff, 0x40);
}
#endif

void mechanical_horse_engine_update() {
    func_080420c0();
    func_0804188c();
    func_08041970();
    func_0804231c();
    func_08042548();
    func_08042438();
    func_08040e80();
    func_0804249c();
    scene_set_music_volume(gMechanicalHorse->music_volume);
}

void mechanical_horse_engine_stop() {
}

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_08042758.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08042758] Cue - Spawn
void mechanical_horse_cue_spawn(struct Cue *cue, struct MechanicalHorseCue *info, u32 lesson) {
    info->halfway = FALSE;
    info->lesson = lesson;
    info->hoof = func_08041940();
    gMechanicalHorse->unk3c[info->hoof].unk2 = 1;
    gMechanicalHorse->unk3c[info->hoof].unk4 = ticks_to_frames(0xc);
    gMechanicalHorse->unk3c[info->hoof].unk3 = info->lesson;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mechanical_horse/asm_080427b0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080427b0] Cue - Update
u32 mechanical_horse_cue_update(struct Cue *cue, struct MechanicalHorseCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x18)) return TRUE;

    if (!info->halfway && (runningTime >= ticks_to_frames(0xc))) {
        func_08041444(1);
        info->halfway = TRUE;
    }
    return FALSE;
}
#endif

void mechanical_horse_cue_despawn(struct Cue *cue, struct MechanicalHorseCue *data) {
}

void mechanical_horse_cue_hit(struct Cue *cue, struct MechanicalHorseCue *data, u32 pressed, u32 released) {
    if (gMechanicalHorse->unk2e9 == 0) {
        gameplay_ignore_this_cue_result();
    } else if (gMechanicalHorse->horse[0].unk2 == 0) {
        func_08041c98();
    } else {
        func_08041ddc();
    }
}

void mechanical_horse_cue_barely(struct Cue *cue, struct MechanicalHorseCue *data, u32 pressed, u32 released) {
    mechanical_horse_cue_hit(cue, data, pressed, released);
}

void mechanical_horse_cue_miss(struct Cue *cue, struct MechanicalHorseCue *data) {
    gameplay_ignore_this_cue_result();
    func_08042020();
}

void mechanical_horse_input_event(u32 pressed, u32 released) {
    if (gMechanicalHorse->horse[0].unk2 == 0) {
        func_08041c98();
    } else {
        func_08041f80();
    }
}

void mechanical_horse_common_beat_animation() {
}

void mechanical_horse_common_display_text() {
}
