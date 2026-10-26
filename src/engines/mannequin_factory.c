#include "engines/mannequin_factory.h"

#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "src/scenes/riq_main_scene.h"
#include "src/scenes/gameplay.h"
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gMannequinFactory ((struct MannequinFactoryEngineData *)gCurrentEngineData)


/* MANNEQUIN FACTORY */


#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022244.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022244] Clear the finish effects
void func_08022244(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        gMannequinFactory->effects[i].state = 0;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022268.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022268] Update a finish effect
//
// State 1 (an OK mannequin) runs the crane, then state 3 its dash; state 2
// (a reject) shows the NG mark -- in version 1 just that, otherwise as a
// clone that ends the game.
void func_08022268(struct MannequinFinishEffect *effect) {
    s16 clone;

    if (!effect->state) return;
    if (++effect->timer < effect->duration) return;

    switch (effect->state) {
        case 1:
            sprite_set_anim(gSpriteHandler, effect->sprite, anim_mannequin_crane, 0, 1, 0x7f, 0);
            sprite_set_anim_speed(gSpriteHandler, effect->sprite, 0x100);
            effect->timer = 0;
            effect->duration = ticks_to_frames(0xc);
            effect->state = 3;
            play_sound(&s_poly_shototu_seqData);
            break;

        case 3:
            sprite_set_anim(gSpriteHandler, effect->sprite, anim_mannequin_crane_dash_effect, 0, 1, 0, 3);
            effect->state = 0;
            play_sound(&s_virus_fork_seqData);
            break;

        case 2:
            if (gMannequinFactory->version == 1) {
                sprite_set_anim(gSpriteHandler, effect->sprite, anim_mannequin_ng_effect, 0, 1, 0, 3);
                play_sound(&s_ghost_walk_seqData);
            } else {
                clone = sprite_clone(gSpriteHandler, effect->sprite);
                sprite_set_z(gSpriteHandler, clone, 0x48be);
                sprite_set_anim(gSpriteHandler, clone, anim_mannequin_ng_effect, 0, 1, 0, 3);
                sprite_set_anim_speed(gSpriteHandler, clone, 0x20);
                func_0802310c();
            }
            effect->state = 0;
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080223ac.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080223ac] Update the finish effects
void func_080223ac(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        func_08022268(&gMannequinFactory->effects[i]);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080223d0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080223d0] Start a finish effect for a mannequin of the given kind
//
// The effect waits a beat less the hit's timing offset before it moves on.
void func_080223d0(u32 kind, s32 offset) {
    struct MannequinFinishEffect *effect;
    u32 i;

    for (i = 0; i < 4; i++) {
        if (!gMannequinFactory->effects[i].state) break;
    }
    if (i > 3) return;

    effect = &gMannequinFactory->effects[i];
    effect->sprite = sprite_create(gSpriteHandler, mannequin_finish_anim[kind], 0, 0xa8, 0x32, 0x48c8, 1, 0x7f, 0);
    sprite_set_anim_speed(gSpriteHandler, effect->sprite, (u16)(((s32)gMannequinFactory->tempo << 8) / 100));
    effect->state = (kind == 1) ? 1 : 2;
    effect->timer = 0;
    effect->duration = clamp_int32(ticks_to_frames(0x18) - offset, 0, 9999);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080224a8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080224a8] Init. the mannequins
void func_080224a8(void) {
    struct Mannequin *mannequin;
    u32 i;

    for (i = 0; i < 6; i++) {
        mannequin = &gMannequinFactory->mannequins[i];
        mannequin->state = 0;
        mannequin->headSprite = sprite_create(gSpriteHandler, anim_mannequin_head, 0, 0x68, 0x40, 0x4864, 0, 0, 0x8000);
        mannequin->eyeLSprite = sprite_create(gSpriteHandler, anim_mannequin_eye_l, 0, 0x40, 0x40, 0x4863, 0, 0, 0x8000);
        mannequin->eyeRSprite = sprite_create(gSpriteHandler, anim_mannequin_eye_r, 0, 0x40, 0x40, 0x4863, 0, 0, 0x8000);
        mannequin->dashSprite = sprite_create(gSpriteHandler, anim_mannequin_dash_effect, 0, 0xc8, 0x40, 0x4865, 1, 0, 0x8002);
        sprite_set_y(gSpriteHandler, mannequin->eyeLSprite, 0x40);
        sprite_set_y(gSpriteHandler, mannequin->eyeRSprite, 0x40);
        sprite_set_y(gSpriteHandler, mannequin->dashSprite, 0x40);
        mannequin->velocity = 0;
    }

    gMannequinFactory->mode = 2;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080225bc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080225bc] Mannequin screen x
//
// 96 px per station. Normally pulled back by the slide-in offset and pushed
// by the slap recoil; in slow motion it glides on with the timer instead.
s32 func_080225bc(struct Mannequin *mannequin) {
    s32 x;

    if (gMannequinFactory->slowMotion) {
        x = (mannequin->position * 0x60) - 0x78;
        return x + ((gMannequinFactory->slowTimer << 3) / gMannequinFactory->slowDuration);
    }

    x = (mannequin->position * 0x60) - (mannequin->xOffset + 0x48);
    return x + (mannequin->velocity >> 8);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022614.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022614] Update a mannequin
void func_08022614(struct Mannequin *mannequin) {
    s16 x;
    s32 v;

    if (!mannequin->state) return;

    if (!gMannequinFactory->slowMotion) {
        mannequin->xOffset = (mannequin->xOffset * 0xc8) >> 8;
        mannequin->velocity = (-(mannequin->velocity * 0xf0)) >> 8;
        v = mannequin->velocity;
        if (v < 0) v = -v;
        if (v <= 0xbf) mannequin->velocity = 0;
    }

    x = func_080225bc(mannequin);
    sprite_set_x(gSpriteHandler, mannequin->headSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->eyeLSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->eyeRSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->dashSprite, x);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080226a0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080226a0] Update the mannequins
void func_080226a0(void) {
    u32 i;

    if (gMannequinFactory->stopped) return;
    for (i = 0; i < 6; i++) {
        func_08022614(&gMannequinFactory->mannequins[i]);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080226d4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080226d4] Engine Event 0x00 (Send In a Mannequin)
//
// Takes a free slot, picks its head (by mode) and schedules the belt's
// clanking for that kind.
void func_080226d4(void) {
    struct Mannequin *mannequin;
    s16 x;
    u32 i;

    if (gMannequinFactory->stopped) return;

    for (i = 0; i < 6; i++) {
        if (!gMannequinFactory->mannequins[i].state) break;
    }
    if (i > 5) return;
    mannequin = &gMannequinFactory->mannequins[i];

    mannequin->xOffset = 0;
    mannequin->position = 0;
    switch (gMannequinFactory->mode) {
        case 0:  mannequin->kind = 1; break;
        case 1:  mannequin->kind = 2; break;
        default: mannequin->kind = agb_random(2) + 1; break;
    }
    mannequin->eyeLVisible = FALSE;
    mannequin->eyeRVisible = FALSE;

    x = (s16)((((mannequin->position * 3) << 21) + (s32)0xffb80000) >> 16);
    sprite_set_x(gSpriteHandler, mannequin->headSprite, x);
    sprite_set_anim_cel(gSpriteHandler, mannequin->headSprite, (s8)mannequin->kind);
    sprite_set_visible(gSpriteHandler, mannequin->headSprite, TRUE);
    sprite_set_x(gSpriteHandler, mannequin->eyeLSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->eyeRSprite, x);
    sprite_set_anim_cel(gSpriteHandler, mannequin->eyeLSprite, (s8)mannequin->kind);
    sprite_set_anim_cel(gSpriteHandler, mannequin->eyeRSprite, (s8)mannequin->kind);
    sprite_set_visible(gSpriteHandler, mannequin->eyeLSprite, mannequin->eyeLVisible);
    sprite_set_visible(gSpriteHandler, mannequin->eyeRSprite, mannequin->eyeRVisible);

    if (mannequin->kind == 1) {
        func_080230cc(0, 0xa0, 0x00);
        func_080230cc(0, 0xc0, 0x0c);
        func_080230cc(0, 0x80, 0x24);
        func_080230cc(0, 0x100, 0x30);
        func_080230cc(1, 0x40, 0x48);
        func_080230cc(1, 0x18, 0x4c);
        func_080230cc(1, 0x18, 0x50);
        func_080230cc(1, 0x28, 0x54);
        func_080230cc(1, 0x20, 0x58);
        func_080230cc(1, 0x40, 0x5c);
        func_080230cc(1, 0x60, 0x60);
    } else {
        func_080230cc(0, 0xd0, 0x00);
        func_080230cc(0, 0x60, 0x0c);
        func_080230cc(0, 0x90, 0x11);
        func_080230cc(0, 0xb0, 0x18);
        func_080230cc(0, 0xc0, 0x24);
        func_080230cc(0, 0x100, 0x30);
    }

    mannequin->state = 1;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022894.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022894] Remove a mannequin
//
// The fourth sprite hidden is the one whose id is the mannequin's xOffset
// field (0x0E), not its dash sprite (0x08) -- that is what the assembly
// does, so it is kept.
void func_08022894(struct Mannequin *mannequin) {
    sprite_set_visible(gSpriteHandler, mannequin->headSprite, FALSE);
    sprite_set_visible(gSpriteHandler, mannequin->eyeLSprite, FALSE);
    sprite_set_visible(gSpriteHandler, mannequin->eyeRSprite, FALSE);
    sprite_set_visible(gSpriteHandler, (s16)mannequin->xOffset, FALSE);
    mannequin->state = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080228d8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080228d8] Spawn a mannequin's cue for its station
void func_080228d8(struct Mannequin *mannequin) {
    s16 clone;

    if (!mannequin->state) return;

    gMannequinFactory->current = mannequin;

    switch (mannequin->position) {
        case 0:
        case 4:
            gameplay_spawn_cue(5);
            break;
        case 1:
            gameplay_spawn_cue((mannequin->kind == 1) ? 4 : 3);
            break;
        case 2:
            gameplay_spawn_cue(0);
            break;
        case 3:
            // Reached the end unfinished: reject it.
            clone = sprite_clone(gSpriteHandler, mannequin->headSprite);
            sprite_set_z(gSpriteHandler, clone, 0x485a);
            sprite_set_anim(gSpriteHandler, clone, anim_mannequin_ng_effect, 0, 1, 0, 3);
            if (gMannequinFactory->version == 1) {
                func_08022894(mannequin);
                play_sound(&s_ghost_walk_seqData);
            } else {
                sprite_set_anim_speed(gSpriteHandler, clone, 0x20);
                func_0802310c();
            }
            break;
    }

    mannequin->advanced = FALSE;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080229bc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080229bc] Engine Event 0x01 (Spawn the Mannequins' Cues)
void func_080229bc(void) {
    u32 i;

    if (gMannequinFactory->stopped) return;
    for (i = 0; i < 6; i++) {
        func_080228d8(&gMannequinFactory->mannequins[i]);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080229f0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080229f0] Move a mannequin on to the next station
void func_080229f0(struct Mannequin *mannequin) {
    s16 x;

    if (mannequin->advanced) return;
    mannequin->advanced = TRUE;

    if (++mannequin->position > 4) {
        func_08022894(mannequin);
        return;
    }
    if (mannequin->position > 3) return;

    mannequin->xOffset = 0x20;
    x = func_080225bc(mannequin);
    sprite_set_x(gSpriteHandler, mannequin->headSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->eyeLSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->eyeRSprite, x);
    sprite_set_x(gSpriteHandler, mannequin->dashSprite, x);
    sprite_set_visible(gSpriteHandler, mannequin->dashSprite, TRUE);
    sprite_set_anim_cel(gSpriteHandler, mannequin->dashSprite, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022a7c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022a7c] Slap a mannequin's head round (direction +1 or -1)
void func_08022a7c(struct Mannequin *mannequin, s32 direction) {
    struct Animation *anim;

    mannequin->kind = (direction + 4 + mannequin->kind) % 4;
    sprite_set_anim_cel(gSpriteHandler, mannequin->headSprite, (s8)mannequin->kind);
    sprite_set_anim_cel(gSpriteHandler, mannequin->eyeLSprite, (s8)mannequin->kind);
    sprite_set_anim_cel(gSpriteHandler, mannequin->eyeRSprite, (s8)mannequin->kind);

    anim = (direction >= 0) ? anim_mannequin_slap_effect_l : anim_mannequin_slap_effect_r;
    sprite_create(gSpriteHandler, anim, 0, 0x48, 0x40, 0x4814, 1, 0, 3);
    mannequin->velocity = 0x300;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022b0c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022b0c] Stamp a mannequin's eye (side 0 = right, otherwise left)
void func_08022b0c(struct Mannequin *mannequin, u32 side) {
    struct Animation *anim;

    anim = (side != 0) ? anim_mannequin_stamp_effect_l : anim_mannequin_stamp_effect_r;
    sprite_create(gSpriteHandler, anim, 0, 0xa8, 0x40, 0x4814, 1, 0, 3);
    mannequin->velocity = 0x300;

    if (mannequin->kind != 1) return;

    if (side == 0) {
        mannequin->eyeRVisible = mannequin->kind;
        sprite_set_anim_cel(gSpriteHandler, mannequin->eyeRSprite, (s8)mannequin->kind);
        sprite_set_visible(gSpriteHandler, mannequin->eyeRSprite, mannequin->eyeRVisible);
    } else {
        mannequin->eyeLVisible = mannequin->kind;
        sprite_set_anim_cel(gSpriteHandler, mannequin->eyeLSprite, (s8)mannequin->kind);
        sprite_set_visible(gSpriteHandler, mannequin->eyeLSprite, mannequin->eyeLVisible);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022ba0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022ba0] Engine Event 0x06 (Set the Head Mode)
void func_08022ba0(u32 mode) {
    gMannequinFactory->mode = mode;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022bb4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022bb4] Init. the hands and stamps
void func_08022bb4(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        gMannequinFactory->buttonCooldowns[i] = 0;
    }

    gMannequinFactory->handRSprite = sprite_create(gSpriteHandler, anim_mannequin_hand_r, 0x7f, 0x2c, 0x58, 0x4800, 1, 0x7f, 0);
    gMannequinFactory->handLSprite = sprite_create(gSpriteHandler, anim_mannequin_hand_l, 0x7f, 0x54, 0x58, 0x4800, 1, 0x7f, 0x8000);
    sprite_set_x(gSpriteHandler, gMannequinFactory->handRSprite, 0x24);
    gMannequinFactory->stampRSprite = sprite_create(gSpriteHandler, anim_mannequin_stamp_r, 0x7f, 0xa8, 0x40, 0x4800, 1, 0x7f, 0);
    gMannequinFactory->stampLSprite = sprite_create(gSpriteHandler, anim_mannequin_stamp_l, 0x7f, 0xa8, 0x40, 0x4800, 1, 0x7f, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022ca0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022ca0] Count down the button cooldowns, re-enabling each at 0
void func_08022ca0(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (gMannequinFactory->buttonCooldowns[i] == 0) continue;
        if (--gMannequinFactory->buttonCooldowns[i] != 0) continue;
        gMannequinFactory->inputButtons |= mannequin_input_buttons[i];
        gameplay_set_input_buttons(gMannequinFactory->inputButtons, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022ce8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022ce8] Press a button: animate the hand or stamp, and lock it out
// for a moment. (0 = A/right stamp, 1 = B/left stamp, 2 = right hand,
// 3 = left hand.) Button 0 also resets the left stamp, as in the original.
void func_08022ce8(u32 button) {
    switch (button) {
        case 0:
            sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->stampRSprite, 0);
            sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->stampLSprite, 0);
            break;
        case 1:
            sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->stampLSprite, 0);
            break;
        case 2:
            sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->handLSprite, 0);
            sprite_set_z(gSpriteHandler, gMannequinFactory->handLSprite, 0x480a);
            sprite_set_z(gSpriteHandler, gMannequinFactory->handRSprite, 0x47f6);
            break;
        case 3:
            sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->handRSprite, 0);
            sprite_set_z(gSpriteHandler, gMannequinFactory->handLSprite, 0x47f6);
            sprite_set_z(gSpriteHandler, gMannequinFactory->handRSprite, 0x480a);
            break;
    }

    gMannequinFactory->inputButtons &= ~mannequin_input_buttons[button];
    gameplay_set_input_buttons(gMannequinFactory->inputButtons, 0);
    gMannequinFactory->buttonCooldowns[button] = ticks_to_frames(0x14);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022dec.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022dec] Graphics Init. 3
void mannequin_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022dfc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022dfc] Graphics Init. 2
void mannequin_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), mannequin_gfx_table, 0x2000);
    run_func_after_task(task, mannequin_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022e2c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022e2c] Graphics Init. 1
