#include "engines/tram_and_pauline.h"

#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/scenes/gameplay.h"
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gTramPauline ((struct TramPaulineEngineData *)gCurrentEngineData)

// Versions 1 and 3 are the circus, the others the casual set.
#define TRAM_PAULINE_IS_CIRCUS() ((gTramPauline->version == 1) || (gTramPauline->version == 3))


/* TRAM & PAULINE */

void tram_pauline_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}

void tram_pauline_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), tram_pauline_gfx_tables[gTramPauline->version], 0x2000);
    run_func_after_task(task, tram_pauline_init_gfx3, 0);
}

void tram_pauline_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), tram_pauline_buffered_textures);
    run_func_after_task(task, tram_pauline_init_gfx2, 0);
}

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_0803fd10.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803fd10] Game Engine Start
void tram_pauline_engine_start(u32 version) {
    struct PrintedTextAnim *text;
    struct Fox *fox;
    struct Animation *anim;
    u8 i;

    gTramPauline->version = version;
    tram_pauline_init_gfx1();
    scene_show_obj_layer();

    if (gTramPauline->version == 1) {
        scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1d, 0x8000);
        scene_set_bg_layer_display(BG_LAYER_3, TRUE, 0, 0, 0, 0x1f, 1);
    }
    if (gTramPauline->version == 2) {
        scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1d, 0x8001);
    }
    if (gTramPauline->version == 3) {
        scene_set_bg_layer_display(BG_LAYER_1, FALSE, 0, 0, 0, 0x1d, 0x8000);
        scene_set_bg_layer_display(BG_LAYER_3, TRUE, 0, 0, 0, 0x1f, 1);
    }

    gTramPauline->font = scene_create_obj_font_printer(0x340, 2);
    text = bmp_font_obj_print_c(gTramPauline->font, D_0805a910, 1, 0xf);
    gTramPauline->textSprite = sprite_create(gSpriteHandler, (struct Animation *)text, 0, 0x50, 0x40, 0, 0, 0, 0);

    for (i = 0; i < 2; i++) {
        fox = &gTramPauline->foxes[i];
        fox->unk_5 = 0;
        fox->unk_F = 0;
        fox->y = 0x98;
        if (i == 0) {
            fox->x = 0x3c;
            fox->unk_14 = 0;
        } else {
            fox->x = 0xb4;
            fox->unk_14 = 1;
        }

        anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_fox_beat : anim_casual_fox_beat;
        fox->sprite = sprite_create(gSpriteHandler, anim, 0, (s16)fox->x, (s16)fox->y, 0x4003, 0, 0, 0);
        if (i == 1) {
            sprite_set_base_palette(gSpriteHandler, gTramPauline->foxes[1].sprite, 3);
        }

        anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_fox_transform_effect : anim_casual_fox_transform_effect;
        gTramPauline->unkSprites[i] = sprite_create(gSpriteHandler, anim, 0, (s16)fox->x, (s16)fox->y, 0x4002, 0, 0, 0);
        sprite_set_visible(gSpriteHandler, gTramPauline->unkSprites[i], FALSE);
    }

    anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_tram_pauline_trampoline1 : anim_casual_tram_pauline_trampoline;
    gTramPauline->trampolineSprite = sprite_create(gSpriteHandler, anim, 0, 0x3c, 0x98, 0x4004, 0, 0, 0);
    anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_tram_pauline_trampoline1 : anim_casual_tram_pauline_trampoline;
    gTramPauline->trampolineSprite2 = sprite_create(gSpriteHandler, anim, 0, 0xb4, 0x98, 0x4004, 0, 0, 0);
    gTramPauline->unk_32 = 0;
    gTramPauline->unk_2e = 0;

    anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_tram_pauline_text_skip : anim_casual_tram_pauline_text_skip;
    gTramPauline->skipTutorialSprite = sprite_create(gSpriteHandler, anim, 0, 0xf0, 0xa0, 1, 0, 0, 0);
    sprite_set_visible(gSpriteHandler, gTramPauline->skipTutorialSprite, FALSE);

    gTramPauline->unk_40 = 0;
    gTramPauline->curtainScroll = 0;
    gameplay_set_input_buttons(0xf1, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_08040064.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040064] Fox lands
