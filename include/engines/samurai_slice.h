#pragma once

#include "global.h"
#include "engines.h"

#include "games/samurai_slice/graphics/samurai_slice_graphics.h"

// Engine Macros/Enums:
enum SamuraiSliceVersionsEnum {
    SAMURAI_SLICE_VER_0,
    SAMURAI_SLICE_VER_REMIX
};


// Engine Types:
// One of the two large demons that walk in with a shadow. 0x34 bytes.
struct SamuraiSliceDemon {
    s16 sprite;             // 0x00
    s16 shadowSprite;       // 0x02
    u8  state;              // 0x04
    u8  unk05[3];
    s32 x;                  // 0x08  16.8 fixed point
    s32 y;                  // 0x0C  16.8 fixed point
    u8  unk10[8];           // 0x10
    s32 travelX;            // 0x18  distance covered so far
    s32 travelY;            // 0x1C
    s32 hopHeight;          // 0x20  16.8 fixed point, above the walk line
    s32 timer;              // 0x24  incremented every frame
    s32 duration;           // 0x28  frames the approach is meant to take
    s32 fallVel;            // 0x2C
    u8  pattern;            // 0x30  which hop pattern this demon uses
    u8  unk31[3];
};

// One of the ten small demons, each with its own affine group. 0x20 bytes.
struct SamuraiSliceMedDemon {
    s16 sprite;             // 0x00
    s8  affineGroup;        // 0x02
    u8  unk03;
    s32 x;                  // 0x04  16.8 fixed point
    s32 y;                  // 0x08  16.8 fixed point
    s32 xVel;               // 0x0C
    s32 yVel;               // 0x10
    s32 spinSpeed;          // 0x14
    s32 gravity;            // 0x18
    u8  alive;              // 0x1C
    u8  rotation;           // 0x1D
    u8  spinning;           // 0x1E
    u8  unk1F;
};

// Field offsets recovered from the engine's own assembly
// (asm/engines/samurai_slice/). sizeof() is 0x1e4 on the GBA, as the
// placeholder this replaces asserted; on a 64-bit host it grows by the one
// pointer at the front, which shifts every offset below it by four -- so do
// not reuse the raw numbers in scripts or tables.
struct SamuraiSliceEngineData {
    struct BitmapFontOBJ *objFont;              // 0x000
    u8  version;                                // 0x004
    u8  unk005[3];
    u32 unk008;                                 // 0x008
    s16 samuraiSprite;                          // 0x00C
    u8  unk00E;                                 // 0x00E
    u8  unk00F;
    struct SamuraiSliceDemon demons[2];         // 0x010
    u8  sliceState;                             // 0x078
    u8  unk079[3];
    s32 bg1ScrollY;                             // 0x07C  16.8 fixed point
    s32 bg2ScrollY;                             // 0x080  16.8 fixed point
    s32 windowWipe;                             // 0x084  16.8 fixed point
    u8  windowWipeTarget;                       // 0x088
    u8  windowWipeDone;                         // 0x089
    s16 textSprite;                             // 0x08A
    s16 sliceEffectSprite;                      // 0x08C
    u8  unk08E[2];
    struct SamuraiSliceMedDemon medDemons[10];  // 0x090
    u8  variant;                                // 0x1D0  selects the a/b sets
    u8  unk1D1;
    s16 introTimer;                             // 0x1D2  counts down over the intro
    struct SoundPlayer *swordSound;              // 0x1D4
    s16 slicesInARow;                           // 0x1D8  drives the flame trail
    s16 flamesSprite;                           // 0x1DA
    u32 flamesY;                                // 0x1DC  16.8 fixed point
    u8  samuraiState;                           // 0x1E0  drives the stance machine
    u8  unk1E1;
    s16 unk1E2;                                 // 0x1E2
};

struct SamuraiSliceCue {
    u8 missed;      // 0x00  set by cue_miss
    u8 handled;     // 0x01  set once cue_update has released the demon
    u8 demonIndex;  // 0x02  which of the two large demons this cue belongs to
};

struct SamuraiSlice_0805a5d4 {
    struct Animation *anim;
    s32 xVelBase;   // both are scaled by the tempo and jittered before use
    s32 yVelBase;
};


