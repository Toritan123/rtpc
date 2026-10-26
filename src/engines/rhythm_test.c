#include "engines/rhythm_test.h"

#include "src/scenes/gameplay.h"
#include "src/text_printer.h"
#include "src/code_08001360.h"
#include "src/lib_0804ca80.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gRhythmTest ((struct RhythmTestEngineData *)gCurrentEngineData)


/* RHYTHM TEST */


#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033960.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033960] Engine Event 05 (Play DrumTech Sequence)
void func_08033960(u32 index) {
    play_drumtech_seq(rhythm_test_trick_drum_seq[index], 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_0803397c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803397c] Reset the timing chart
void func_0803397c(void) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;

    rhythmTest->markerCount = 0;
    rhythmTest->chartY = 0x88;
    rhythmTest->chartEnabled = FALSE;
    gRhythmTest->scrollTarget = D_03004b10.BG_OFS[BG_LAYER_2].y;
    gRhythmTest->scrollSpeed = -1;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080339bc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080339bc] Plot one input on the timing chart
void func_080339bc(s32 offset) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;
    s16 x, marker, line;
    s32 delta;

    if (!rhythmTest->chartEnabled) return;
    if (rhythmTest->markerCount > 0x1f) return;

    offset = clamp_int32(offset, -0x30, 0x30);
    x = offset + 0x78;

    marker = sprite_create(gSpriteHandler, anim_rhythm_test_chart_marker, 0, x, rhythmTest->chartY,
                           0x8800, 0, 0, 0);
    sprite_set_origin_x_y(gSpriteHandler, marker,
                          &D_03004b10.BG_OFS[BG_LAYER_2].x, &D_03004b10.BG_OFS[BG_LAYER_2].y);
    rhythmTest->markerSprites[rhythmTest->markerCount] = marker;

    // Every marker after the first also gets a line back to its predecessor.
    // The line's cel is the distance between the two, and it is mirrored and
    // shifted when this input landed earlier than the last one.
    if (rhythmTest->markerCount != 0) {
        line = sprite_create(gSpriteHandler, anim_rhythm_test_chart_line, 0, x, rhythmTest->chartY,
                             0x8801, 0, 0x17, 0);
        sprite_set_origin_x_y(gSpriteHandler, line,
                              &D_03004b10.BG_OFS[BG_LAYER_2].x, &D_03004b10.BG_OFS[BG_LAYER_2].y);

        delta = rhythmTest->lastOffset - offset;
        sprite_set_anim_cel(gSpriteHandler, line, (s8)((delta < 0) ? -delta : delta));
        if (delta < 0) {
            sprite_attr_set(gSpriteHandler, line, 0x1000);
            sprite_set_x(gSpriteHandler, line, offset + 0x79);
        }
        rhythmTest->lineSprites[rhythmTest->markerCount - 1] = line;
    }

    rhythmTest->lastOffset = offset;
    rhythmTest->chartY -= 4;
    rhythmTest->markerCount++;
    func_08033b34(4);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033b24.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033b24] Engine Event 01 (Enable/Disable the Chart)
void func_08033b24(u32 enabled) {
    gRhythmTest->chartEnabled = enabled;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033b34.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033b34] Engine Event 02 (Scroll the Chart Up)
void func_08033b34(s32 amount) {
    gRhythmTest->scrollTarget -= amount;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033b48.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033b48] Step BG layer 2 toward scrollTarget
void func_08033b48(void) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;
    s16 scrollY = D_03004b10.BG_OFS[BG_LAYER_2].y;
    s16 speed = rhythmTest->scrollSpeed;
    s16 target = rhythmTest->scrollTarget;

    if ((speed < 0) ? (scrollY > target) : (scrollY < target)) {
        scrollY += speed;
    }
    D_03004b10.BG_OFS[BG_LAYER_2].y = scrollY;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033b9c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033b9c] Engine Event 03 (Bring the Chart Forward)
void func_08033b9c(void) {
    struct RhythmTestEngineData *rhythmTest;
    s32 i;

    scene_set_bg_layer_priority(BG_LAYER_2, 0);

    for (i = 0; i < (s32)(gRhythmTest->markerCount - 1); i++) {
        sprite_set_z(gSpriteHandler, gRhythmTest->markerSprites[i], 0x800);
        sprite_set_z(gSpriteHandler, gRhythmTest->lineSprites[i], 0x801);
    }
    sprite_set_z(gSpriteHandler, gRhythmTest->markerSprites[i], 0x800);

    rhythmTest = gRhythmTest;
    rhythmTest->scrollTarget = -2;
    rhythmTest->scrollSpeed = 2;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033c2c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033c2c] Graphics Init. 3
