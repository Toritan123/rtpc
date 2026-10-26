#include "engines/samurai_slice.h"
#include "src/scenes/gameplay.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gSamuraiSlice ((struct SamuraiSliceEngineData *)gCurrentEngineData)


/* SAMURAI SLICE */


#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030c48.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030c48] Graphics Init. 3
void samurai_slice_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030c58.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030c58] Graphics Init. 2
void samurai_slice_init_gfx2(void) {
    u32 temp;

    func_0800c604(0);
    temp = func_08002ee0(get_current_mem_id(), samurai_slice_gfx_table, 0x2000);
    run_func_after_task(temp, &samurai_slice_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030c88.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030c88] Graphics Init. 1
void samurai_slice_init_gfx1(void) {
    u32 temp;

    func_0800c604(0);
    temp = start_new_texture_loader(get_current_mem_id(), samurai_slice_buffered_textures);
    run_func_after_task(temp, &samurai_slice_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030cb4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030cb4] Game Engine Start
void samurai_slice_engine_start(u32 version) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;
    struct PrintedTextAnim *text;
    u32 i;

    samuraiSlice->version = version;

    samurai_slice_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1c, 0x8000);
    scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 0x1e, 0x8001);
    scene_set_bg_layer_display(BG_LAYER_3, TRUE, 0, 0, 0, 0x1b, 2);

    samuraiSlice->objFont = scene_create_obj_font_printer(0x340, 2);
    text = bmp_font_obj_print_c(samuraiSlice->objFont, D_0805a5d0, 0, 0);
    gSamuraiSlice->textSprite = sprite_create(gSpriteHandler, (struct Animation *)text,
                                              0, 0x3c, 0x38, 0, 0, 0, 0);
    gameplay_set_input_buttons(A_BUTTON, 0);

    gSamuraiSlice->samuraiSprite = sprite_create(gSpriteHandler, anim_samurai_beat_1,
                                                 1, 0xe, 0x7b, 0xa, 0, 0, 0);
    gSamuraiSlice->unk00E = 0;

    gSamuraiSlice->flamesSprite = sprite_create(gSpriteHandler, anim_samurai_flames,
                                                0, 0x14, 0x78, 0x14, 1, 0, 0);
    sprite_set_visible(gSpriteHandler, gSamuraiSlice->flamesSprite, FALSE);
    gSamuraiSlice->flamesY = 0x8000;

    for (i = 0; i < 2; i++) {
        func_080319b4(&gSamuraiSlice->demons[i]);
    }
    func_08032228();

    gSamuraiSlice->sliceEffectSprite = sprite_create(gSpriteHandler, anim_samurai_slice_effect,
                                                     0, 0x4a, 0x60, 2, 1, 0, 2);
    sprite_set_visible(gSpriteHandler, gSamuraiSlice->sliceEffectSprite, FALSE);

    for (i = 0; i < 10; i++) {
        func_080324b8(&gSamuraiSlice->medDemons[i]);
    }

    gSamuraiSlice->variant = 0;
    gSamuraiSlice->introTimer = 0;
    gSamuraiSlice->slicesInARow = 0;
    gSamuraiSlice->samuraiState = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030e84.s"
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030f00.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030f00] Engine Event 07 (STUB)
void samurai_slice_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030f04.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030f04] Engine Event 01 (Interpolate Music Speed)
void func_08030f04(s32 target) {
    gSamuraiSlice->unk008 = target;
    // The original addresses this as D_030053c0 + 0x190, which is musicVolume:
    // its own comment gives the absolute address D_03005550, and
    // 0x03005550 - 0x030053c0 == 0x190.
    scene_start_integer_interp(1, ticks_to_frames(0xc), &D_030053c0.musicVolume,
                               D_030053c0.musicVolume, target);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030f34.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030f34] Engine Event 06 (Set Samurai State)
