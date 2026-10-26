#include "engines/metronome.h"

#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/scenes/gameplay.h"
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gMetronome ((struct MetronomeEngineData *)gCurrentEngineData)


/* METRONOME */


// Graphics Init. 3
void metronome_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}

// Graphics Init. 2
void metronome_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), metronome_gfx_table, 0x2000);
    run_func_after_task(task, metronome_init_gfx3, 0);
}

// Graphics Init. 1
void metronome_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), metronome_buffered_textures);
    run_func_after_task(task, metronome_init_gfx2, 0);
}

#ifndef PLATFORM_PC
#include "asm/engines/metronome/asm_08035488.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08035488] Game Engine Start
void metronome_engine_start(u32 version) {
    struct Animation *text;
    u32 i;

    gMetronome->version = version;
    metronome_init_gfx1();
    scene_show_obj_layer();

    gMetronome->pendulum = create_affine_sprite(anim_metronome_pendulum, 0, 0x78, 0x90, 0x4800,
                                                0x100, 0, 1, 0, 0, 0);
    gMetronome->unk_8 = 0x140;
    gMetronome->unk_a = 0;

    gMetronome->unk_e = sprite_create(gSpriteHandler, anim_metronome_bird_marker, 0, 0x78, 0x20, 0x480a, 1, 0x7f, 0);
    gMetronome->unk_10 = sprite_create(gSpriteHandler, anim_metronome_timing_meter, 5, 0x78, 0x20, 0x480a, 0, 0, 0);
    gMetronome->unk_12 = sprite_create(gSpriteHandler, anim_metronome_bird, 0, 0x78, 0x20, 0x480a, 1, 0x7f, 0);
    gMetronome->unk_14 = sprite_create(gSpriteHandler, anim_metronome_score_counter, 0, 0xd8, 0x14, 0x479d, 0, 0, 0x8000);
    for (i = 0; i < 3; i++) {
        gMetronome->unk_16[i] = sprite_create(gSpriteHandler, anim_metronome_score_num, 0, 0xd8, 0x14, 0x479c, 0, 0x7f, 0x8000);
    }

    gMetronome->faces[0] = sprite_create(gSpriteHandler, anim_metronome_face_l, 0, 1000, 1000, 0x4864, 0, 0, 0);
    gMetronome->faces[1] = sprite_create(gSpriteHandler, anim_metronome_face_r, 0, 1000, 1000, 0x4864, 0, 0, 0);
    for (i = 0; i < 2; i++) {
        gMetronome->faceStates[i] = 0;
    }

    dma3_fill(0, (void *)(VRAMBase + 0x16000), 0x1800, 0x20, 0x200);

    text = text_printer_get_unformatted_line_anim(get_current_mem_id(), 0, 0x18, 0, D_0805a694, 1, 0, 0x100);
    gMetronome->unk_22 = sprite_create(gSpriteHandler, text, 0, 0x78, 0x40, 0x480a, 0, 0, 0x8000);
    sprite_set_base_palette(gSpriteHandler, gMetronome->unk_22, 4);

    text = text_printer_get_unformatted_line_anim(get_current_mem_id(), 0, 0x1a, 0, D_0805a6c0, 1, 2, 0x100);
    gMetronome->unk_24 = sprite_create(gSpriteHandler, text, 0, 0x78, 0x54, 0x480a, 0, 0, 0x8000);
    sprite_set_base_palette(gSpriteHandler, gMetronome->unk_24, 4);

    text = text_printer_get_unformatted_line_anim(get_current_mem_id(), 0, 0x1c, 0, D_0805a6c8, 1, 0, 0x100);
    gMetronome->unk_26 = sprite_create(gSpriteHandler, text, 0, 0x78, 0x40, 0x480a, 0, 0, 0x8000);
    sprite_set_base_palette(gSpriteHandler, gMetronome->unk_26, 4);

    gMetronome->score = 0;
    gMetronome->unk_2a = 0;
    gMetronome->unk_2c = 0x20;
    gMetronome->unk_2e = 0;
    gMetronome->unk_2f = 0;
    func_080359e8();

    gameplay_set_input_buttons(A_BUTTON, 0);
    set_next_scene(&scene_results_ver_score);
}
#endif

void func_08035780(u32 arg0) {
    u32 unk, unk2;
    
    if (arg0 == 0) {
        unk = 0;
        unk2 = 0x400;
    } else {
        unk = 0x400;
        unk2 = 0x800;
    }

    scene_start_integer_interp(1, ticks_to_frames(0x18), &gMetronome->unk_a, unk, unk2);
    gMetronome->unk_c = arg0;
}

#ifndef PLATFORM_PC
#include "asm/engines/metronome/asm_080357c4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080357c4] Engine Event 01 (Beat)
//
// Resets the bird marker, counts the countdown down while it is shown, and
// sends back whichever face (alternating with unk_c) was put out.
void func_080357c4(u32 sound) {
    struct MetronomeUnknownMovementData *move;
    s16 face;
    u32 i;

    sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_e, 0);
    if (sound) {
        play_sound(&s_metro_count1_seqData);
    }

    if ((gMetronome->unk_2c != 0) && (gMetronome->unk_2a != 0)) {
        gMetronome->unk_2c--;
        func_080359e8();
        if (gMetronome->unk_2c == 0) {
            play_sound(&s_metro_tin_seqData);
        } else if (gMetronome->unk_2c <= 3) {
            play_sound(&s_metro_count2_seqData);
        }
    }

    i = (gMetronome->unk_c + 1) & 1;
    face = gMetronome->faces[i];

    switch (gMetronome->faceStates[i]) {
        case 1:
            move = &D_089e5890[i];
            break;
        case 2:
            move = &D_089e58a0[i];
            break;
        default:
            return;
    }

    scene_move_sprite_sine_vel(face, 1, move->initX, move->initY, ticks_to_frames(0xc));
    gMetronome->faceStates[i] = 0;
}
#endif