// Engine Data:
extern const char D_0805a5d0[];
extern const struct SamuraiSlice_0805a5d4 D_0805a5d4[];


// Engine Definition Data:
extern struct CompressedData *samurai_slice_buffered_textures[];
extern struct GraphicsTable samurai_slice_gfx_table[];
extern struct Animation *samurai_slicing_anim[];
extern struct Animation *samurai_beat_anim[];
extern struct Animation *D_089e4928[];
extern struct Animation *D_089e4940[];


// Functions:
extern void samurai_slice_init_gfx3(void); // Graphics Init. 3
extern void samurai_slice_init_gfx2(void); // Graphics Init. 2
extern void samurai_slice_init_gfx1(void); // Graphics Init. 1
extern void samurai_slice_engine_start(u32 version); // Game Engine Start
// extern ? func_08030e84(?);
extern void samurai_slice_engine_event_stub(void); // Engine Event 00 (STUB)
extern void func_08030f04(s32 target); // Engine Event 01 (Interpolate Music Speed)
extern void func_08030f34(u32 state); // Engine Event 06 (Set Samurai State)
extern void samurai_slice_engine_update(void); // Game Engine Update
extern void func_0803113c(const char *string); // Engine Event 05 (Show Text)
extern void func_0803118c(s32 x); // Engine Event 09 (Move Samurai)
extern void samurai_slice_engine_stop(void); // Game Engine Stop
extern void samurai_slice_cue_spawn(struct Cue *, struct SamuraiSliceCue *, u32 isSecondSlice); // Cue - Spawn
extern u32 samurai_slice_cue_update(struct Cue *, struct SamuraiSliceCue *, u32 runningTime, u32 duration); // Cue - Update
extern void samurai_slice_cue_despawn(struct Cue *, struct SamuraiSliceCue *); // Cue - Despawn
extern void samurai_slice_cue_hit(struct Cue *, struct SamuraiSliceCue *, u32 pressed, u32 released); // Cue - Hit
extern void samurai_slice_cue_barely(struct Cue *, struct SamuraiSliceCue *, u32 pressed, u32 released); // Cue - Barely
extern void samurai_slice_cue_miss(struct Cue *, struct SamuraiSliceCue *); // Cue - Miss
extern void func_080316ec(void *unused, s16 spriteId); // Swing finished: settle into the beat pose
extern void func_08031770(u32 streak); // Engine Event 08 (Set Streak Pose)
extern void samurai_slice_input_event(u32 pressed, u32 released); // Input Event
extern void func_080317f4(void); // Player swing
extern void samurai_slice_common_beat_animation(); // Common Event 0 (Beat Animation)
extern void samurai_slice_common_display_text(); // Common Event 1 (Display Text, Unimplemented)
extern void func_080319b4(struct SamuraiSliceDemon *demon); // Init. Large Demon
extern void func_08031a6c(struct SamuraiSliceDemon *demon, u32 pattern); // Send a large demon in
extern void func_08031bc0(u32 pattern); // Engine Event 02 (Spawn Demon)
extern void func_08031c54(u32 value); // Engine Event 04 (Set Phrase Variant)
extern s32 func_08031c68(s32 ticks, s32 t); // Parabolic hop height
extern void func_08031c94(struct SamuraiSliceDemon *demon); // Large demon: approach
extern void func_08032070(void *unused, s16 spriteId); // Samurai recovers from being hit
extern void func_080320c8(struct SamuraiSliceDemon *demon); // Large demon: fall away
extern void func_080321c8(void); // Update both large demons
extern void func_08032228(void); // Reset Background Scroll
extern void func_08032298(void); // Slice sequence step 1
extern void func_08032330(void); // Slice sequence step 2
extern void func_08032430(u32 wipeTarget); // Engine Event 03 (Start Slice Wipe)
extern void func_08032478(void); // Update the slice sequence
extern void func_080324a4(u16 tracks); // Engine Event 07 (Select Music Tracks)
extern void func_080324b8(struct SamuraiSliceMedDemon *demon); // Init. Small Demon
extern void func_08032510(u32 type, s32 x, s32 y); // Throw one demon piece
extern void func_08032708(struct SamuraiSliceMedDemon *demon); // Small demon update
extern void func_080327a4(void); // Update the ten small demons