void func_08030f34(u32 state) {
    gSamuraiSlice->samuraiState = state;
    gameplay_set_input_buttons(0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08030f54.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08030f54] Game Engine Update
void samurai_slice_engine_update(void) {
    struct SamuraiSliceEngineData *samuraiSlice;
    intptr_t anim;
    s16 timer;

    func_080321c8();
    func_08032478();
    func_080327a4();

    samuraiSlice = gSamuraiSlice;
    if (samuraiSlice->introTimer > 0) {
        timer = --samuraiSlice->introTimer;
        if (timer == 0) {
            // The intro is over: restore normal script speed and put both
            // animations back in step with the tempo.
            set_beatscript_speed(0x100);
            scene_set_music_pitch_env(0);
            sprite_set_anim_speed(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                  (get_beatscript_tempo() << 8) / 0x8c);
            sprite_set_anim_speed(gSpriteHandler, gSamuraiSlice->flamesSprite,
                                  (get_beatscript_tempo() << 8) / 0x8c);
        } else if (timer <= 0x56) {
            scene_show_bg_layer(BG_LAYER_1);
            scene_show_bg_layer(BG_LAYER_2);
            scene_show_bg_layer(BG_LAYER_3);
        }
    }

    switch (gSamuraiSlice->samuraiState) {
        case 1:
            // The flames rise until they leave the top of the screen.
            gSamuraiSlice->flamesY += 0x100;
            if ((gSamuraiSlice->flamesY >> 8) > 0x77) {
                gSamuraiSlice->flamesY = 0x7800;
                sprite_set_visible(gSpriteHandler, gSamuraiSlice->flamesSprite, FALSE);
            }
            sprite_set_y(gSpriteHandler, gSamuraiSlice->flamesSprite,
                         (s16)(gSamuraiSlice->flamesY >> 8));
            break;

        // Each of these winds the samurai's stance back one step, but only
        // from the pose the previous step left him in.
        case 2:
            anim = sprite_get_data(gSpriteHandler, gSamuraiSlice->samuraiSprite, 7);
            if (anim == (intptr_t)anim_samurai_beat_3) {
                sprite_set_anim(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                anim_samurai_beat_2, 0, 0, 0, 0);
            }
            break;

        case 3:
            anim = sprite_get_data(gSpriteHandler, gSamuraiSlice->samuraiSprite, 7);
            if (anim == (intptr_t)anim_samurai_beat_2) {
                sprite_set_anim(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                anim_samurai_beat_1, 0, 0, 0, 0);
            }
            break;

        case 4:
            anim = sprite_get_data(gSpriteHandler, gSamuraiSlice->samuraiSprite, 7);
            if (anim == (intptr_t)anim_samurai_beat_1) {
                sprite_set_anim(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                anim_samurai_finish2, 0, 1, 0x7f, 0);
            }
            break;

        default:
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_0803113c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803113c] Engine Event 05 (Show Text)
void func_0803113c(const char *string) {
    struct PrintedTextAnim *text;

    delete_bmp_font_obj_text_anim(gSamuraiSlice->objFont, gSamuraiSlice->textSprite);
    text = bmp_font_obj_print_c(gSamuraiSlice->objFont, string, 1, 0xc);
    sprite_set_anim(gSpriteHandler, gSamuraiSlice->textSprite,
                    (struct Animation *)text, 0, 1, 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_0803118c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803118c] Engine Event 09 (Move Samurai)
void func_0803118c(s32 x) {
    sprite_set_x(gSpriteHandler, gSamuraiSlice->samuraiSprite, (s16)(x + 0xe));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080311b4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080311b4] Game Engine Stop
void samurai_slice_engine_stop(void) {
    D_03004b10.BLDMOD = 0;
    D_03004b10.COLEV = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080311c8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080311c8] Cue - Spawn
void samurai_slice_cue_spawn(struct Cue *cue, struct SamuraiSliceCue *info, u32 demonIndex) {
    info->missed = FALSE;
    info->handled = FALSE;
    info->demonIndex = demonIndex;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080311d4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080311d4] Cue - Update
u32 samurai_slice_cue_update(struct Cue *cue, struct SamuraiSliceCue *info, u32 runningTime, u32 duration) {
    struct SamuraiSliceDemon *demon;

    if (runningTime > (u32)ticks_to_frames(0x78)) return TRUE;

    demon = &gSamuraiSlice->demons[info->demonIndex];

    // Once a missed cue is old enough, the demon is let go: it drops away and
    // the scene's window, music and flames are put back to their idle state.
    if (info->handled) return FALSE;
    if (!info->missed) return FALSE;
    if (runningTime < (u32)ticks_to_frames(0xe) + duration) return FALSE;

    info->handled = TRUE;
    demon->state = 4;
    demon->fallVel = -0x600;
    sprite_set_z(gSpriteHandler, demon->sprite, 5);
    if (demon->pattern <= 1) {
        sprite_set_anim_cel(gSpriteHandler, demon->sprite, 0);
    }

    gSamuraiSlice->sliceState = 0;
    gSamuraiSlice->windowWipe = 0;
    D_03004b10.COLEV = 0x1000;
    scene_set_music_track_volume(gSamuraiSlice->unk1E2, 0);
    gSamuraiSlice->slicesInARow = 0;
    sprite_set_visible(gSpriteHandler, gSamuraiSlice->flamesSprite, FALSE);

    return FALSE;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080312b4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080312b4] Cue - Despawn
void samurai_slice_cue_despawn(struct Cue *cue, struct SamuraiSliceCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080312b8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080312b8] Cue - Hit
void samurai_slice_cue_hit(struct Cue *cue, struct SamuraiSliceCue *info, u32 pressed, u32 released) {
    struct SamuraiSliceDemon *demon = &gSamuraiSlice->demons[info->demonIndex];
    struct SongHeader *shout;
    intptr_t anim;
    s32 pitch;

    gSamuraiSlice->slicesInARow++;

    // Which pieces to throw is worked out from the animation on screen, not
    // from the pattern the demon was spawned with.
    anim = sprite_get_data(gSpriteHandler, demon->sprite, 7);
    if      (anim == (intptr_t)anim_small_demon_hop)         demon->pattern = 0;
    else if (anim == (intptr_t)anim_med_demon_hop)           demon->pattern = 1;
    else if (anim == (intptr_t)anim_propeller_demon_hover)   demon->pattern = 2;
    else if (anim == (intptr_t)anim_winged_demon_fly)        demon->pattern = 3;

    switch (demon->pattern) {
        case 0:
            func_08032510(1, demon->x, demon->y);
            func_08032510(0, demon->x, demon->y);
            break;
        case 1:
            func_08032510(2, demon->x, demon->y);
            func_08032510(0, demon->x, demon->y);
            break;
        case 2:
            func_08032510(4, demon->x, demon->y);
            func_08032510(3, demon->x, demon->y);
            func_08032510(0, demon->x, demon->y);
            break;
        case 3:
            func_08032510(0, demon->x, demon->y);
            func_08032510(5, demon->x, demon->y);
            break;
        case 4:
        case 5:
            // The big ones break into two halves plus three scattered bits.
            func_08032510(7, demon->x, demon->y);
            func_08032510(6, demon->x, demon->y);
            func_08032510(0, demon->x - 0x800, demon->y - 0x800);
            func_08032510(0, demon->x, demon->y + 0x800);
            func_08032510(0, demon->x + 0x800, demon->y);
            break;
        default:
            break;
    }

    sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
    sprite_set_visible(gSpriteHandler, demon->shadowSprite, FALSE);
    demon->state = 2;

    shout = (gSamuraiSlice->slicesInARow <= 2) ? &s_sword_ho_seqData
          : (gSamuraiSlice->slicesInARow <= 4) ? &s_sword_hi_seqData
          :                                      &s_sword_orya_seqData;

    // Cutting the last demon of the set drops everything into slow motion.
    if (((gSamuraiSlice->variant == 0) && (demon->pattern == 4)) ||
        ((gSamuraiSlice->variant == 1) && (demon->pattern == 5))) {
        set_beatscript_speed(0x40);
        scene_set_music_pitch_env(-0xc00);
        gSamuraiSlice->introTimer = 0x5a;

        gSamuraiSlice->swordSound = play_sound_in_player(4, shout);
        set_soundplayer_speed(gSamuraiSlice->swordSound, 0x40);
        set_soundplayer_pitch(gSamuraiSlice->swordSound, -0xc00);

        scene_hide_bg_layer(BG_LAYER_1);
        scene_hide_bg_layer(BG_LAYER_2);
        scene_hide_bg_layer(BG_LAYER_3);
    } else {
        gSamuraiSlice->swordSound = play_sound_in_player(4, shout);
        set_soundplayer_speed(gSamuraiSlice->swordSound, 0x100);

        pitch = gSamuraiSlice->slicesInARow << 7;
        if (pitch > 0x300) pitch = 0x300;
        set_soundplayer_pitch(gSamuraiSlice->swordSound, (s16)pitch);
    }

    func_080317f4();

    if (gSamuraiSlice->sliceState == 1) {
        gSamuraiSlice->sliceState = 2;
        sprite_set_anim_cel(gSpriteHandler, gSamuraiSlice->sliceEffectSprite, 0);
        sprite_set_visible(gSpriteHandler, gSamuraiSlice->sliceEffectSprite, TRUE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031588.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031588] Cue - Barely
void samurai_slice_cue_barely(struct Cue *cue, struct SamuraiSliceCue *info, u32 pressed, u32 released) {
    struct SamuraiSliceDemon *demon = &gSamuraiSlice->demons[info->demonIndex];
    intptr_t anim;

    // A graze breaks the streak, so the flame trail goes out.
    gSamuraiSlice->slicesInARow = 0;
    sprite_set_visible(gSpriteHandler, gSamuraiSlice->flamesSprite, FALSE);

    anim = sprite_get_data(gSpriteHandler, demon->sprite, 7);
    if      (anim == (intptr_t)anim_small_demon_hop)        demon->pattern = 0;
    else if (anim == (intptr_t)anim_med_demon_hop)          demon->pattern = 1;
    else if (anim == (intptr_t)anim_propeller_demon_hover)  demon->pattern = 2;
    else if (anim == (intptr_t)anim_winged_demon_fly)       demon->pattern = 3;

    func_080317f4();

    // One piece only, from the "barely" half of the table (entries 8..12).
    switch (demon->pattern) {
        case 0: func_08032510(8,  demon->x, demon->y); break;
        case 1: func_08032510(9,  demon->x, demon->y); break;
        case 2: func_08032510(10, demon->x, demon->y); break;
        case 3: func_08032510(11, demon->x, demon->y); break;
        case 4:
        case 5: func_08032510(12, demon->x, demon->y); break;
        default: break;
    }

    demon->state = 3;
    sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
    sprite_set_visible(gSpriteHandler, demon->shadowSprite, FALSE);

    gSamuraiSlice->sliceState = 0;
    gSamuraiSlice->windowWipe = 0;
    D_03004b10.COLEV = 0x1000;
    scene_set_music_track_volume(gSamuraiSlice->unk1E2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080316e4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080316e4] Cue - Miss
void samurai_slice_cue_miss(struct Cue *cue, struct SamuraiSliceCue *info) {
    info->missed = TRUE;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080316ec.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080316ec] Swing finished: settle into the beat pose
void func_080316ec(void *unused, s16 spriteId) {
    func_0800c604(0);
    // The resting pose matches the swing that was just used, so a streak
    // leaves the samurai standing differently.
    sprite_set_anim(gSpriteHandler, spriteId,
                    samurai_beat_anim[(gSamuraiSlice->slicesInARow > 2)
                                      ? 2 : gSamuraiSlice->slicesInARow],
                    0x7f, 1, 0x7f, 0);
    sprite_set_callback(gSpriteHandler, spriteId, NULL, 0);
    sprite_set_anim_speed(gSpriteHandler, spriteId, 0x100);
    gameplay_set_input_buttons(A_BUTTON, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031770.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031770] Engine Event 08 (Set Streak Pose)
void func_08031770(u32 streak) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;

    samuraiSlice->slicesInARow = streak;
    sprite_set_anim(gSpriteHandler, samuraiSlice->samuraiSprite,
                    samurai_beat_anim[((u16)streak > 2) ? 2 : samuraiSlice->slicesInARow],
                    0x7f, 1, 0x7f, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080317c8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080317c8] Input Event
void samurai_slice_input_event(u32 pressed, u32 released) {
    if (gSamuraiSlice->introTimer == 0) {
        play_sound(&s_furi_seqData);
    }
    func_080317f4();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080317f4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080317f4] Player swing
void func_080317f4(void) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;
    u32 combo = samuraiSlice->slicesInARow;
    s32 value;

    // The first three swings in a row each have their own animation; past
    // that the third one repeats.
    sprite_set_anim(gSpriteHandler, samuraiSlice->samuraiSprite,
                    samurai_slicing_anim[(combo > 2) ? 2 : combo], 1, 1, 2, 4);
    sprite_set_callback(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                        (void *)func_080316ec, 0);
    sprite_set_anim_speed(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                          (get_beatscript_tempo() << 8) / 0x8c);
    gSamuraiSlice->unk00E = 1;
    gameplay_set_input_buttons(0, 0);

    // From the fourth swing on, a flame trail appears; it climbs and changes
    // palette with the streak, both capped.
    if (gSamuraiSlice->slicesInARow > 2) {
        sprite_set_visible(gSpriteHandler, gSamuraiSlice->flamesSprite, TRUE);

        value = gSamuraiSlice->slicesInARow - 3;
        if (value > 5) value = 5;
        sprite_set_base_palette(gSpriteHandler, gSamuraiSlice->flamesSprite, (s8)value);

        value = (gSamuraiSlice->slicesInARow - 3) * 5;
        if (value > 0x1e) value = 0x1e;
        gSamuraiSlice->flamesY = (0x80 - value) << 8;

        sprite_set_y(gSpriteHandler, gSamuraiSlice->flamesSprite,
                     (s16)(gSamuraiSlice->flamesY >> 8));
        sprite_set_anim_speed(gSpriteHandler, gSamuraiSlice->flamesSprite,
                              (get_beatscript_tempo() << 8) / 0x8c);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_0803193c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803193c] Common Event 0 (Beat Animation)
void samurai_slice_common_beat_animation(void) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;
    intptr_t anim;

    if (samuraiSlice->version == 0) return;

    anim = sprite_get_data(gSpriteHandler, samuraiSlice->samuraiSprite, 7);
    if ((anim != (intptr_t)anim_samurai_beat_2) &&
        (anim != (intptr_t)anim_samurai_beat_1) &&
        (anim != (intptr_t)anim_samurai_beat_3)) {
        return;
    }

    sprite_set_anim_cel(gSpriteHandler, gSamuraiSlice->samuraiSprite, 0);
    sprite_set_playback(gSpriteHandler, gSamuraiSlice->samuraiSprite, 1, 0x7f, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080319b0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080319b0] Common Event 1 (Display Text, Unimplemented)
void samurai_slice_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080319b4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080319b4] Init. Large Demon
void func_080319b4(struct SamuraiSliceDemon *demon) {
    demon->sprite = sprite_create(gSpriteHandler, anim_med_demon_hop, 0, 0x78, 0x42, 0x800a, 0, 0, 0);
    demon->shadowSprite = sprite_create(gSpriteHandler, anim_demon_shadow, 0, 0x40, 0x40, 0x8014, 0, 0, 0);
    demon->state = 0;
    demon->x = 0xf000;
    demon->y = 0x2800;

    sprite_set_x_y(gSpriteHandler, demon->sprite, 0xf0, 0x28);
    // The shadow tracks the demon's 16.8 position; >> 8 with the low bits
    // dropped is what the original's shift pair works out to.
    sprite_set_x_y(gSpriteHandler, demon->shadowSprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));
    sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
    sprite_set_visible(gSpriteHandler, demon->shadowSprite, FALSE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031a6c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031a6c] Send a large demon in
void func_08031a6c(struct SamuraiSliceDemon *demon, u32 pattern) {
    demon->state = 1;
    demon->pattern = pattern;
    demon->x = 0xf000;
    demon->y = 0x2800;
    demon->travelX = 0;
    demon->travelY = 0;
    demon->hopHeight = 0;
    demon->timer = 0;
    demon->duration = ticks_to_frames(0xc0);

    // Two sets of demon animations; which one is in use is set by engine
    // event 04.
    if (gSamuraiSlice->variant == 0) {
        sprite_set_anim(gSpriteHandler, demon->sprite, D_089e4928[demon->pattern], 0, 0, 0, 0);
    } else {
        sprite_set_anim(gSpriteHandler, demon->sprite, D_089e4940[demon->pattern], 0, 0, 0, 0);
    }

    // Patterns 2..5 animate themselves; 0 and 1 are stepped by the cel
    // ladder in func_08031c94 instead.
    if ((u8)(demon->pattern - 2) <= 3) {
        sprite_set_playback(gSpriteHandler, demon->sprite, 1, 0, 0);
    }

    // Patterns 4 and 5 are the big ones, and get the bigger shadow.
    if ((u8)(demon->pattern - 4) <= 1) {
        sprite_set_anim(gSpriteHandler, demon->shadowSprite, anim_large_demon_shadow, 0, 0, 0, 0);
    } else {
        sprite_set_anim(gSpriteHandler, demon->shadowSprite, anim_demon_shadow, 0, 0, 0, 0);
    }

    sprite_set_x_y_z(gSpriteHandler, demon->sprite,
                     (s16)(demon->x >> 8), (s16)(demon->y >> 8), 0x800a);
    sprite_set_x_y(gSpriteHandler, demon->shadowSprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));
    sprite_set_visible(gSpriteHandler, demon->sprite, TRUE);
    sprite_set_visible(gSpriteHandler, demon->shadowSprite, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031bc0.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031bc0] Engine Event 02 (Spawn Demon)
void func_08031bc0(u32 pattern) {
    struct SamuraiSliceDemon *demon = gSamuraiSlice->demons;
    struct SoundPlayer *soundPlayer;
    struct SongHeader *phrase;
    u32 isSecond = pattern & 1;

    // Odd patterns use the second demon slot, so a pair can be in flight.
    if (isSecond == 1) demon = &gSamuraiSlice->demons[1];
    func_08031a6c(demon, pattern);

    // The calling phrase only plays for the leading demon, and not at all in
    // version 1.
    if (gSamuraiSlice->version == 1) return;
    if (isSecond != 0) return;

    if (gSamuraiSlice->variant == 0) {
        phrase = &s_iai_frase1a_seqData;
        if (pattern > 1) {
            phrase = &s_iai_frase3a_seqData;
            if (pattern <= 3) phrase = &s_iai_frase2a_seqData;
        }
    } else {
        phrase = &s_iai_frase1b_seqData;
        if (pattern > 1) {
            phrase = &s_iai_frase3b_seqData;
            if (pattern <= 3) phrase = &s_iai_frase2b_seqData;
        }
    }

    soundPlayer = play_sound(phrase);
    set_soundplayer_speed(soundPlayer, (get_beatscript_tempo() << 8) / 0x8c);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031c54.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031c54] Engine Event 04 (Set Phrase Variant)
void func_08031c54(u32 value) {
    gSamuraiSlice->variant = value;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031c68.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031c68] Height of a parabolic hop: peaks at (ticks << 8) halfway
s32 func_08031c68(s32 ticks, s32 t) {
    s32 frames = (u16)ticks_to_frames(ticks);

    return (-(t * 4) * (ticks << 8) * (t - frames)) / (frames * frames);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08031c94.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08031c94] Large demon: hop in from the right
//
// The horizontal march is linear in the demon's own timer; the vertical part
// is a chain of parabolic hops selected by `pattern`, except patterns 2 and 3
// which hover on a sine wave until the final approach.
void func_08031c94(struct SamuraiSliceDemon *demon) {
    s32 t = demon->timer;
    s32 span = t * 27;
    s32 shadowY;
    s32 height;

    demon->travelX = (span << 11) / demon->duration;
    demon->x = 0xf000 - demon->travelX;
    demon->travelY = (span << 9) / demon->duration;
    shadowY = 0x2800 + demon->travelY;

    switch (demon->pattern) {
        case 0:
            // Six hops of 0x18 ticks. The last two both measure from the
            // 0x78 boundary, as in the original, so the sixth arc keeps
            // running past its own end.
            if      (t < ticks_to_frames(0x18)) demon->hopHeight = func_08031c68(0x18, t);
            else if (t < ticks_to_frames(0x30)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x18));
            else if (t < ticks_to_frames(0x48)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x30));
            else if (t < ticks_to_frames(0x60)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x48));
            else if (t < ticks_to_frames(0x78)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x60));
            else if (t < ticks_to_frames(0x90)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x78));
            else if (t < ticks_to_frames(0xa0)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x78));
            else                                demon->hopHeight = 0;
            break;

        case 1:
            if      (t < ticks_to_frames(0x18)) demon->hopHeight = func_08031c68(0x18, t);
            else if (t < ticks_to_frames(0x30)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x18));
            else if (t < ticks_to_frames(0x48)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x30));
            else if (t < ticks_to_frames(0x60)) demon->hopHeight = func_08031c68(0x18, t - ticks_to_frames(0x48));
            else if (t < ticks_to_frames(0x78)) demon->hopHeight = func_08031c68(0x30, t - ticks_to_frames(0x60));
            else if (t < ticks_to_frames(0xa0)) demon->hopHeight = func_08031c68(0x30, t - ticks_to_frames(0x60));
            else                                demon->hopHeight = 0;
            break;

        case 2:
        case 3:
            if ((t >= ticks_to_frames(0x78)) && (t < ticks_to_frames(0xa0))) {
                demon->hopHeight = func_08031c68(0x30, t - ticks_to_frames(0x60));
            } else {
                // Hovering: a sine bob riding on a slow descent.
                height = gSineTable[(((t % ticks_to_frames(0x30)) << 11)
                                     / ticks_to_frames(0x30)) & 0x7ff] << 3;
                demon->hopHeight = height + 0x3000;
                demon->hopHeight = (height + 0x800)
                                 - ((-((t * 5) << 14)) / (demon->duration * 5));
            }
            break;

        case 4:
        case 5:
            if ((t >= ticks_to_frames(0x60)) && (t < ticks_to_frames(0x90))) {
                demon->hopHeight = func_08031c68(0x30, t - ticks_to_frames(0x60));
            } else if ((t >= ticks_to_frames(0x90)) && (t < ticks_to_frames(0xa0))) {
                demon->hopHeight = func_08031c68(0x30, t - ticks_to_frames(0x60));
            } else {
                demon->hopHeight = 0;
            }
            break;

        default:
            break;
    }

    demon->y = shadowY - demon->hopHeight;
    shadowY -= 0x400;

    // The legs are picked from how high the demon currently is.
    if (demon->pattern <= 1) {
        height = demon->hopHeight >> 8;
        if (t > ticks_to_frames(0x78)) {
            // As in the original: the first test already covers everything
            // below -0x38, so the -0x3a and -0x3c cases never run.
            if      (height < -0x38) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 1);
            else if (height < -0x3a) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 2);
            else if (height < -0x3c) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 3);
            else                     sprite_set_anim_cel(gSpriteHandler, demon->sprite, 0);
        } else {
            if      (height <= 1) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 3);
            else if (height <= 3) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 2);
            else if (height <= 5) sprite_set_anim_cel(gSpriteHandler, demon->sprite, 1);
            else                  sprite_set_anim_cel(gSpriteHandler, demon->sprite, 0);
        }
    }

    sprite_set_x_y(gSpriteHandler, demon->sprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));
    if (t > ticks_to_frames(0x78)) shadowY += 0x4000;
    sprite_set_x_y(gSpriteHandler, demon->shadowSprite,
                   (s16)(demon->x >> 8), (s16)(shadowY >> 8));

    if (demon->travelX > 0xd800) demon->state = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032070.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032070] Samurai recovers from being hit