void func_080358b0(void) {
    if (gMetronome->score != 0) {
        beatscript_enable_loops();
        gMetronome->score = 0;
    } else {
        beatscript_disable_loops();
    }
}

void func_080358d8(void) {
    if (gMetronome->unk_2c != 0) {
        beatscript_enable_loops();
    } else {
        beatscript_disable_loops();
    }
}

void func_080358f8(void) {
}

#ifndef PLATFORM_PC
#include "asm/engines/metronome/asm_080358fc.s"
#endif

void func_080359e8(void) {
    u32 digits;
    u32 i;
    s32 digitX;
    u32 score;

    score = gMetronome->unk_2c;
    digits = 1;
    
    if (9 < score) {
        digits = 2;
    }
    if (99 < score) {
        digits++;
    }

    for (i = 0; i < 3; i++) {
        sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_16[i], 0x7f);
    }

    digitX = (digits - 1) * 5 + 216;

    for (i = 0; i < digits; i++) {
        sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_16[i], score % 10);
        score = score / 10;
        sprite_set_x(gSpriteHandler, gMetronome->unk_16[i], digitX); 

        digitX -= 10;
    }
}

#ifndef PLATFORM_PC
#include "asm/engines/metronome/asm_08035ab0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08035ab0] Game Engine Update
//
// Swings the pendulum; L and R flip two debug toggles, R showing the timing
// meter.
void metronome_engine_update(void) {
    s32 swing;

    swing = gMetronome->unk_8 * gSineTable[((s16)gMetronome->unk_a + 0x200) & 0x7ff];
    affine_sprite_set_rotation(gMetronome->pendulum, (swing << 8) >> 16);

    if (D_03004afc & LEFT_SHOULDER_BUTTON) {
        gMetronome->unk_2e ^= 1;
    }
    if (D_03004afc & RIGHT_SHOULDER_BUTTON) {
        gMetronome->unk_2f ^= 1;
    }

    sprite_set_visible(gSpriteHandler, gMetronome->unk_10, gMetronome->unk_2f);
}
#endif

void metronome_engine_stop(void) {  
}

void metronome_cue_spawn(struct Cue *cue, struct MetronomeCue *info) {
}

u32 metronome_cue_update(struct Cue *cue, struct MetronomeCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x30)) {
        return TRUE;
    } else {
        return FALSE;    
    }
}

void metronome_cue_despawn(struct Cue *cue, struct MetronomeCue *info) {
}

void metronome_cue_hit(struct Cue *cue, struct MetronomeCue *info, u32 pressed, u32 released) {
    u32 unk = gameplay_get_last_hit_offset();
    u32 unk2;
    
    sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_12, 0);
    unk2 = clamp_int32(unk + 5, 0, 10);
    sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_10, unk2);

    play_sound(&s_metro_hit_seqData);
    play_sound(&s_metro_hato_seqData);
}

void metronome_cue_barely(struct Cue *cue, struct MetronomeCue *info, u32 pressed, u32 released) {
    u32 unk = gameplay_get_last_hit_offset();
    u32 unk2 = clamp_int32(unk + 5, 0, 10);

    sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_10, unk2);
}

void metronome_cue_miss(struct Cue *cue, struct MetronomeCue *info) {
    gMetronome->score++;
}

void metronome_input_event(u32 pressed, u32 released) {
    gMetronome->score++;
    sprite_set_anim_cel(gSpriteHandler, gMetronome->unk_10, 5); 
}

void metronome_common_beat_animation(void) {
}

void metronome_common_display_text(s32 text) {
    u32 i;
    
    sprite_set_visible(gSpriteHandler, gMetronome->unk_22, FALSE);
    sprite_set_visible(gSpriteHandler, gMetronome->unk_24, FALSE);
    sprite_set_visible(gSpriteHandler, gMetronome->unk_26, FALSE);

    gMetronome->unk_2a = 0;

    switch (text) {
        case 0:
            sprite_set_visible(gSpriteHandler, gMetronome->unk_22, TRUE);
            break;
        case 1:
            sprite_set_visible(gSpriteHandler, gMetronome->unk_24, TRUE);
            gMetronome->unk_2a = 1;
            sprite_set_visible(gSpriteHandler, gMetronome->unk_14, TRUE);
    
            for (i = 0; i < 3; i++) {
                sprite_set_visible(gSpriteHandler, gMetronome->unk_16[i], TRUE);
            }
            break;
        case 2:
            sprite_set_visible(gSpriteHandler, gMetronome->unk_26, TRUE);
            break;
    }
}

void metronome_common_init_tutorial(struct Scene *skipDest) {
    if (skipDest != NULL) {
        gameplay_enable_tutorial(TRUE);
        gameplay_set_skip_destination(skipDest);
    } else {
        gameplay_enable_tutorial(FALSE);
    }
}