void rhythm_test_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
    func_080041d0(0x4b, 0x5e, 0x5e);
    func_0800425c(4, 0x50);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033c50.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033c50] Graphics Init. 2
void rhythm_test_init_gfx2(void) {
    u32 temp;

    func_0800c604(0);
    temp = func_08002ee0(get_current_mem_id(), rhythm_test_gfx_table, 0x2000);
    run_func_after_task(temp, &rhythm_test_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033c80.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033c80] Graphics Init. 1
void rhythm_test_init_gfx1(void) {
    u32 temp;

    func_0800c604(0);
    temp = start_new_texture_loader(get_current_mem_id(), rhythm_test_buffered_textures);
    run_func_after_task(temp, &rhythm_test_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033cac.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033cac] Game Engine Start
void rhythm_test_engine_start(u32 version) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;
    struct TextPrinter *textPrinter;

    rhythmTest->version = version;

    rhythm_test_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 0x1e, 1);
    scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0x18, 0, 0x1c, 0x8002);
    scene_set_bg_layer_display(BG_LAYER_3, TRUE, 0, 0, 0, 0x1f, 3);

    textPrinter = text_printer_create_new(get_current_mem_id(), 2, 0xf0, 0x1e);
    text_printer_set_x_y(textPrinter, 0, 0x10);
    text_printer_center_by_content(textPrinter, TRUE);
    gameplay_set_text_printer(textPrinter);
    gameplay_set_text_z(0x48c8);

    rhythmTest->buttonSprite = sprite_create(gSpriteHandler, anim_rhythm_test_button,
                                             0, 120, 100, 0x4800, 1, 0x7f, 0);
    rhythmTest->monitorNoteSprite = sprite_create(gSpriteHandler, anim_rhythm_test_monitor_note,
                                                  0x7f, 120, 60, 0x4864, 1, 0x7f, 0x8000);
    rhythmTest->monitorNoteState = 0;
    rhythmTest->monitorCountSprite = sprite_create(gSpriteHandler, anim_rhythm_test_monitor_count_num,
                                                   10, 120, 60, 0x4800, 0, 0x7f, 0x8000);

    func_0803397c();
    init_drumtech(&gRhythmTest->drumTech);
    gameplay_set_input_buttons(A_BUTTON, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033dfc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033dfc] Engine Event 07 (STUB)