void func_08032070(void *unused, s16 spriteId) {
    func_0800c604(0);
    sprite_set_anim(gSpriteHandler, spriteId, anim_samurai_beat_1, 1, 0, 0, 0);
    sprite_set_callback(gSpriteHandler, spriteId, NULL, 0);
    sprite_set_anim_speed(gSpriteHandler, spriteId, 0x100);
    gameplay_set_input_buttons(A_BUTTON, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080320c8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080320c8] Large demon: sail past the samurai and off the left edge
void func_080320c8(struct SamuraiSliceDemon *demon) {
    intptr_t anim;

    demon->fallVel += 0x30;
    demon->y += demon->fallVel;
    demon->x -= 0x180;

    sprite_set_x_y(gSpriteHandler, demon->sprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));
    sprite_set_x(gSpriteHandler, demon->shadowSprite, (s16)(demon->x >> 8));

    // Reaching the samurai without having been cut is what hurts him.
    if ((demon->x >> 8) <= 0x30) {
        anim = sprite_get_data(gSpriteHandler, gSamuraiSlice->samuraiSprite, 7);
        if (anim != (intptr_t)anim_samurai_hurt) {
            sprite_set_anim(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                            anim_samurai_hurt, 0, 1, 1, 4);
            sprite_set_callback(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                (void *)func_08032070, 0);
            sprite_set_anim_speed(gSpriteHandler, gSamuraiSlice->samuraiSprite,
                                  (get_beatscript_tempo() << 8) / 0x8c);
            play_sound(&s_iai_yarare_seqData);
            gameplay_set_input_buttons(0, 0);
        }
    }

    if ((demon->x >> 8) <= -0x10) {
        sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
        sprite_set_visible(gSpriteHandler, demon->shadowSprite, FALSE);
        demon->state = 0;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080321c8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080321c8] Update both large demons
void func_080321c8(void) {
    struct SamuraiSliceDemon *demon = gSamuraiSlice->demons;
    u32 i;

    for (i = 0; i < 2; i++, demon++) {
        switch (demon->state) {
            case 1: func_08031c94(demon); break;
            case 4: func_080320c8(demon); break;
            default: break;
        }
        demon->timer++;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032228.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032228] Reset Background Scroll
void func_08032228(void) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;

    samuraiSlice->sliceState = 0;
    samuraiSlice->bg1ScrollY = 0;
    samuraiSlice->bg2ScrollY = 0;
    scene_set_bg_layer_pos(BG_LAYER_1, 0, (s16)(samuraiSlice->bg1ScrollY >> 8));
    scene_set_bg_layer_pos(BG_LAYER_2, 0, (s16)(gSamuraiSlice->bg2ScrollY >> 8));

    gSamuraiSlice->windowWipeDone = FALSE;
    gSamuraiSlice->windowWipe = 0;
    D_03004b10.BLDMOD = 0x3846;
    D_03004b10.COLEV = 0x1000;
    gSamuraiSlice->unk1E2 = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032298.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032298] Slice sequence step 1: draw the window open
void func_08032298(void) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;
    s32 step;

    // Slower once the target has been reached once, so it eases in.
    if (samuraiSlice->windowWipeDone == 0) {
        step = (get_beatscript_tempo() * 3 * 8) / 0x8c;
    } else {
        step = (get_beatscript_tempo() << 6) / 0x8c;
    }
    gSamuraiSlice->windowWipe += step;

    samuraiSlice = gSamuraiSlice;
    if (samuraiSlice->windowWipe >= (s32)(samuraiSlice->windowWipeTarget << 8)) {
        samuraiSlice->windowWipe = samuraiSlice->windowWipeTarget << 8;
        samuraiSlice->windowWipeDone = TRUE;
    }

    D_03004b10.COLEV = ((0x10 - (gSamuraiSlice->windowWipe >> 8)) << 8)
                      | (gSamuraiSlice->windowWipe >> 8);
    scene_set_music_track_volume(gSamuraiSlice->unk1E2,
                                 (u16)((3 * (gSamuraiSlice->windowWipe >> 8)) << 2));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032330.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032330] Slice sequence step 2: the dash past, window closing again