void func_08040064(u32 arg) {
    u8 i = arg;
    struct Fox *fox = &gTramPauline->foxes[i];

    fox->unk_5 = 1;
    sprite_set_anim(gSpriteHandler, fox->sprite,
                    tram_pauline_anim_table[gTramPauline->version][1][fox->unk_14], 0, 1, 0x7f, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_080400d0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080400d0] Fox jumps
//
// 0 and 1 jump fox 0 or 1 into state 2, 2 and 3 into state 3.
void func_080400d0(u32 arg) {
    struct Fox *fox;
    u8 i = arg;
    u8 state;

    if (i > 1) {
        i -= 2;
        state = 3;
    } else {
        state = 2;
    }

    fox = &gTramPauline->foxes[i];
    fox->unk_5 = state;
    fox->unk_F = 0;
    fox->unk_7 = i;
    sprite_set_anim(gSpriteHandler, fox->sprite,
                    tram_pauline_anim_table[gTramPauline->version][2][fox->unk_14], 0, 0, 0, 0);
    play_sound_in_player(3, &s_tran_jump_se_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_0804016c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0804016c] Fox transforms (hit)
//
// Only in mid-jump (states 2 and 3). Forms 0/1 are the two foxes' own
// shapes, 2/3 the swapped ones, 4/5 the half-way ones a barely leaves.
void func_0804016c(u8 i) {
    struct Fox *fox = &gTramPauline->foxes[i];
    u32 palette = 0xff;

    if ((u8)(fox->unk_5 - 2) > 1) return;
    if (fox->unk_5 == 2) fox->unk_5 = 5;
    if (fox->unk_14 > 5) return;

    switch (fox->unk_14) {
        case 0:
            fox->unk_14 = 2;
            break;
        case 1:
            fox->unk_14 = 3;
            palette = 0;
            break;
        case 2:
        case 4:
            fox->unk_14 = 0;
            break;
        case 3:
        case 5:
            fox->unk_14 = 1;
            palette = 3;
            break;
    }

    sprite_set_anim(gSpriteHandler, fox->sprite,
                    tram_pauline_anim_table[gTramPauline->version][3][fox->unk_14], 0, 1, 0x7f, 0);
    if (palette != 0xff) {
        sprite_set_base_palette(gSpriteHandler, fox->sprite, palette);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_08040314.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040314] Fox half-transforms (barely)
void func_08040314(u8 i) {
    struct Fox *fox = &gTramPauline->foxes[i];

    if ((u8)(fox->unk_5 - 2) > 1) return;
    if (fox->unk_5 == 2) fox->unk_5 = 5;
    if (fox->unk_14 > 5) return;

    if ((fox->unk_14 & 1) == 0) {
        fox->unk_14 = 4;
        sprite_set_anim(gSpriteHandler, fox->sprite,
                        tram_pauline_anim_table[gTramPauline->version][3][fox->unk_14], 0, 1, 0x7f, 0);
    } else {
        fox->unk_14 = 5;
        sprite_set_anim(gSpriteHandler, fox->sprite,
                        tram_pauline_anim_table[gTramPauline->version][3][fox->unk_14], 0, 1, 0x7f, 0);
        sprite_set_base_palette(gSpriteHandler, fox->sprite, 0);
    }
}
#endif


void func_08040434(u32 arg0) { 
    gTramPauline->unk_40 = arg0;
    
    switch (arg0) {
        case 0:
            sprite_set_x_y(gSpriteHandler, gTramPauline->textSprite, 0x50, 0x40);
            break;
        case 1:
            sprite_set_x_y(gSpriteHandler, gTramPauline->textSprite, 0xa0, 0x40);
            break;
        case 2:
            sprite_set_x_y(gSpriteHandler, gTramPauline->textSprite, 0x78, 0x40);
            break;
        case 3:
            sprite_set_x_y(gSpriteHandler, gTramPauline->textSprite, 0x78, 0x40);
            sprite_set_visible(gSpriteHandler, gTramPauline->skipTutorialSprite, FALSE);
            break;
    }
}

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_080404c4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080404c4] Flex the fox's trampoline
//
// Picks the trampoline cel from how far into the jump (mode 0 = the
// take-off, 1 = the landing, 2 = a big jump) the fox's timer is. The two
// trampolines sit 4 bytes apart on the GBA, at 0x2C and 0x30.
void func_080404c4(u8 i, u8 mode) {
    s16 trampoline = (i == 0) ? gTramPauline->trampolineSprite : gTramPauline->trampolineSprite2;
    u32 t = gTramPauline->foxes[i].unk_F;
    s8 cel;

    if (mode == 0) {
        if (t < ticks_to_frames(0x3)) cel = 1;
        else if (t < ticks_to_frames(0x6)) cel = 2;
        else if (t < ticks_to_frames(0x9)) cel = 3;
        else if (t < ticks_to_frames(0x15)) cel = 4;
        else if (t < ticks_to_frames(0x18)) cel = 3;
        else cel = 2;
    } else if (mode == 1) {
        if (t < ticks_to_frames(0xc)) cel = 2;
        else if (t < ticks_to_frames(0x12)) cel = 1;
        else cel = 0;
    } else {
        if (t < ticks_to_frames(0x3)) cel = 3;
        else if (t < ticks_to_frames(0x6)) cel = 4;
        else if (t < ticks_to_frames(0x9)) cel = 5;
        else if (t < ticks_to_frames(0xc)) cel = 4;
        else if (t < ticks_to_frames(0xf)) cel = 5;
        else if (t < ticks_to_frames(0x12)) cel = 4;
        else if (t < ticks_to_frames(0x15)) cel = 3;
        else if (t < ticks_to_frames(0x18)) cel = 2;
        else cel = 3;
    }

    sprite_set_anim_cel(gSpriteHandler, trampoline, cel);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_08040718.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040718] Move the foxes
//
// Each state runs the fox's timer and puts it on a parabola: 0 is the idle
// bounce, 1 the settle after landing, 2-5 a jump whose length and height
// come from D_0805a914 / D_0805a91c. All of it is unsigned arithmetic, as in
// the original; the squared terms come out right modulo 2^32.
void func_08040718(void) {
    struct Fox *fox;
    u32 a, b, c, d, t, dur, base;
    u8 i;

    for (i = 0; i < 2; i++) {
        fox = &gTramPauline->foxes[i];

        if (fox->unk_5 == 0) {
            fox->unk_F++;
            a = ticks_to_frames(0xc);
            b = ticks_to_frames(0xc);
            c = ticks_to_frames(0xc);
            d = ticks_to_frames(0xc);
            t = gTramPauline->foxes[i].unk_F;
            fox->y = ((((t - b) * (t - a)) << 4) / (c * d)) + 0x88;
            sprite_set_y(gSpriteHandler, fox->sprite, (s16)fox->y);
            func_080404c4(i, 0);
        } else if (fox->unk_5 == 1) {
            fox->unk_F++;
            d = ticks_to_frames(0x18);
            fox->y = ((fox->unk_F << 3) / d) + 0x90;
            sprite_set_y(gSpriteHandler, fox->sprite, (s16)fox->y);
            func_080404c4(i, 1);
        } else if ((u8)(fox->unk_5 - 2) <= 3) {
            fox->unk_F++;
            dur = D_0805a914[fox->unk_7];
            base = D_0805a91c[fox->unk_7];
            a = ticks_to_frames(dur);
            b = ticks_to_frames(dur);
            c = ticks_to_frames(dur);
            d = ticks_to_frames(dur);
            t = fox->unk_F;
            fox->y = base + (((0x98 - base) * ((t - a) * (t - b))) / (c * d));
            sprite_set_y(gSpriteHandler, fox->sprite, (s16)fox->y);
            func_080404c4(i, 2);

            if ((fox->unk_F > ticks_to_frames(dur + 6)) && (fox->unk_5 == 3)) {
                fox->unk_5 = 4;
                sprite_set_anim(gSpriteHandler, fox->sprite,
                                tram_pauline_anim_table[gTramPauline->version][1][fox->unk_14], 0, 1, 0x7f, 0);
            }

            if (fox->unk_F >= ticks_to_frames(dur * 2)) {
                fox->unk_5 = 0;
                fox->unk_F = 0;
                sprite_set_y(gSpriteHandler, fox->sprite, 0x98);
                sprite_set_anim(gSpriteHandler, fox->sprite,
                                tram_pauline_anim_table[gTramPauline->version][0][fox->unk_14], 0, 0, 0, 0);
            }
            continue;
        } else {
            continue;
        }

        // States 0 and 1 settle back onto the trampoline at the end.
        if (fox->unk_F >= ticks_to_frames(0x18)) {
            sprite_set_y(gSpriteHandler, fox->sprite, 0x98);
        }
    }
}
#endif

void tram_pauline_engine_update() {
    s32 unk;
    s32 unk2;
    s16 pos;
    
    func_08040718(); // looks like this one updates the positions of the foxes.. lots of math... eek..

    // this is probably the curtains opening thing
    
    if (gTramPauline->version != 1) {
        return;
    }

    if (gTramPauline->curtainScroll > 0x9fff) {
        return;
    }

    unk = ticks_to_frames(0xc0);
    gTramPauline->curtainScroll += 0xa000 / unk;

    pos = gTramPauline->curtainScroll * 0x100 >> 0x10;
    
    scene_set_bg_layer_pos(BG_LAYER_1, 0, pos);
}

void tram_pauline_engine_stop(void) {
}

void tram_pauline_cue_spawn(struct Cue *cue, struct TramPaulineCue *info, u32 character) {
    info->unk = character;
}

u32 tram_pauline_cue_update(struct Cue *cue, struct TramPaulineCue *info, u32 duration) {
    u8 i;
    s32 unk;

    for (i = 0; i < 2; i++) {
        sprite_set_x_y(gSpriteHandler, 
            gTramPauline->unkSprites[i], 
            gTramPauline->foxes[i].x, 
            gTramPauline->foxes[i].y);
    }

    unk = ticks_to_frames(0x30);

    if (duration > unk) {
        return 1;
    } else {
        return 0;
    }
}

void tram_pauline_cue_despawn(struct Cue *cue, struct TramPaulineCue *info) {
}

#ifndef PLATFORM_PC
#include "asm/engines/tram_and_pauline/asm_08040a84.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08040a84] Cue - Hit
void tram_pauline_cue_hit(struct Cue *cue, struct TramPaulineCue *info, u32 pressed, u32 released) {
    struct Animation *anim;

    func_0804016c(info->unk);
    anim = TRAM_PAULINE_IS_CIRCUS() ? anim_circus_fox_transform_effect : anim_casual_fox_transform_effect;
    sprite_set_anim(gSpriteHandler, gTramPauline->unkSprites[info->unk], anim, 0, 1, 0x7f, 2);
    sprite_set_visible(gSpriteHandler, gTramPauline->unkSprites[info->unk], TRUE);
}
#endif

void tram_pauline_cue_barely(struct Cue *cue, struct TramPaulineCue *info, u32 pressed, u32 released) {
    func_08040314(info->unk);
    beatscript_enable_loops();
}

void tram_pauline_cue_miss(struct Cue *cue, struct TramPaulineCue *info) {
    beatscript_enable_loops();
}

void tram_pauline_input_event(u32 pressed, u32 released) {
}

void tram_pauline_common_beat_animation(void) {
    u8 i;
    
    for (i = 0; i <= 1; i++) {
        if (gTramPauline->foxes[i].unk_5 <= 1) {
           gTramPauline->foxes[i].unk_F = 0;
        }
    }
}

void tram_pauline_common_display_text(const char *text) {
    struct PrintedTextAnim *textAnim;
    
    if (text == NULL) {
        sprite_set_visible(gSpriteHandler, gTramPauline->textSprite, FALSE);
    } else {
        delete_bmp_font_obj_text_anim(gTramPauline->font, gTramPauline->textSprite);
        textAnim = bmp_font_obj_print_c(gTramPauline->font, text, 1, 0xc);
        sprite_set_anim(gSpriteHandler, gTramPauline->textSprite, textAnim->frames, 0, 1, 0, 0);
        sprite_set_visible(gSpriteHandler, gTramPauline->textSprite, TRUE);
    }
}

void tram_pauline_common_init_tutorial(struct Scene *skipDestination) {
    if (skipDestination != NULL) {
        gameplay_enable_tutorial(TRUE);
        gameplay_set_skip_destination(skipDestination);
        sprite_set_visible(gSpriteHandler, gTramPauline->skipTutorialSprite, TRUE);
    } else {
        gameplay_enable_tutorial(FALSE);
        sprite_set_visible(gSpriteHandler, gTramPauline->skipTutorialSprite, FALSE);
    }
}