void mannequin_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), mannequin_buffered_textures);
    run_func_after_task(task, mannequin_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022e58.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022e58] Game Engine Start
void mannequin_engine_start(u32 version) {
    gMannequinFactory->version = version;
    mannequin_init_gfx1();
    scene_show_obj_layer();

    if (gMannequinFactory->version == 0) {
        scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1d, 1);
    } else {
        scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1e, 1);
    }

    func_08022bb4();
    func_080224a8();
    func_08022244();
    init_drumtech(&gMannequinFactory->drumTech);
    func_0802327c();

    gMannequinFactory->inputButtons = 0x21;
    gameplay_set_input_buttons(0x21, 0);
    gMannequinFactory->stopped = FALSE;
    gMannequinFactory->gameOverScript = NULL;
    gMannequinFactory->gameOver = FALSE;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022efc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022efc] Engine Event 0x07 (STUB)
void mannequin_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022f00.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022f00] Engine Event 0x02 (Set the Tempo)
void func_08022f00(u32 tempo) {
    gMannequinFactory->tempo = tempo;
    set_beatscript_tempo((u16)tempo);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022f1c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022f1c] Engine Event 0x03 (Scale the Tempo by factor/256, 60-360)
void func_08022f1c(u32 factor) {
    gMannequinFactory->tempo = clamp_int32((factor * gMannequinFactory->tempo) >> 8, 0x3c, 0x168);
    set_beatscript_tempo(gMannequinFactory->tempo);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022f4c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022f4c] Start slow motion