void func_08032330(void) {
    struct SamuraiSliceEngineData *samuraiSlice = gSamuraiSlice;
    s32 step;

    // Everything moves at a quarter speed while the intro is still running.
    if (samuraiSlice->introTimer != 0) {
        samuraiSlice->bg1ScrollY += 0x200;
    } else {
        samuraiSlice->bg1ScrollY += 0x800;
    }

    samuraiSlice = gSamuraiSlice;
    if (samuraiSlice->introTimer != 0) {
        samuraiSlice->bg2ScrollY -= 0x200;
    } else {
        samuraiSlice->bg2ScrollY -= 0x800;
    }

    samuraiSlice = gSamuraiSlice;
    if ((samuraiSlice->bg1ScrollY >> 8) > 0xef) samuraiSlice->bg1ScrollY = 0xf000;
    samuraiSlice = gSamuraiSlice;
    if ((samuraiSlice->bg2ScrollY >> 8) <= -0xf0) samuraiSlice->bg2ScrollY = -0xf000;

    step = (get_beatscript_tempo() << 8) / 0x8c;
    samuraiSlice = gSamuraiSlice;
    samuraiSlice->windowWipe -= step;
    if (samuraiSlice->windowWipe <= 0) samuraiSlice->windowWipe = 0;

    scene_set_bg_layer_pos(BG_LAYER_1, 0, (s16)(gSamuraiSlice->bg1ScrollY >> 8));
    scene_set_bg_layer_pos(BG_LAYER_2, 0, (s16)(gSamuraiSlice->bg2ScrollY >> 8));

    D_03004b10.COLEV = ((0x10 - (gSamuraiSlice->windowWipe >> 8)) << 8)
                      | (gSamuraiSlice->windowWipe >> 8);
    scene_set_music_track_volume(gSamuraiSlice->unk1E2,
                                 (u16)((3 * (gSamuraiSlice->windowWipe >> 8)) << 2));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032430.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032430] Engine Event 03 (Start Slice Wipe)
