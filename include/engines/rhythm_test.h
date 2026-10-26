#pragma once

#include "global.h"
#include "engines.h"
#include "engines/night_walk.h" // struct DrumTechController

#include "games/rhythm_test/graphics/rhythm_test_graphics.h"

// Engine Types:
// Field offsets recovered from the engine's own assembly (asm/engines/rhythm_test/).
// drumTech is 0x350 bytes on the GBA, so sizeof() is still 0x3e8 there; on a
// 64-bit host it grows with its four pointers, which is what we want.
struct RhythmTestEngineData {
    u32 version;                // 0x00
    s16 buttonSprite;           // 0x04  Sprite: the button being tapped
    s16 monitorNoteSprite;      // 0x06  Sprite: note on the monitor
    u8  monitorNoteState;       // 0x08  Beat animation runs while this is 1..2
    u8  unk09;                  // 0x09
    s16 monitorCountSprite;     // 0x0A  Sprite: count readout
    u16 markerCount;            // 0x0C  Markers plotted so far (stops at 0x1f)
    s16 markerSprites[32];      // 0x0E  Sprite: one chart marker per input
    s16 lineSprites[31];        // 0x4E  Sprite: line joining marker n-1 to n
    s16 lastOffset;             // 0x8C  Previous marker's timing offset
    s16 chartY;                 // 0x8E  Next marker's Y; walks up by 4 each time
    u8  chartEnabled;           // 0x90  Markers are only plotted while set
    u8  unk91;                  // 0x91
    s16 scrollTarget;           // 0x92  Target BG_OFS[2].y
    s16 scrollSpeed;            // 0x94  Per-frame step toward scrollTarget
    u8  unk96[2];               // 0x96
    struct DrumTechController drumTech; // 0x98
};

struct RhythmTestCue {
    /* add fields here */
};


// Engine Definition Data:
extern struct DrumTechNote *rhythm_test_trick_drum_seq[];
extern struct CompressedData *rhythm_test_buffered_textures[];
extern struct GraphicsTable rhythm_test_gfx_table[];


// Functions:
extern void func_08033960(u32 index); // Engine Event 05 (Play DrumTech Sequence)
extern void func_0803397c(void); // Reset the timing chart
extern void func_080339bc(s32 offset); // Plot one input on the timing chart
extern void func_08033b24(u32 enabled); // Engine Event 01 (Enable/Disable the Chart)
extern void func_08033b34(s32 amount); // Engine Event 02 (Scroll the Chart Up)
extern void func_08033b48(void); // Step BG layer 2 toward scrollTarget
extern void func_08033b9c(void); // Engine Event 03 (Bring the Chart Forward)
extern void rhythm_test_init_gfx3(void); // Graphics Init. 3
extern void rhythm_test_init_gfx2(void); // Graphics Init. 2
extern void rhythm_test_init_gfx1(void); // Graphics Init. 1
extern void rhythm_test_engine_start(u32 version); // Game Engine Start
extern void rhythm_test_engine_event_stub(void); // Engine Event 07 (STUB)
extern void func_08033e00(u32 state); // Engine Event 00 (Set the Monitor Display)
extern void func_08033f08(void); // Reset the button sprite to its unpressed cel
extern void func_08033f28(s32 count); // Engine Event 04 (Tick the Count Readout)
extern void func_08033f80(void); // Engine Event 06 (Clear the Chart)
extern void rhythm_test_engine_update(void); // Game Engine Update
extern void rhythm_test_engine_stop(void); // Game Engine Stop
extern void rhythm_test_cue_spawn(struct Cue *, struct RhythmTestCue *, u32 unused); // Cue - Spawn
extern u32  rhythm_test_cue_update(struct Cue *, struct RhythmTestCue *, u32 runningTime, u32 duration); // Cue - Update
extern void rhythm_test_cue_despawn(struct Cue *, struct RhythmTestCue *); // Cue - Despawn
extern void rhythm_test_cue_hit(struct Cue *, struct RhythmTestCue *, u32 pressed, u32 released); // Cue - Hit
extern void rhythm_test_cue_barely(struct Cue *, struct RhythmTestCue *, u32 pressed, u32 released); // Cue - Barely
extern void rhythm_test_cue_miss(struct Cue *, struct RhythmTestCue *); // Cue - Miss
extern void func_080340a4(void); // Play the tap sound
extern void rhythm_test_input_event(u32 pressed, u32 released); // Input Event
extern void rhythm_test_common_beat_animation(void); // Common Event 0 (Beat Animation)
extern void rhythm_test_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
extern void rhythm_test_common_init_tutorial(void); // Common Event 2 (Init. Tutorial, Unimplemented)