void func_08022f4c(void) {
    u32 i;

    set_beatscript_tempo(gMannequinFactory->tempo >> 1);
    scene_set_music_pitch(-0xb00);
    gMannequinFactory->slowMotion = TRUE;
    gMannequinFactory->slowTimer = 0;
    gMannequinFactory->slowDuration = ticks_to_frames(0x16);

    for (i = 0; i < 6; i++) {
        if (gMannequinFactory->mannequins[i].state) {
            sprite_set_visible(gSpriteHandler, gMannequinFactory->mannequins[i].dashSprite, FALSE);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08022fb8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08022fb8] Update slow motion, restoring tempo and pitch at the end
void func_08022fb8(void) {
    struct Mannequin *mannequin;
    u32 i;

    if (!gMannequinFactory->slowMotion) return;
    if (++gMannequinFactory->slowTimer <= gMannequinFactory->slowDuration) return;

    set_beatscript_tempo(gMannequinFactory->tempo);
    scene_set_music_pitch(0);
    gMannequinFactory->slowMotion = FALSE;

    for (i = 0; i < 6; i++) {
        mannequin = &gMannequinFactory->mannequins[i];
        if (mannequin->state) {
            sprite_set_anim_cel(gSpriteHandler, mannequin->dashSprite, 0);
            sprite_set_visible(gSpriteHandler, mannequin->dashSprite, TRUE);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_0802303c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0802303c] Game Engine Update
void mannequin_engine_update(void) {
    func_08022fb8();
    func_08022ca0();
    func_080226a0();
    func_080223ac();
    update_drumtech();

    if (gMannequinFactory->gameOver && (D_03004afc & A_BUTTON)) {
        set_pause_beatscript_scene(FALSE);
        gMannequinFactory->gameOver = FALSE;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_0802308c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0802308c] Play a factory sound: low byte picks it, the rest is volume
void func_0802308c(u32 arg) {
    func_0800c604(0);
    if (gMannequinFactory->stopped) return;
    play_sound_w_pitch_volume(D_089df404[arg & 0xff], arg >> 8, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080230cc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080230cc] Play a factory sound now, or after `delay` ticks
void func_080230cc(u32 sound, u32 volume, u32 delay) {
    u32 arg = sound | (volume << 8);

    if (delay != 0) {
        schedule_function_call(get_current_mem_id(), func_0802308c, arg, ticks_to_frames(delay));
    } else {
        func_0802308c(arg);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_0802310c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0802310c] Game over: stop the belt and switch to the game-over script
void func_0802310c(void) {
    gameplay_inputs_enabled(FALSE);
    gMannequinFactory->stopped = TRUE;
    scene_stop_music();
    play_sound(&s_ghost_just_hit_seqData);
    if (gMannequinFactory->gameOverScript != NULL) {
        func_0801d968(gMannequinFactory->gameOverScript);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023150.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023150] Engine Event 0x04 (Set the Game-Over Script)
void func_08023150(const struct Beatscript *script) {
    gMannequinFactory->gameOverScript = script;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023164.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023164] Engine Event 0x05 (Show Game Over, and save the high score)
void func_08023164(void) {
    sprite_create(gSpriteHandler, anim_mannequin_game_over, 0, 0x78, 0x48, 0x800, 1, 0, 0);
    gMannequinFactory->gameOver = TRUE;
    // [D_030046a8 + 0x2A8]: the Mannequin Factory high score.
    D_030046a8->data.unk294[1] = gMannequinFactory->highScore;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080231c8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080231c8] Show the score and the high score, four digits each
void func_080231c8(void) {
    u32 value;
    u32 i;

    value = gMannequinFactory->score;
    for (i = 0; i < 4; i++) {
        sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->scoreSprites[i], (s8)(value % 10));
        value /= 10;
    }

    value = gMannequinFactory->highScore;
    for (i = 0; i < 4; i++) {
        sprite_set_anim_cel(gSpriteHandler, gMannequinFactory->highScoreSprites[i], (s8)(value % 10));
        value /= 10;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_0802327c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0802327c] Init. the scores (shown in every version but 1)
void func_0802327c(void) {
    u32 visible = (gMannequinFactory->version != 1);
    u32 i;

    gMannequinFactory->score = 0;
    gMannequinFactory->highScore = D_030046a8->data.unk294[1];

    for (i = 0; i < 4; i++) {
        gMannequinFactory->scoreSprites[i] = sprite_create(gSpriteHandler, anim_mannequin_score_num, 0, 0x58, 0x90, 0x800, 0, 0, 0);
        sprite_set_x(gSpriteHandler, gMannequinFactory->scoreSprites[i], (s16)(0x58 - (i * 8)));
        sprite_set_visible(gSpriteHandler, gMannequinFactory->scoreSprites[i], visible);
    }

    for (i = 0; i < 4; i++) {
        gMannequinFactory->highScoreSprites[i] = sprite_create(gSpriteHandler, anim_mannequin_high_score_num, 0, 0x60, 0x88, 0x800, 0, 0, 0);
        sprite_set_x(gSpriteHandler, gMannequinFactory->highScoreSprites[i], (s16)(0x60 - (i * 4)));
        sprite_set_visible(gSpriteHandler, gMannequinFactory->highScoreSprites[i], visible);
    }

    func_080231c8();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080233b4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080233b4] Add points (capped at 9999), raising the high score with it
void func_080233b4(u32 points) {
    if (gMannequinFactory->version == 1) return;

    gMannequinFactory->score += points;
    if (gMannequinFactory->score > 9999) gMannequinFactory->score = 9999;
    if (gMannequinFactory->score > gMannequinFactory->highScore) {
        gMannequinFactory->highScore = gMannequinFactory->score;
    }
    func_080231c8();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023400.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023400] Game Engine Stop
void mannequin_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023404.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023404] Cue - Spawn
void mannequin_cue_spawn(struct Cue *cue, struct MannequinFactoryCue *info, u32 type) {
    info->mannequin = gMannequinFactory->current;
    info->type = type;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023418.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023418] Cue - Update
u32 mannequin_cue_update(struct Cue *cue, struct MannequinFactoryCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x18)) ? TRUE : FALSE;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023434.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023434] Cue - Despawn
void mannequin_cue_despawn(struct Cue *cue, struct MannequinFactoryCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023438.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023438] Cue - Hit
void mannequin_cue_hit(struct Cue *cue, struct MannequinFactoryCue *info, u32 pressed, u32 released) {
    if (gMannequinFactory->stopped) return;

    func_08022ce8(info->type);

    switch (info->type) {
        case 0:
            // Finished: send it off.
            func_080223d0(info->mannequin->kind, gameplay_get_last_hit_offset());
            func_08022894(info->mannequin);
            play_sound_in_player_w_pitch_volume(7, &s_SD5_seqData, 0x140, 0);
            func_080233b4(1);
            break;
        case 1:
            func_08022b0c(info->mannequin, 1);
            play_sound(&s_SD4_seqData);
            break;
        case 2:
            func_080229f0(info->mannequin);
            func_08022a7c(info->mannequin, 1);
            play_sound(&s_HC_seqData);
            break;
        case 3:
            func_080229f0(info->mannequin);
            func_08022a7c(info->mannequin, -1);
            play_sound_in_player_w_pitch_volume(6, &s_HC_seqData, 0x140, 0);
            func_080233b4(1);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_080234f4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080234f4] Cue - Barely
void mannequin_cue_barely(struct Cue *cue, struct MannequinFactoryCue *info, u32 pressed, u32 released) {
    mannequin_cue_hit(cue, info, pressed, released);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023500.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023500] Cue - Miss
void mannequin_cue_miss(struct Cue *cue, struct MannequinFactoryCue *info) {
    if (gMannequinFactory->stopped) return;

    func_080229f0(info->mannequin);
    if (info->type == 5) {
        gameplay_ignore_this_cue_result();
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023530.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023530] Input Event
void mannequin_input_event(u32 pressed, u32 released) {
    if (pressed & A_BUTTON) func_08022ce8(0);
    if (pressed & B_BUTTON) func_08022ce8(1);
    if (pressed & DPAD_LEFT) func_08022ce8(3);
    if (pressed & DPAD_RIGHT) func_08022ce8(2);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023574.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023574] Common Event 0 (Beat Animation)
void mannequin_common_beat_animation(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_08023578.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08023578] Common Event 1 (Display Text)
void mannequin_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/mannequin_factory/asm_0802357c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0802357c] Common Event 2 (Init. Tutorial)
void mannequin_common_init_tutorial(void) {
}
#endif
