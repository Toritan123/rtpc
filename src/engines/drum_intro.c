#include "engines/drum_intro.h"
#include "src/scenes/gameplay.h"
#include "src/memory.h"
#include "src/code_08001360.h"
#include "src/task_pool.h"
#include "src/memory_heap.h"
#include "src/code_08007468.h"
#include "src/text_printer.h"
#include "src/code_0800b778.h"
#include "src/lib_0804ca80.h"
#include "src/audio.h"
#include "src/scenes/riq_main_scene.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gDrumIntro ((struct DrumIntroEngineData *)gCurrentEngineData)


/* DRUM INTRO */


#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080239a0.s"
#else
// Engine Event 0x08 (Play Tanuki & Monkey BGM Sequence)
void func_080239a0(u32 index) {
    play_drumtech_seq(tanuki_and_monkey_bgm_seq_table[index], 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080239bc.s"
#else
// Engine Event 0x09 (Show/Hide BG3, Set Scroll)
void func_080239bc(s32 x) {
    if (x < 0) {
        scene_hide_bg_layer(BG_LAYER_3);
    } else {
        scene_show_bg_layer(BG_LAYER_3);
        D_03004b10.BG_OFS[BG_LAYER_3].x = -(x * 108);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080239ec.s"
#else
// Use the Tanuki & Monkey OBJ Tiles
void func_080239ec(s16 sprite) {
    sprite_set_base_tile(gSpriteHandler, sprite, 0x280);
    sprite_set_base_palette(gSpriteHandler, sprite, 8);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023a18.s"
#else
// Create Tanuki & Monkey Sprites
void func_08023a18(void) {
    s16 *sprites;

    sprites = gDrumIntro->tanuki;
    sprites[0] = sprite_create(gSpriteHandler, anim_drum_tanuki_beat, 0x7f, 44, 140, 0x4c00, 1, 0x7f, 0);
    func_080239ec(sprites[0]);
    sprites[1] = sprite_create(gSpriteHandler, anim_drum_tanuki_use_tom_l, 0x7f, 39, 140, 0x4bf6, 1, 0x7f, 0);
    func_080239ec(sprites[1]);
    sprites[2] = sprite_create(gSpriteHandler, anim_drum_tanuki_use_tom_r, 0x7f, 49, 140, 0x4bf6, 1, 0x7f, 0);
    func_080239ec(sprites[2]);
    sprites[3] = sprite_create(gSpriteHandler, anim_drum_tanuki_kit_tom2, 0x7f, 44, 150, 0x4bec, 1, 0x7f, 0);
    func_080239ec(sprites[3]);

    sprites = gDrumIntro->monkey;
    sprites[0] = sprite_create(gSpriteHandler, anim_drum_monkey_beat, 0x7f, 96, 140, 0x4c00, 1, 0x7f, 0);
    func_080239ec(sprites[0]);
    sprites[1] = sprite_create(gSpriteHandler, anim_drum_monkey_use_snare_l, 0x7f, 96, 140, 0x4bf6, 1, 0x7f, 0);
    func_080239ec(sprites[1]);
    sprites[2] = sprite_create(gSpriteHandler, anim_drum_monkey_use_snare_r, 0x7f, 96, 140, 0x4bf6, 1, 0x7f, 0);
    func_080239ec(sprites[2]);
    sprites[3] = sprite_create(gSpriteHandler, anim_drum_monkey_kit_snare1, 0x7f, 96, 150, 0x4bec, 1, 0x7f, 0);
    func_080239ec(sprites[3]);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023bb8.s"
#else
// Engine Event 0x0B (Set Text Table)
void func_08023bb8(const char **textTable) {
    gDrumIntro->textTable = textTable;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023bcc.s"
#else
// Engine Event 0x0C (Display Current Text)
void func_08023bcc(void) {
    gameplay_display_text(gDrumIntro->textTable[gDrumIntro->textIndex]);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023bf4.s"
#else
// Engine Event 0x0D (Reset Text Index)
void func_08023bf4(void) {
    gDrumIntro->textIndex = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023c0c.s"
#else
// Engine Event 0x0E (Advance Text Index; Change Script After the Last)
void func_08023c0c(void) {
    gDrumIntro->textIndex++;
    if (gDrumIntro->textIndex > 2) {
        gDrumIntro->textIndex = 2;
        func_0801d95c(gDrumIntro->endScript);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023c44.s"
#else
// Engine Event 0x0F (Set Script)
void func_08023c44(const struct Beatscript *script) {
    gDrumIntro->endScript = script;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023c58.s"
#else
// Engine Event 0x0A (Set Auto-Input Cue Type)
void func_08023c58(u32 type) {
    gDrumIntro->autoCueType = type;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023c6c.s"
#else
// Cue - Spawn (Auto-Input)
void drum_intro_cue_spawn_auto(struct Cue *cue, struct DrumIntroCue *info, u32 buttonMask) {
    u32 type = gDrumIntro->autoCueType;

    info->type = type;
    gameplay_set_cue_input_buttons(cue, D_089dfe94[type]);

    switch (type) {
        case 0:
            play_drumtech_note(14, 0x100, 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->monkey[1], 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->monkey[3], 0);
            break;
        case 1:
            play_drumtech_note(14, 0x100, 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->monkey[2], 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->monkey[3], 0);
            break;
        case 2:
            play_drumtech_note(25, 0x100, 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->tanuki[1], 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->tanuki[3], 0);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d44.s"
#else
// Cue - Update (Auto-Input)
u32 drum_intro_cue_update_auto(struct Cue *cue, struct DrumIntroCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x78));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d60.s"
#else
// Cue - Despawn (Auto-Input)
void drum_intro_cue_despawn_auto(struct Cue *cue, struct DrumIntroCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d64.s"
#else
// Cue - Hit (Auto-Input)
void drum_intro_cue_hit_auto(struct Cue *cue, struct DrumIntroCue *info, u32 pressed, u32 released) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d68.s"
#else
// Cue - Barely (Auto-Input)
void drum_intro_cue_barely_auto(struct Cue *cue, struct DrumIntroCue *info, u32 pressed, u32 released) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d6c.s"
#else
// Cue - Miss (Auto-Input)
void drum_intro_cue_miss_auto(struct Cue *cue, struct DrumIntroCue *info) {
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023d78.s"
#else
// Flash a Kit Piece
void func_08023d78(s16 sprite, s8 *timer) {
    if (*timer >= 0) {
        *timer = 80;
        sprite_set_visible(gSpriteHandler, sprite, TRUE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023da0.s"
#else
// Drum Kit Event (STUB)
void func_08023da0(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023da4.s"
#else
// Drum Kit Event - D-Pad Left (Left Bass Drum)
void func_08023da4(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;

    sprite_set_anim_cel(gSpriteHandler, drummer->legL, 0);
    sprite_set_anim_cel(gSpriteHandler, drummer->bassL, 0);
    sprite_set_anim_cel(gSpriteHandler, drummer->body, 2);
    func_08023d78(drummer->bassL, &drummer->bassLTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023df8.s"
#else
// Drum Kit Event - B Button (Right Bass Drum)
void func_08023df8(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;

    sprite_set_anim_cel(gSpriteHandler, drummer->legR, 0);
    sprite_set_anim_cel(gSpriteHandler, drummer->bassR, 0);
    sprite_set_anim_cel(gSpriteHandler, drummer->body, 2);
    func_08023d78(drummer->bassR, &drummer->bassRTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023e4c.s"
#else
// Drum Kit Event (STUB)
void func_08023e4c(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023e50.s"
#else
// Drum Kit Event - D-Pad Right (Snare, Left Arm)
// The asm calls func_08024bd0() twice more for the position and discards the
// result: both drummers hit the snare at the same spot.
void func_08023e50(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_snare_l : anim_drum_samurai_use_snare_l;

    sprite_set_anim(gSpriteHandler, drummer->armL, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armL, 127, 87);
    sprite_set_anim_cel(gSpriteHandler, drummer->snare, 0);
    func_08023d78(drummer->snare, &drummer->snareTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023edc.s"
#else
// Drum Kit Event - A Button (Snare, Right Arm)
void func_08023edc(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_snare_r : anim_drum_samurai_use_snare_r;

    sprite_set_anim(gSpriteHandler, drummer->armR, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armR, 130, 85);
    sprite_set_anim_cel(gSpriteHandler, drummer->snare, 0);
    func_08023d78(drummer->snare, &drummer->snareTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023f68.s"
#else
// Drum Kit Event - STUB
void func_08023f68(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023f6c.s"
#else
// Drum Kit Event - D-Pad Down (Tom)
void func_08023f6c(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_tom : anim_drum_samurai_use_tom;

    sprite_set_anim(gSpriteHandler, drummer->armL, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armL, 123, 107);
    sprite_set_anim_cel(gSpriteHandler, drummer->tom, 0);
    func_08023d78(drummer->tom, &drummer->tomTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08023ffc.s"
#else
// Drum Kit Event - D-Pad Up (Drum Kit 2; Hi-Hat)
void func_08023ffc(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_hihat : anim_drum_samurai_use_hihat;

    sprite_set_anim(gSpriteHandler, drummer->armL, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armL, 118, 80);
    sprite_set_playback(gSpriteHandler, drummer->hihat, 1, 0x7f, 0);
    sprite_set_anim_cel(gSpriteHandler, drummer->hihat, 0);
    func_08023d78(drummer->hihat, &drummer->hihatTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080240a4.s"
#else
// Drum Kit Event - L Button (Splash)
void func_080240a4(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_splash : anim_drum_samurai_use_splash;

    sprite_set_anim(gSpriteHandler, drummer->armL, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armL, 110, 70);
    sprite_set_anim_cel(gSpriteHandler, drummer->splash, 0);
    func_08023d78(drummer->splash, &drummer->splashTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024134.s"
#else
// Drum Kit Event - R Button (Crash)
void func_08024134(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_use_crash : anim_drum_samurai_use_crash;

    sprite_set_anim(gSpriteHandler, drummer->armR, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armR, 130, 55);
    sprite_set_anim_cel(gSpriteHandler, drummer->crash, 0);
    func_08023d78(drummer->crash, &drummer->crashTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080241c0.s"
#else
// Drum Kit Event - D-Pad Up (Drum Kit 1; Snare Roll)
void func_080241c0(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct Animation *anim = func_08024bd0() ? anim_drum_player_snare_roll : anim_drum_samurai_snare_roll;

    sprite_set_anim(gSpriteHandler, drummer->armL, anim, 0, 1, 0x7f, 0);
    func_08024bd0();
    func_08024bd0();
    sprite_set_x_y(gSpriteHandler, drummer->armL, 127, 87);
    sprite_set_anim_cel(gSpriteHandler, drummer->snare, 1);
    func_08023d78(drummer->snare, &drummer->snareTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_0802424c.s"
#else
// Graphics Init. 3
void drum_intro_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
    D_03004b10.objPalette[15][1] = 0;
    if (gDrumIntro->version == ENGINE_VER_DRUM_INTRO_CUTSCENE) {
        D_03004b10.bgPalette[0][0] = 0;
        D_03004b10.objPalette[15][1] = 0x7fff;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_0802428c.s"
#else
// Graphics Init. 2
void drum_intro_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), drum_intro_gfx_tables[gDrumIntro->version], 0x2000);
    run_func_after_task(task, drum_intro_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080242cc.s"
#else
// Graphics Init. 1
void drum_intro_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), drum_intro_buffered_textures);
    run_func_after_task(task, drum_intro_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080242f8.s"
#else
// Game Engine Start
void drum_intro_engine_start(u32 version) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;
    struct TextPrinter *textPrinter;

    gDrumIntro->version = version;
    drum_intro_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, FALSE, -64, -16, 0, 29, 1);

    // As in the drum kit events, the extra func_08024bd0() pair before each
    // sprite_create() picks between identical positions; its result is unused.
#define DRUMMER_ANIM(name) (func_08024bd0() ? anim_drum_player_##name : anim_drum_samurai_##name)
#define DRUMMER_POS() (func_08024bd0(), func_08024bd0())
    {
        struct Animation *anim;

        anim = DRUMMER_ANIM(body); DRUMMER_POS();
        drummer->body = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x4800, -1, 0, 0);
        anim = DRUMMER_ANIM(head); DRUMMER_POS();
        drummer->head = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x4805, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(waist); DRUMMER_POS();
        drummer->waist = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x47fb, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(use_snare_l); DRUMMER_POS();
        drummer->armL = sprite_create(gSpriteHandler, anim, 0x7f, 127, 87, 0x47ec, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(use_snare_r); DRUMMER_POS();
        drummer->armR = sprite_create(gSpriteHandler, anim, 0x7f, 130, 85, 0x480a, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(use_pedal_l); DRUMMER_POS();
        drummer->legL = sprite_create(gSpriteHandler, anim, 2, 120, 120, 0x47f6, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(use_pedal_r); DRUMMER_POS();
        drummer->legR = sprite_create(gSpriteHandler, anim, 2, 120, 120, 0x4814, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_bass_l); DRUMMER_POS();
        drummer->bassL = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x4864, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_bass_r); DRUMMER_POS();
        drummer->bassR = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x486e, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_snare); DRUMMER_POS();
        drummer->snare = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x480f, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_tom); DRUMMER_POS();
        drummer->tom = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x47f1, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_hihat); DRUMMER_POS();
        drummer->hihat = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x47f2, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_splash); DRUMMER_POS();
        drummer->splash = sprite_create(gSpriteHandler, anim, 0x7f, 120, 120, 0x4878, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_crash); DRUMMER_POS();
        drummer->crash = sprite_create(gSpriteHandler, anim, 0x7f, 120, 120, 0x4882, 1, 0x7f, 0);
        anim = DRUMMER_ANIM(kit_seat); DRUMMER_POS();
        drummer->seat = sprite_create(gSpriteHandler, anim, 0, 120, 120, 0x4805, 1, 0x7f, 0);
    }
#undef DRUMMER_ANIM
#undef DRUMMER_POS

    drummer->bassLTimer = -1;
    drummer->bassRTimer = -1;
    drummer->snareTimer = -1;
    drummer->tomTimer = -1;
    drummer->hihatTimer = -1;
    drummer->splashTimer = -1;
    drummer->crashTimer = -1;

    sprite_id_set_origin_x_y(gSpriteHandler, sprite_handler_get_mem_id(gSpriteHandler),
                             (s16 *)&D_03004b10.BG_OFS[BG_LAYER_1].x, (s16 *)&D_03004b10.BG_OFS[BG_LAYER_1].y);
    sprite_set_origin_x_y(gSpriteHandler, drummer->head, &drummer->originX, &drummer->originY);
    sprite_set_origin_x_y(gSpriteHandler, drummer->armL, &drummer->originX, &drummer->originY);
    sprite_set_origin_x_y(gSpriteHandler, drummer->armR, &drummer->originX, &drummer->originY);
    drummer->originX = D_03004b10.BG_OFS[BG_LAYER_1].x;
    drummer->originY = D_03004b10.BG_OFS[BG_LAYER_1].y;

    textPrinter = text_printer_create_new(get_current_mem_id(), 4, 128, 30);
    gameplay_set_text_printer(textPrinter);
    text_printer_set_palette(textPrinter, 15);
    text_printer_set_colors(textPrinter, 0);
    text_printer_center_by_content(textPrinter, TRUE);

    switch (gDrumIntro->version) {
        case ENGINE_VER_DRUM_INTRO_TEACHER:
            text_printer_set_x_y(textPrinter, 10, 48);
            break;
        case ENGINE_VER_DRUM_INTRO_PLAYER:
            text_printer_set_x_y(textPrinter, 12, 10);
            break;
        case ENGINE_VER_DRUM_INTRO_TANUKI_MONKEY:
            text_printer_set_x_y(textPrinter, 10, 32);
            break;
        case ENGINE_VER_DRUM_INTRO_CUTSCENE:
            text_printer_set_x_y(textPrinter, 10, 48);
            gameplay_set_text_advance_icon(1);
            break;
    }

    gameplay_set_input_buttons(0xf3, 0);
    gDrumIntro->inputEnabled = FALSE;
    gDrumIntro->drumMask = 0xffff;
    init_drumtech(&gDrumIntro->drumTech);
    func_08024ed0();
    gameplay_display_skip_icon(0);

    switch (version) {
        case ENGINE_VER_DRUM_INTRO_TEACHER:
            scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, 0);
            break;
        case ENGINE_VER_DRUM_INTRO_PLAYER:
            scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, 1);
            gDrumIntro->extraSprite = sprite_create(gSpriteHandler, anim_drum_player_unk25, 0, 56, 48, 0x48c8, 1, 0, 0x8000);
            break;
        case ENGINE_VER_DRUM_INTRO_TANUKI_MONKEY:
            scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, 1);
            scene_set_bg_layer_display(BG_LAYER_3, FALSE, -108, 0, 0, 31, 0);
            func_08023a18();
            scene_set_bg_layer_pos(BG_LAYER_1, -64, -20);
            break;
        case ENGINE_VER_DRUM_INTRO_CUTSCENE:
            // Every kit piece but the snare hides on the next frame and only
            // shows while it is being played.
            drummer->bassLTimer = 1;
            drummer->bassRTimer = 1;
            drummer->tomTimer = 1;
            drummer->hihatTimer = 1;
            drummer->splashTimer = 1;
            drummer->crashTimer = 1;
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024978.s"
#else
// Engine Event 0x10 (STUB)
void drum_intro_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_0802497c.s"
#else
// Engine Event 0x00 (Display the Text for the Current Loop)
void func_0802497c(const char **texts) {
    const char *prev = texts[0];
    u32 loopCount = gameplay_get_loop_counter();
    u32 i;

    for (i = 0; i <= loopCount; i++) {
        const char *text = texts[i];

        if (text == NULL) {
            gameplay_display_text(prev);
            return;
        }
        if (i == loopCount) {
            gameplay_display_text(text);
        }
        prev = text;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080249c0.s"
#else
// Engine Event 0x01 (Play Drum Kit)
void func_080249c0(u32 buttons) {
    play_drumtech_kit_w_anim(drum_intro_kits[gDrumIntro->version], gDrumIntro->drumMask & buttons);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080249f0.s"
#else
// Update Drummer Origin (follows the body's bounce)
void func_080249f0(void) {
    s8 cel = sprite_get_anim_cel(gSpriteHandler, gDrumIntro->drummer.body);
    struct Vector2 *offset = &D_089e0164[cel];

    gDrumIntro->drummer.originX = D_03004b10.BG_OFS[BG_LAYER_1].x + offset->x;
    gDrumIntro->drummer.originY = D_03004b10.BG_OFS[BG_LAYER_1].y + offset->y;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024a4c.s"
#else
// Engine Event 0x02 (Set Face Animation)
void func_08024a4c(u32 anim) {
    s16 head = gDrumIntro->drummer.head;

    sprite_set_z(gSpriteHandler, head, D_089e01b0[anim]);
    sprite_set_anim(gSpriteHandler, gDrumIntro->drummer.head, D_089e0170[anim][gDrumIntro->version],
                    D_089e01b8[anim][0], D_089e01b8[anim][1], D_089e01b8[anim][2], 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ae4.s"
#else
// Engine Event 0x03 (Set Extra Sprite State)
void func_08024ae4(u32 state) {
    switch (state) {
        case 0:
        case 1:
            sprite_set_visible(gSpriteHandler, gDrumIntro->extraSprite, FALSE);
            break;
        case 2:
            sprite_set_anim(gSpriteHandler, gDrumIntro->extraSprite, anim_drum_player_unk25, 0, 1, 0, 0);
            sprite_set_visible(gSpriteHandler, gDrumIntro->extraSprite, TRUE);
            break;
        case 3:
            sprite_set_anim(gSpriteHandler, gDrumIntro->extraSprite, anim_drum_player_unk24, 0, 1, 0x7f, 0);
            sprite_set_visible(gSpriteHandler, gDrumIntro->extraSprite, TRUE);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ba0.s"
#else
// Engine Event 0x04 (Enable/Disable Free Drumming)
void func_08024ba0(u32 enable) {
    gDrumIntro->inputEnabled = enable;
    if (enable) {
        gameplay_set_input_buttons(0x3f3, 0);
    } else {
        gameplay_set_input_buttons(0, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024bd0.s"
#else
// Get Drummer (0 = Samurai, 1 = Player)
u32 func_08024bd0(void) {
    return D_089e01c4[gDrumIntro->version];
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024be8.s"
#else
// Engine Event 0x07 (Set Drum Mask)
void func_08024be8(u32 mask) {
    gDrumIntro->drumMask = mask;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024bfc.s"
#else
// Update Kit Piece Flash Timer
void func_08024bfc(s16 sprite, s8 *timer) {
    if (*timer > 0) {
        if (--*timer == 0) {
            sprite_set_visible(gSpriteHandler, sprite, FALSE);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024c2c.s"
#else
// Update All Flash Timers
void func_08024c2c(void) {
    struct DrumIntroDrummer *drummer = &gDrumIntro->drummer;

    func_08024bfc(drummer->bassL, &drummer->bassLTimer);
    func_08024bfc(drummer->bassR, &drummer->bassRTimer);
    func_08024bfc(drummer->snare, &drummer->snareTimer);
    func_08024bfc(drummer->hihat, &drummer->hihatTimer);
    func_08024bfc(drummer->splash, &drummer->splashTimer);
    func_08024bfc(drummer->crash, &drummer->crashTimer);
    func_08024bfc(drummer->tom, &drummer->tomTimer);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024cb0.s"
#else
// Game Engine Update
void drum_intro_engine_update(void) {
    func_080249f0();

    if (gDrumIntro->inputEnabled) {
        play_drumtech_kit_w_anim(drum_intro_kits[gDrumIntro->version], D_03004afc & gDrumIntro->drumMask);
    }

    if (gDrumIntro->version == ENGINE_VER_DRUM_INTRO_TEACHER) {
        gDrumIntro->bgWobblePhase += 0x10;
        D_03004b10.BG_OFS[BG_LAYER_2].y = clamp_int32((sins(gDrumIntro->bgWobblePhase) * 3) >> 8, -1, 2);
    }

    update_drumtech();
    func_08024c2c();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024d44.s"
#else
// Game Engine Stop
void drum_intro_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024d48.s"
#else
// Cue - Spawn
void drum_intro_cue_spawn(struct Cue *cue, struct DrumIntroCue *info, u32 type) {
    info->type = type;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024d4c.s"
#else
// Cue - Update
u32 drum_intro_cue_update(struct Cue *cue, struct DrumIntroCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x30));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024d68.s"
#else
// Cue - Despawn
void drum_intro_cue_despawn(struct Cue *cue, struct DrumIntroCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024d6c.s"
#else
// Cue - Hit
void drum_intro_cue_hit(struct Cue *cue, struct DrumIntroCue *info, u32 pressed, u32 released) {
    s32 offset = gameplay_get_last_hit_offset();

    if ((gDrumIntro->mode != NULL) && (gDrumIntro->mode->hit != NULL)) {
        gDrumIntro->mode->hit(gDrumIntro->modeData, info, offset);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024da4.s"
#else
// Cue - Barely
void drum_intro_cue_barely(struct Cue *cue, struct DrumIntroCue *info, u32 pressed, u32 released) {
    s32 offset = gameplay_get_last_hit_offset();

    if ((gDrumIntro->mode != NULL) && (gDrumIntro->mode->barely != NULL)) {
        gDrumIntro->mode->barely(gDrumIntro->modeData, info, offset);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ddc.s"
#else
// Cue - Miss
void drum_intro_cue_miss(struct Cue *cue, struct DrumIntroCue *info) {
    if ((gDrumIntro->mode != NULL) && (gDrumIntro->mode->miss != NULL)) {
        gDrumIntro->mode->miss(gDrumIntro->modeData, info);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024e0c.s"
#else
// Input Event
void drum_intro_input_event(u32 pressed, u32 released) {
    if ((gDrumIntro->mode != NULL) && (gDrumIntro->mode->input != NULL)) {
        gDrumIntro->mode->input(gDrumIntro->modeData);
    }
    if (gDrumIntro->version == ENGINE_VER_DRUM_INTRO_TANUKI_MONKEY) {
        beatscript_enable_loops();
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024e48.s"
#else
// Common Event 0 (Beat Animation)
void drum_intro_common_beat_animation(void) {
    switch (gDrumIntro->version) {
        case ENGINE_VER_DRUM_INTRO_TEACHER:
        case ENGINE_VER_DRUM_INTRO_PLAYER:
        case ENGINE_VER_DRUM_INTRO_CUTSCENE:
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->drummer.head, 0);
            break;
        case ENGINE_VER_DRUM_INTRO_TANUKI_MONKEY:
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->drummer.head, 0);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->tanuki[0], 1);
            sprite_set_anim_cel(gSpriteHandler, gDrumIntro->monkey[0], 2);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ecc.s"
#else
// Common Event 1 (Display Text, Unimplemented)
void drum_intro_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ed0.s"
#else
// Clear Mode
void func_08024ed0(void) {
    gDrumIntro->modeID = 0;
    gDrumIntro->mode = NULL;
    gDrumIntro->modeData = NULL;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ef4.s"
#else
// Engine Event 0x05 (Set Mode)
void func_08024ef4(u32 modeID) {
    const struct DrumKitCueData *mode = D_089e0398[modeID];

    if (gDrumIntro->modeData != NULL) {
        mem_heap_dealloc(gDrumIntro->modeData);
        gDrumIntro->modeData = NULL;
    }

    gDrumIntro->modeID = modeID;
    gDrumIntro->mode = mode;

    if (mode != NULL) {
        gDrumIntro->modeData = mem_heap_alloc_id(get_current_mem_id(), mode->dataSize);
        if (mode->init != NULL) {
            mode->init(gDrumIntro->modeData);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024f64.s"
#else
// Engine Event 0x06 (Display Mode Result)
void func_08024f64(void) {
    const struct DrumKitCueData *mode = gDrumIntro->mode;
    struct SongHeader *sound;

    if (mode != NULL) {
        gDrumIntro->modeText = D_08059f94;
        sound = NULL;
        if (mode->getSound != NULL) {
            sound = mode->getSound(gDrumIntro->modeData);
        }
        gameplay_display_text(gDrumIntro->modeText);
        play_sound(sound);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024fb4.s"
#else
// Drum Kit Cue Func. 0 (Init.)
void func_08024fb4(void *data) {
    ((struct DrumIntroModeData *)data)->counter = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024fbc.s"
#else
// Drum Kit Cue Func. 1 (Input)
void func_08024fbc(void *data) {
    ((struct DrumIntroModeData *)data)->counter++;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024fc4.s"
#else
// Drum Kit Cue Func. 2 (Hit: 0 = perfect, 1 = off by more than a frame)
void func_08024fc4(void *data, struct DrumIntroCue *info, s32 offset) {
    struct DrumIntroModeData *mode = data;
    u8 rank = (ABS(offset) > 1) ? 1 : 0;

    if (info->type == 0) {
        mode->rankA = rank;
    } else {
        mode->rankB = rank;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08024ff4.s"
#else
// Drum Kit Cue Func. 3 (Barely: 2 = early, 3 = late)
void func_08024ff4(void *data, struct DrumIntroCue *info, s32 offset) {
    struct DrumIntroModeData *mode = data;
    u8 rank = (offset < 0) ? 2 : 3;

    if (info->type == 0) {
        mode->rankA = rank;
    } else {
        mode->rankB = rank;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025020.s"
#else
// Drum Kit Cue Func. 4 (Miss)
void func_08025020(void *data, struct DrumIntroCue *info) {
    struct DrumIntroModeData *mode = data;

    if (info->type == 0) {
        mode->rankA = 4;
    } else {
        mode->rankB = 4;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025038.s"
#else
// Drum Kit Cue Func. 5 (Pick the Result Text and Fanfare)
struct SongHeader *func_08025038(void *data) {
    struct DrumIntroModeData *mode = data;
    struct SongHeader *sound = &s_fanfare_drum3_seqData;
    u32 a = mode->rankA;
    u32 b = mode->rankB;

    gDrumIntro->modeText = D_08059f98;

    if (mode->counter > 2) {
        gDrumIntro->modeText = D_08059f9c;
    } else if (a == 4) {
        if (b == 4) {
            gDrumIntro->modeText = (mode->counter != 0) ? D_08059fb4 : D_08059fd0;
        } else {
            gDrumIntro->modeText = D_08059fe8;
        }
    } else if (b == 4) {
        gDrumIntro->modeText = D_0805a004;
    } else if (((a == 2) && (b == 3)) || ((a == 3) && (b == 2))) {
        gDrumIntro->modeText = D_0805a020;
    } else if ((a == 2) || (b == 2)) {
        gDrumIntro->modeText = D_0805a038;
    } else if ((a == 3) || (b == 3)) {
        gDrumIntro->modeText = D_0805a048;
    } else if ((a == 0) || (b == 0)) {
        gDrumIntro->modeText = D_0805a058;
        sound = &s_fanfare_drum1_seqData;
    } else {
        gDrumIntro->modeText = D_0805a06c;
        sound = &s_fanfare_drum2_seqData;
    }

    return sound;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251d0.s"
#else
// Drum Kit Cue Func. 0 (Init.)
void func_080251d0(void *data) {
    ((struct DrumIntroModeData *)data)->counter = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251d8.s"
#else
// Drum Kit Cue Func. 1 (Input)
void func_080251d8(void *data) {
    ((struct DrumIntroModeData *)data)->counter++;
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251e8.s"
#else
// Drum Kit Cue Func. 2 (STUB)
void func_080251e8(void *data, struct DrumIntroCue *info, s32 offset) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251ec.s"
#else
// Drum Kit Cue Func. 3 (STUB)
void func_080251ec(void *data, struct DrumIntroCue *info, s32 offset) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251f0.s"
#else
// Drum Kit Cue Func. 4 (Miss)
void func_080251f0(void *data, struct DrumIntroCue *info) {
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_080251fc.s"
#else
// Drum Kit Cue Func. 5
struct SongHeader *func_080251fc(void *data) {
    return &s_f_drumdr_miss_seqData;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025204.s"
#else
// Drum Kit Cue Func. 0 (Init.)
void func_08025204(void *data) {
    ((struct DrumIntroModeData *)data)->counter = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_0802520c.s"
#else
// Drum Kit Cue Func. 1 (Input)
void func_0802520c(void *data) {
    ((struct DrumIntroModeData *)data)->counter++;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025214.s"
#else
// Drum Kit Cue Func. 2 (STUB)
void func_08025214(void *data, struct DrumIntroCue *info, s32 offset) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025218.s"
#else
// Drum Kit Cue Func. 3 (STUB)
void func_08025218(void *data, struct DrumIntroCue *info, s32 offset) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_0802521c.s"
#else
// Drum Kit Cue Func. 4 (STUB)
void func_0802521c(void *data, struct DrumIntroCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/drumming_lessons/asm_08025220.s"
#else
// Drum Kit Cue Func. 5
struct SongHeader *func_08025220(void *data) {
    return &s_f_drumdr_miss_seqData;
}
#endif
