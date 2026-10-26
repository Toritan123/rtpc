#include "engines/ninja_bodyguard.h"
#include "src/scenes/gameplay.h"
#include "src/code_08001360.h"
#include "src/code_08007468.h"
#include "src/code_08008dcc.h"
#include "src/code_0800b778.h"
#include "src/task_pool.h"
#include "src/lib_0804ca80.h"
#include "src/audio.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gNinjaBodyguard ((struct NinjaBodyguardEngineData *)gCurrentEngineData)


/* NINJA BODYGUARD */


#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803bd88.s"
#else
// Get Animation
struct Animation *ninja_get_anim(u32 anim) {
    return ninja_bodyguard_anim_table[anim][gNinjaBodyguard->version];
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803bda8.s"
#else
// Create Cutscene Arrow
void func_0803bda8(void) {
    struct AffineSprite *sprite;

    sprite = create_affine_sprite(ninja_get_anim(NINJA_ANIM_CUTSCENE_ARROW), 0, 120, 200, 0x800, 0x100, 0x200, 0, 0, 0x8000, 0);
    gNinjaBodyguard->cutsceneArrow = sprite;
    affine_sprite_rotate_with_orbit(sprite, TRUE);
    gNinjaBodyguard->cutsceneArrowDuration = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803be04.s"
#else
// Engine Event 0x10 (Show/Hide Cutscene BG)
void func_0803be04(u32 show) {
    scene_set_bg_layer_display(BG_LAYER_0, show, 0, 0, 2, 28, 0);
    if (!show) {
        affine_sprite_set_visible(gNinjaBodyguard->cutsceneArrow, FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803be44.s"
#else
// Update Cutscene Arrow Orbit
void func_0803be44(void) {
    s32 angle;

    angle = math_lerp(0x600 - 200, 0x600 + 200, gNinjaBodyguard->cutsceneArrowTime, gNinjaBodyguard->cutsceneArrowDuration);
    affine_sprite_set_orbit(gNinjaBodyguard->cutsceneArrow, angle, 120);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803be88.s"
#else
// Engine Event 0x11 (Fly Cutscene Arrow)
void func_0803be88(u32 ticks) {
    gNinjaBodyguard->cutsceneArrowTime = 0;
    gNinjaBodyguard->cutsceneArrowDuration = ticks_to_frames(ticks);
    affine_sprite_set_visible(gNinjaBodyguard->cutsceneArrow, TRUE);
    func_0803be44();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803bec4.s"
#else
// Update Cutscene Arrow
void func_0803bec4(void) {
    if (gNinjaBodyguard->cutsceneArrowDuration != 0) {
        gNinjaBodyguard->cutsceneArrowTime++;
        func_0803be44();
        if (gNinjaBodyguard->cutsceneArrowTime >= gNinjaBodyguard->cutsceneArrowDuration) {
            gNinjaBodyguard->cutsceneArrowDuration = 0;
            affine_sprite_set_visible(gNinjaBodyguard->cutsceneArrow, FALSE);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803bf14.s"
#else
// Create Archers
void func_0803bf14(void) {
    u32 i;

    for (i = 0; i < 8; i++) {
        gNinjaBodyguard->archers[i] = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_ARCHER_DRAW), 0, 130, 115, 0x4800, 0, 0, 0x8000);
    }
    gNinjaBodyguard->archerCount = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803bf74.s"
#else
// Engine Event 0x07 (Line Up Archers)
void func_0803bf74(u32 count) {
    u32 i;
    s32 y;

    gNinjaBodyguard->archerCount = count;
    gNinjaBodyguard->nextArcher = 0;

    y = 115 - (count - 1) * 8;
    for (i = 0; i < count; i++) {
        sprite_set_x_y_z(gSpriteHandler, gNinjaBodyguard->archers[i],
                         ((count - 1) * 16) - (i * 32) + 98, y, 0x4800 + count - i);
        sprite_set_anim(gSpriteHandler, gNinjaBodyguard->archers[i], ninja_get_anim(NINJA_ANIM_ARCHER_DRAW), 0, 0, 0, 0);
        y += 16;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c034.s"
#else
// Engine Event 0x08 (Archers Draw)
void func_0803c034(void) {
    u32 i;

    for (i = 0; i < gNinjaBodyguard->archerCount; i++) {
        sprite_set_playback(gSpriteHandler, gNinjaBodyguard->archers[i], 1, 0x7f, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c08c.s"
#else
// Engine Event 0x09 (Next Archer Fires)
// With no archers the original divides by zero in __umodsi3 and then skips
// everything (the remainder is never below 0); that case returns early here.
void func_0803c08c(u32 keepDrawn) {
    struct SoundPlayer *player;
    u32 count = gNinjaBodyguard->archerCount;
    u32 archer;

    if (count == 0) {
        return;
    }

    archer = (u16)(gNinjaBodyguard->nextArcher % count);
    if (archer >= count) {
        return;
    }

    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->archers[archer], ninja_get_anim(NINJA_ANIM_ARCHER_FIRE), 0, 1, 0x7f, 0);
    player = play_sound_in_player(4, &s_ninja_yumi_seqData);
    set_soundplayer_volume(player, gNinjaBodyguard->cueVolume);
    set_soundplayer_pitch(player, (s32)(((u32)gNinjaBodyguard->cueVolume << 17) + 0xfe000000) >> 16);

    gNinjaBodyguard->nextArcher++;
    if ((gNinjaBodyguard->nextArcher >= gNinjaBodyguard->archerCount) && (keepDrawn == 0)) {
        archer = (u16)(gNinjaBodyguard->nextArcher % gNinjaBodyguard->archerCount);
        sprite_set_anim(gSpriteHandler, gNinjaBodyguard->archers[archer], ninja_get_anim(NINJA_ANIM_ARCHER_DRAW), 0, 1, 0x7f, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c190.s"
#else
// Show Scene 0 (Ninja & Lord)
void func_0803c190(void) {
    scene_show_bg_layer(BG_LAYER_1);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->ninja, TRUE);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->lord, TRUE);
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->lord, ninja_get_anim(NINJA_ANIM_LORD_BLINK), 0, 1, 0, 0);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->buttonIndicator, gNinjaBodyguard->buttonIndicatorVisible);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c20c.s"
#else
// Show Scene 1 (Archers)
void func_0803c20c(void) {
    u32 i;

    scene_show_bg_layer(BG_LAYER_2);
    for (i = 0; i < gNinjaBodyguard->archerCount; i++) {
        sprite_set_visible(gSpriteHandler, gNinjaBodyguard->archers[i], TRUE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c260.s"
#else
// Show Scene 2 (Lord)
void func_0803c260(void) {
    scene_show_bg_layer(BG_LAYER_1);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->lord, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c28c.s"
#else
// Show Scene 3 (Lord)
void func_0803c28c(void) {
    scene_show_bg_layer(BG_LAYER_1);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->lord, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c2b8.s"
#else
// Engine Event 0x0B (Show Scene)
void func_0803c2b8(u32 scene) {
    switch (scene) {
        case 0:
            func_0803c190();
            break;
        case 1:
            func_0803c20c();
            break;
        case 2:
            func_0803c260();
            break;
        case 3:
            func_0803c28c();
            break;
    }
    gNinjaBodyguard->currentScene = scene;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c2f4.s"
#else
// Hide Scenes 0, 2, 3
void func_0803c2f4(void) {
    u32 i;

    scene_hide_bg_layer(BG_LAYER_0);
    scene_hide_bg_layer(BG_LAYER_1);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->ninja, FALSE);
    for (i = 0; i < 16; i++) {
        sprite_set_visible(gSpriteHandler, gNinjaBodyguard->arrows[i], FALSE);
    }
    gNinjaBodyguard->nextArrow = 0;
    for (i = 0; i < 24; i++) {
        affine_sprite_set_visible(gNinjaBodyguard->pieces[i].sprite, FALSE);
    }
    gNinjaBodyguard->nextArrow = 0;
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->lord, FALSE);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->wallArrow, FALSE);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->heartEyes, FALSE);
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->buttonIndicator, FALSE);
    gameplay_set_input_buttons(0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c3c4.s"
#else
// Hide Scene 1
void func_0803c3c4(void) {
    u32 i;

    scene_hide_bg_layer(BG_LAYER_2);
    for (i = 0; i < 8; i++) {
        sprite_set_visible(gSpriteHandler, gNinjaBodyguard->archers[i], FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c400.s"
#else
// Engine Event 0x0C (Hide Current Scene)
void func_0803c400(void) {
    D_03004b10.bgPalette[0][0] = 0;

    switch (gNinjaBodyguard->currentScene) {
        case 0:
        case 2:
        case 3:
            func_0803c2f4();
            break;
        case 1:
            func_0803c3c4();
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c43c.s"
#else
// Create Ninja, Arrows and Arrow Pieces
void func_0803c43c(void) {
    u32 i;

    gNinjaBodyguard->ninja = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_APPEAR), 0, 100, 120, 0x4800, 1, 0x7f, 0x8000);

    for (i = 0; i < 16; i++) {
        gNinjaBodyguard->arrows[i] = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_ARROW_DEFLECT_L), 0, 133, 78, 0x47f6, 0, 0, 0x8000);
    }
    gNinjaBodyguard->nextArrow = 0;

    for (i = 0; i < 24; i++) {
        gNinjaBodyguard->pieces[i].sprite = create_affine_sprite(ninja_get_anim(NINJA_ANIM_ARROW_PIECES), 0, 0, 0, 0x4800, 0x100, 0, 0, 0, 0x8000, 1);
    }
    gNinjaBodyguard->nextPiece = 0;
    gNinjaBodyguard->barelyCount = 0;
    gNinjaBodyguard->missCount = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c52c.s"
#else
// Update Arrow Pieces
void func_0803c52c(void) {
    struct NinjaArrowPiece *piece;
    s32 x, y;
    u32 i;

    for (i = 0; i < 24; i++) {
        piece = &gNinjaBodyguard->pieces[i];

        piece->velY += piece->gravity;
        piece->x += piece->velX;
        piece->y += piece->velY;
        piece->rotation += piece->rotationSpeed;
        affine_sprite_set_x_y(piece->sprite, (s16)(piece->x >> 8), (s16)(piece->y >> 8));
        affine_sprite_set_rotation(piece->sprite, piece->rotation);

        // Stop once the piece reaches the (sloped) floor.
        x = piece->x >> 8;
        y = piece->y >> 8;
        if ((y - 120) > ((x - 100) / 4)) {
            piece->gravity = 0;
            piece->rotationSpeed = 0;
            piece->velY = 0;
            piece->velX = 0;
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c5c0.s"
#else
// Update Arrow Pieces (Wrapper)
void func_0803c5c0(void) {
    func_0803c52c();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c5cc.s"
#else
// Engine Event 0x03 (Ninja Appears)
void func_0803c5cc(void) {
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->ninja, TRUE);
    play_sound(&s_ninja_chakuti_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c5f8.s"
#else
// Engine Event 0x04 (Ninja Raises Sword)
void func_0803c5f8(void) {
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_RAISE_SWORD), 0, 1, 0x7f, 0);
    play_sound(&s_ninja_kamae_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c638.s"
#else
// Engine Event 0x05 (Set Swing Side)
void func_0803c638(u32 side) {
    gNinjaBodyguard->swingSide = side;

    switch (side) {
        case 0:
            gameplay_set_input_buttons(A_BUTTON, 0);
            // startCel is R3, still 0x7f from the loopCel argument.
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SWING_L), 0x7f, 0, 0x7f, 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 0);
            break;
        case 1:
            gameplay_set_input_buttons((DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN), 0);
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SWING_R), 0x7f, 0, 0x7f, 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 1);
            break;
    }

    gNinjaBodyguard->barelyCount = 0;
    gNinjaBodyguard->missCount = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c6fc.s"
#else
// Engine Event 0x0D (Set Button Indicator Visibility)
void func_0803c6fc(u32 visible) {
    gNinjaBodyguard->buttonIndicatorVisible = visible;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c710.s"
#else
// Spawn Arrow (0 = deflect right, 1 = deflect left, 2 = into the wall, 3 = to the ninja)
void func_0803c710(u32 type) {
    struct SpriteVector3 *pos;
    struct Animation *anim = NULL;
    s16 sprite;
    s32 x = 0, y = 0;
    u16 z = 0;

    sprite = gNinjaBodyguard->arrows[gNinjaBodyguard->nextArrow];

    switch (type) {
        case 0:
            x = 133; y = 78; z = 0x47f6;
            anim = ninja_get_anim(NINJA_ANIM_ARROW_DEFLECT_R);
            break;
        case 1:
            x = 133; y = 78; z = 0x47f6;
            anim = ninja_get_anim(NINJA_ANIM_ARROW_DEFLECT_L);
            break;
        case 2:
            pos = &D_089e69cc[agb_random(4)];
            x = pos->x; y = pos->y; z = pos->z;
            anim = ninja_get_anim(NINJA_ANIM_ARROW_TO_WALL);
            break;
        case 3:
            x = 133; y = 78; z = 0x47f6;
            anim = ninja_get_anim(NINJA_ANIM_ARROW_TO_NINJA);
            break;
    }

    x = x - 4 + agb_random(9);
    y = y - 4 + agb_random(9);
    sprite_set_x_y_z(gSpriteHandler, sprite, x, y, z);
    sprite_set_anim(gSpriteHandler, sprite, anim, 0, 1, 0x7f, 0);
    sprite_set_visible(gSpriteHandler, sprite, TRUE);

    gNinjaBodyguard->nextArrow++;
    if (gNinjaBodyguard->nextArrow > 15) {
        gNinjaBodyguard->nextArrow = 0;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c834.s"
#else
// Spawn Arrow Piece
void func_0803c834(u32 cel, s32 x, s32 velX, s32 velY, s16 rotationSpeed) {
    struct NinjaArrowPiece *piece = &gNinjaBodyguard->pieces[gNinjaBodyguard->nextPiece];

    x += 92;
    piece->x = x << 8;
    piece->y = 60 << 8;
    piece->velX = velX;
    piece->velY = velY;
    piece->gravity = 0x20;
    piece->rotation = 0;
    piece->rotationSpeed = rotationSpeed;
    affine_sprite_set_x_y(piece->sprite, x, 60);
    affine_sprite_set_anim_cel(piece->sprite, cel);
    affine_sprite_set_rotation(piece->sprite, 0);
    affine_sprite_set_visible(piece->sprite, TRUE);

    gNinjaBodyguard->nextPiece++;
    if (gNinjaBodyguard->nextPiece > 23) {
        gNinjaBodyguard->nextPiece = 0;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c8c4.s"
#else
// Spawn Both Halves of a Sliced Arrow
void func_0803c8c4(void) {
    s32 velX, velY;
    s16 rotationSpeed;

    velX = -0x80 - agb_random(0x100);
    velY = -0x100 - agb_random(0x100);
    rotationSpeed = -0x20 - agb_random(99);
    func_0803c834(0, -8, velX, velY, rotationSpeed);

    velX = agb_random(0x100) + 0x80;
    velY = -0x100 - agb_random(0x100);
    rotationSpeed = agb_random(99) + 0x20;
    func_0803c834(1, 8, velX, velY, rotationSpeed);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c960.s"
#else
// Engine Event 0x06 (STUB)
void func_0803c960(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c964.s"
#else
// Create Lord & Heart Eyes
void func_0803c964(void) {
    gNinjaBodyguard->lord = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_LORD_BLINK), 0, 140, 105, 0x4864, 1, 0, 0x8000);
    gNinjaBodyguard->heartEyes = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_HEART_EYES), 0, 138, 75, 0x4863, 1, 0, 0x8000);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c9f4.s"
#else
// (STUB)
void func_0803c9f4(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803c9f8.s"
#else
// Engine Event 0x00 (Lord Walks In)
void func_0803c9f8(u32 ticks) {
    sprite_set_visible(gSpriteHandler, gNinjaBodyguard->lord, TRUE);
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->lord, ninja_get_anim(NINJA_ANIM_LORD_WALK), 0, 1, 0, 0);

    if (ticks != 0) {
        scene_set_sprite_motion_lerp(gNinjaBodyguard->lord, -16, 66, 140, 105, ticks_to_frames(ticks));
    } else {
        sprite_set_x_y(gSpriteHandler, gNinjaBodyguard->lord, 140, 105);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803ca8c.s"
#else
// Engine Event 0x01 (Lord Stops)
void func_0803ca8c(void) {
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->lord, ninja_get_anim(NINJA_ANIM_LORD_WALK), 0, 0, 0, 0);
    play_sound(&s_f_ninja_v_nanu_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cad0.s"
#else
// Engine Event 0x02 (Lord Blinks)
void func_0803cad0(void) {
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->lord, ninja_get_anim(NINJA_ANIM_LORD_BLINK), 0, 1, 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cb0c.s"
#else
// Engine Event 0x0A (Arrow Hits the Wall)
void func_0803cb0c(void) {
    gNinjaBodyguard->wallArrow = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_ARROW_TO_WALL), 0, 168, 73, 0x485a, 1, 0x7f, 0);
    play_sound(&s_f_ninja_kabe_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cb60.s"
#else
// Graphics Init. 3
void ninja_bodyguard_init_gfx3(void) {
    func_0800c604(0);
    D_03004b10.bgPalette[0][0] = 0;
    gameplay_start_screen_fade_in();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cb7c.s"
#else
// Graphics Init. 2
void ninja_bodyguard_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), ninja_bodyguard_gfx_tables[gNinjaBodyguard->version], 0x2000);
    run_func_after_task(task, ninja_bodyguard_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cbbc.s"
#else
// Graphics Init. 1
void ninja_bodyguard_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), ninja_bodyguard_buffered_textures);
    run_func_after_task(task, ninja_bodyguard_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cbe8.s"
#else
// Game Engine Start
void ninja_bodyguard_engine_start(u32 version) {
    gNinjaBodyguard->version = version;
    ninja_bodyguard_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, FALSE, 0, 0, 0, 29, 1);
    scene_set_bg_layer_display(BG_LAYER_2, FALSE, 0, 0, 0, 30, 1);
    gNinjaBodyguard->currentScene = 0;

    func_0803c43c();
    func_0803c964();
    func_0803bf14();
    func_0803bda8();

    gNinjaBodyguard->buttonIndicator = sprite_create(gSpriteHandler, ninja_get_anim(NINJA_ANIM_BUTTON_INDICATOR), 0, 64, 64, 0x47f6, 0, 0, 0x8000);
    sprite_set_x_y(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 100, 120);
    gNinjaBodyguard->buttonIndicatorVisible = FALSE;
    gNinjaBodyguard->cueVolume = 0x100;
    gameplay_set_input_buttons(0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803ccb0.s"
#else
// Engine Event 0x12 (STUB)
void ninja_bodyguard_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803ccb4.s"
#else
// Engine Event 0x0E (Stop Music After a Delay)
void func_0803ccb4(u32 ticks) {
    schedule_function_call(get_current_mem_id(), scene_stop_music, 0, ticks_to_frames(ticks));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cce0.s"
#else
// Engine Event 0x0F (Set Cue Volume)
void func_0803cce0(u32 volume) {
    gNinjaBodyguard->cueVolume = volume;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803ccf4.s"
#else
// Game Engine Update
void ninja_bodyguard_engine_update(void) {
    func_0803c5c0();
    func_0803bec4();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cd04.s"
#else
// Game Engine Stop
void ninja_bodyguard_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cd08.s"
#else
// Cue - Spawn
void ninja_bodyguard_cue_spawn(struct Cue *cue, struct NinjaBodyguardCue *info, u32 unused) {
    info->volume = gNinjaBodyguard->cueVolume;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cd1c.s"
#else
// Cue - Update
u32 ninja_bodyguard_cue_update(struct Cue *cue, struct NinjaBodyguardCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x24));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cd38.s"
#else
// Cue - Despawn
void ninja_bodyguard_cue_despawn(struct Cue *cue, struct NinjaBodyguardCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cd3c.s"
#else
// Cue - Hit
void ninja_bodyguard_cue_hit(struct Cue *cue, struct NinjaBodyguardCue *info, u32 pressed, u32 released) {
    struct SoundPlayer *player;

    switch (gNinjaBodyguard->swingSide) {
        case 0:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SLICE_R), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons((DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN), 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 1);
            gNinjaBodyguard->swingSide = 1;
            break;
        case 1:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SLICE_L), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons(A_BUTTON, 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 0);
            gNinjaBodyguard->swingSide = 0;
            break;
    }

    func_0803c8c4();
    func_0803c710(3);
    player = play_sound(&s_ninja_hit_seqData);
    set_soundplayer_volume(player, info->volume);
    set_soundplayer_pitch(player, (s32)(((u32)info->volume << 17) + 0xfe000000) >> 16);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803ce24.s"
#else
// Cue - Barely
void ninja_bodyguard_cue_barely(struct Cue *cue, struct NinjaBodyguardCue *info, u32 pressed, u32 released) {
    switch (gNinjaBodyguard->swingSide) {
        case 0:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SLICE_R), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons((DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN), 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 1);
            gNinjaBodyguard->swingSide = 1;
            func_0803c710(0);
            break;
        case 1:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SLICE_L), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons(A_BUTTON, 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 0);
            gNinjaBodyguard->swingSide = 0;
            func_0803c710(1);
            break;
    }

    play_sound(&s_ninja_kin_seqData);
    gNinjaBodyguard->barelyCount++;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cf00.s"
#else
// Cue - Miss
void ninja_bodyguard_cue_miss(struct Cue *cue, struct NinjaBodyguardCue *info) {
    func_0803c710(2);
    sprite_set_anim(gSpriteHandler, gNinjaBodyguard->lord, ninja_get_anim(NINJA_ANIM_LORD_SCARED1), 0, 1, 0x7f, 0);
    play_sound(&s_f_ninja_kabe_seqData);
    gNinjaBodyguard->missCount++;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803cf60.s"
#else
// Input Event
void ninja_bodyguard_input_event(u32 pressed, u32 released) {
    switch (gNinjaBodyguard->swingSide) {
        case 0:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SWING_R), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons((DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN), 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 1);
            gNinjaBodyguard->swingSide = 1;
            break;
        case 1:
            sprite_set_anim(gSpriteHandler, gNinjaBodyguard->ninja, ninja_get_anim(NINJA_ANIM_SWING_L), 0, 1, 0x7f, 0);
            gameplay_set_input_buttons(A_BUTTON, 0);
            sprite_set_anim_cel(gSpriteHandler, gNinjaBodyguard->buttonIndicator, 0);
            gNinjaBodyguard->swingSide = 0;
            break;
    }

    play_sound(&s_ninja_furu_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803d010.s"
#else
// Common Event 0 (Beat Animation, Unimplemented)
void ninja_bodyguard_common_beat_animation(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/ninja_bodyguard/asm_0803d014.s"
#else
// Common Event 1 (Display Text, Unimplemented)
void ninja_bodyguard_common_display_text(void) {
}
#endif