void rhythm_test_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033e00.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033e00] Engine Event 00 (Set the Monitor Display)
void func_08033e00(u32 state) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;
    s16 noteSprite = rhythmTest->monitorNoteSprite;
    struct Animation *anim = NULL;
    s8 startCel = 0;
    s8 direction = 1;

    // Leaving the count readout hides it again.
    if (rhythmTest->monitorNoteState == 5) {
        sprite_set_visible(gSpriteHandler, rhythmTest->monitorCountSprite, FALSE);
    }
    gRhythmTest->monitorNoteState = state;

    switch (state) {
        case 1: anim = anim_rhythm_test_monitor_note; break;
        case 2: anim = anim_rhythm_test_monitor_text2; break;
        case 3: anim = anim_rhythm_test_monitor_text1; direction = 0; break;
        case 4: anim = anim_rhythm_test_monitor_text1; direction = 0; startCel = 1; break;
        case 5:
            anim = anim_rhythm_test_monitor_count_bg;
            startCel = 1;
            sprite_set_visible(gSpriteHandler, gRhythmTest->monitorCountSprite, TRUE);
            break;
        default: break; // 0 and anything above 5 just hide the monitor
    }

    if (anim != NULL) {
        sprite_set_anim(gSpriteHandler, noteSprite, anim, startCel, direction, 0x7f, 0);
        sprite_set_visible(gSpriteHandler, noteSprite, TRUE);
    } else {
        sprite_set_visible(gSpriteHandler, noteSprite, FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033f08.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033f08] Reset the button sprite to its unpressed cel
void func_08033f08(void) {
    sprite_set_anim_cel(gSpriteHandler, gRhythmTest->buttonSprite, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033f28.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033f28] Engine Event 04 (Tick the Count Readout)
void func_08033f28(s32 count) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;

    if (rhythmTest->monitorNoteState != 5) return;

    sprite_set_anim_cel(gSpriteHandler, rhythmTest->monitorCountSprite, (s8)count);
    // Cel 0 while count is zero, cel 1 for every non-zero count.
    sprite_set_anim_cel(gSpriteHandler, gRhythmTest->monitorNoteSprite,
                        (s8)((u32)(-count | count) >> 31));
    play_sound_w_pitch_volume(&s_f_machine_click_seqData, 0x100, (count != 0) ? 0x300 : 0x800);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08033f80.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08033f80] Engine Event 06 (Clear the Chart)
void func_08033f80(void) {
    s32 i;

    scene_set_bg_layer_priority(BG_LAYER_2, 2);
    D_03004b10.BG_OFS[BG_LAYER_2].y = 0x18;

    for (i = 0; i < (s32)(gRhythmTest->markerCount - 1); i++) {
        sprite_delete(gSpriteHandler, gRhythmTest->markerSprites[i]);
        sprite_delete(gSpriteHandler, gRhythmTest->lineSprites[i]);
    }
    sprite_delete(gSpriteHandler, gRhythmTest->markerSprites[i]);

    func_08033e00(0);
    func_0803397c();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034004.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034004] Game Engine Update
void rhythm_test_engine_update(void) {
    func_08033b48();
    update_drumtech();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034014.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034014] Game Engine Stop
void rhythm_test_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034018.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034018] Cue - Spawn
void rhythm_test_cue_spawn(struct Cue *cue, struct RhythmTestCue *info, u32 unused) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_0803401c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803401c] Cue - Update
u32 rhythm_test_cue_update(struct Cue *cue, struct RhythmTestCue *info, u32 runningTime, u32 duration) {
    if (runningTime > ticks_to_frames(0x30)) {
        return TRUE;
    } else {
        return FALSE;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034038.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034038] Cue - Despawn
void rhythm_test_cue_despawn(struct Cue *cue, struct RhythmTestCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_0803403c.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_0803403c] Cue - Hit
void rhythm_test_cue_hit(struct Cue *cue, struct RhythmTestCue *info, u32 pressed, u32 released) {
    s32 offset;

    func_08033f08();
    func_080340a4();
    // Doubled, then jittered by one of {-1, 0, +1} so identical timings do not
    // stack into a single column on the chart.
    offset = (gameplay_get_last_hit_offset() * 2) - 1;
    func_080339bc(offset + agb_random(3));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034068.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034068] Cue - Barely
void rhythm_test_cue_barely(struct Cue *cue, struct RhythmTestCue *info, u32 pressed, u32 released) {
    s32 offset;

    func_08033f08();
    func_080340a4();
    offset = (gameplay_get_last_hit_offset() * 2) - 1;
    func_080339bc(offset + agb_random(3));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_08034094.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08034094] Cue - Miss
void rhythm_test_cue_miss(struct Cue *cue, struct RhythmTestCue *info) {
    // Releasing the script's loop is what lets the test move on.
    beatscript_enable_loops();
    func_080339bc(0x30);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080340a4.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080340a4] Play the tap sound
void func_080340a4(void) {
    play_sound_w_pitch_volume(&s_BD3_seqData, 0xc0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080340b8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080340b8] Input Event
void rhythm_test_input_event(u32 pressed, u32 released) {
    func_08033f08();
    func_080340a4();
    beatscript_enable_loops();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080340cc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080340cc] Common Event 0 (Beat Animation)
void rhythm_test_common_beat_animation(void) {
    struct RhythmTestEngineData *rhythmTest = gRhythmTest;

    if ((rhythmTest->monitorNoteState >= 1) && (rhythmTest->monitorNoteState <= 2)) {
        sprite_set_anim_cel(gSpriteHandler, rhythmTest->monitorNoteSprite, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080340f8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080340f8] Common Event 1 (Display Text, Unimplemented)
void rhythm_test_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/rhythm_test/asm_080340fc.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080340fc] Common Event 2 (Init. Tutorial, Unimplemented)
void rhythm_test_common_init_tutorial(void) {
}
#endif