void func_08032430(u32 wipeTarget) {
    struct SamuraiSliceEngineData *samuraiSlice;

    gSamuraiSlice->sliceState = 1;
    gSamuraiSlice->windowWipeTarget = wipeTarget;

    samuraiSlice = gSamuraiSlice;
    samuraiSlice->bg1ScrollY = 0;
    samuraiSlice->bg2ScrollY = 0;
    scene_set_bg_layer_pos(BG_LAYER_1, 0, (s16)(samuraiSlice->bg1ScrollY >> 8));
    scene_set_bg_layer_pos(BG_LAYER_2, 0, (s16)(gSamuraiSlice->bg2ScrollY >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032478.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032478] Update the slice sequence
void func_08032478(void) {
    switch (gSamuraiSlice->sliceState) {
        case 1: func_08032298(); break;
        case 2: func_08032330(); break;
        default: break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080324a4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080324a4] Engine Event 07 (Select Music Tracks)
void func_080324a4(u16 tracks) {
    gSamuraiSlice->unk1E2 = tracks;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080324b8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080324b8] Init. Small Demon
void func_080324b8(struct SamuraiSliceMedDemon *demon) {
    demon->alive = 0;
    demon->affineGroup = scene_affine_group_alloc();
    demon->sprite = sprite_create(gSpriteHandler, anim_med_demon_hop, 0, 0x78, 0x42, 5, 0, 0, 0);
    assign_sprite_affine_param(demon->sprite, demon->affineGroup);
    sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032510.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032510] Throw one demon piece
//
// `type` indexes D_0805a5d4 for the animation and the base velocities; both
// are scaled by the tempo, jittered, and biased by the current streak so a
// longer streak throws the pieces further.
void func_08032510(u32 type, s32 x, s32 y) {
    struct SamuraiSliceMedDemon *demon = gSamuraiSlice->medDemons;
    s32 streak, kick, tempo, jitter, speed;
    u32 i;

    type = (u8)type;

    for (i = 0; demon->alive != 0; i++, demon++) {
        if (i >= 9) return;
    }
    if (i > 9) return;

    demon->alive = TRUE;
    // Nothing passes a type above 7, so in practice the pieces never spin.
    demon->spinning = (type > 7);
    demon->rotation = 0;
    demon->x = x;
    demon->y = y;

    streak = gSamuraiSlice->slicesInARow;

    jitter = (u16)agb_random(0x80);
    speed = (u16)agb_random((u16)((streak * 3) << 5));
    tempo = get_beatscript_tempo();
    demon->xVel = (tempo * (D_0805a5d4[type].xVelBase + jitter
                            + (speed - ((streak * 3) << 4) - 0x40))) / 0x8c;

    switch (type) {
        case 0:                             kick = streak * 0x66;     break;
        case 1: case 2: case 3: case 5:     kick = (-streak) << 6;    break;
        case 4:                             kick = ((-streak) * 5) << 4; break;
        default:                            kick = 0;                 break;
    }

    jitter = (u16)agb_random(0x100);
    tempo = get_beatscript_tempo();
    demon->yVel = (tempo * (D_0805a5d4[type].yVelBase + jitter + (kick - 0x80))) / 0x8c;

    demon->spinSpeed = (get_beatscript_tempo() * 8) / 0x8c;
    demon->gravity = ((get_beatscript_tempo() * 3) << 4) / 0x8c;

    sprite_set_anim(gSpriteHandler, demon->sprite, D_0805a5d4[type].anim, 0, 0, 0, 0);
    sprite_set_x_y(gSpriteHandler, demon->sprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));
    set_affine_scale_rotation(demon->affineGroup, 0x100, (s8)demon->rotation);
    sprite_set_visible(gSpriteHandler, demon->sprite, TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_08032708.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08032708] Small demon: fly out, spin, and despawn off screen
void func_08032708(struct SamuraiSliceMedDemon *demon) {
    s32 vx, vy;

    demon->yVel += demon->gravity;
    vy = demon->yVel;
    vx = demon->xVel;

    // The pieces drift at a quarter speed while the intro is still running.
    if (gSamuraiSlice->introTimer != 0) {
        vx /= 4;
        vy /= 4;
    }
    demon->x += vx;
    demon->y += vy;
    sprite_set_x_y(gSpriteHandler, demon->sprite,
                   (s16)(demon->x >> 8), (s16)(demon->y >> 8));

    if (demon->spinning) {
        demon->rotation += demon->spinSpeed;
        set_affine_scale_rotation(demon->affineGroup, 0x100, (s8)demon->rotation);
    }

    // Unsigned on purpose, as in the original: off the top of the screen
    // wraps to a large value and despawns too.
    if ((u32)((demon->y >> 8) + 0x2f) > 0xee) {
        demon->alive = FALSE;
        sprite_set_visible(gSpriteHandler, demon->sprite, FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/samurai_slice/asm_080327a4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080327a4] Update the ten small demons
void func_080327a4(void) {
    struct SamuraiSliceMedDemon *demon = gSamuraiSlice->medDemons;
    u32 i;

    for (i = 0; i < 10; i++, demon++) {
        if (demon->alive == 1) {
            func_08032708(demon);
        }
    }
}
#endif
