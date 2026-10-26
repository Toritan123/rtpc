#pragma once

#include "global.h"
#include "engines.h"
#include "engines/night_walk.h"

#include "games/toss_boys/graphics/toss_boys_graphics.h"

// Engine Macros/Enums:
enum TossBoysVersionsEnum {
    ENGINE_VER_TOSS_BOYS,
    ENGINE_VER_TOSS_REMIX_5,
    ENGINE_VER_TOSS_BOYS_2
};

enum TossBoysAnimationsEnum {
    TOSS_BOYS_ANIM_BEAT_RED,
    TOSS_BOYS_ANIM_BEAT_BLUE,
    TOSS_BOYS_ANIM_BEAT_YELLOW,
    TOSS_BOYS_ANIM_PASS_RED,
    TOSS_BOYS_ANIM_PASS_BLUE,
    TOSS_BOYS_ANIM_PASS_YELLOW,
    TOSS_BOYS_ANIM_DISPENSER,
    TOSS_BOYS_ANIM_BALL1,
    TOSS_BOYS_ANIM_BALL3,
    TOSS_BOYS_ANIM_BALL2,
    TOSS_BOYS_ANIM_BALL_WOBBLE,
    TOSS_BOYS_ANIM_MISS_RED,
    TOSS_BOYS_ANIM_MISS_BLUE,
    TOSS_BOYS_ANIM_MISS_YELLOW,
    TOSS_BOYS_ANIM_SUPER_PASS_RED,
    TOSS_BOYS_ANIM_SUPER_PASS_BLUE,
    TOSS_BOYS_ANIM_SUPER_PASS_YELLOW,
    TOSS_BOYS_ANIM_POP_EFFECT,
    TOSS_BOYS_ANIM_SUPER_BEAT_RED,
    TOSS_BOYS_ANIM_SUPER_BEAT_BLUE,
    TOSS_BOYS_ANIM_SUPER_BEAT_YELLOW,
    TOSS_BOYS_ANIM_CATCH_RED,
    TOSS_BOYS_ANIM_CATCH_BLUE,
    TOSS_BOYS_ANIM_CATCH_YELLOW,
    TOSS_BOYS_ANIM_READY_RED,
    TOSS_BOYS_ANIM_READY_BLUE,
    TOSS_BOYS_ANIM_READY_YELLOW,
    TOSS_BOYS_ANIM_POP_RED,
    TOSS_BOYS_ANIM_POP_BLUE,
    TOSS_BOYS_ANIM_POP_YELLOW,
    TOSS_BOYS_ANIM_BARELY_RED,
    TOSS_BOYS_ANIM_BARELY_BLUE,
    TOSS_BOYS_ANIM_BARELY_YELLOW,
    TOSS_BOYS_ANIM_ARROW_RED,
    TOSS_BOYS_ANIM_ARROW_BLUE,
    TOSS_BOYS_ANIM_ARROW_YELLOW
};

enum TossBoyActionsEnum {
    TOSS_BOY_ACTION_PASS,
    TOSS_BOY_ACTION_SUPER_PASS,
    TOSS_BOY_ACTION_CATCH,
    TOSS_BOY_ACTION_POP
};


// Engine Types:
struct TossBoysEngineData {
    u8 version;                          // 0x000
    struct DrumTechController drumTech;  // 0x004
    s16 boys[3];                         // 0x354 (R, B, Y)
    u8 animTimer[3];                     // 0x35A
    u8 boyAction[3];                     // 0x35D (TossBoyActionsEnum)
    u8 barelyTimer[3];                   // 0x360
    s16 dispenser;                       // 0x364
    s16 ball;                            // 0x366
    s8 ballAffine;                       // 0x368
    u16 ballRotation;                    // 0x36A
    s16 ballRotationSpeed;               // 0x36C
    u16 ballSquash;                      // 0x36E
    u8 ballTarget;                       // 0x370 (3 = the dispenser)
    u8 ballSource;                       // 0x371
    u16 ballTravelTicks;                 // 0x372
    u16 ballArcHeight;                   // 0x374
    s32 ballMotionTask;                  // 0x378
    u8 ballActive;                       // 0x37C
    u8 ballWobbling;                     // 0x37D
    u8 wobblePhase;                      // 0x37E
    u16 inputButtons;                    // 0x380
    u32 inputLockTimers[3];              // 0x384
    u16 nextDrumNote;                    // 0x390
    s16 pitchBend;                       // 0x392
    u16 pitchPhase;                      // 0x394
    u16 popTimer;                        // 0x396
    u8 popRecovered;                     // 0x398
    u16 flashLevel;                      // 0x39A
    s16 soshi;                           // 0x39C (Remix 5 only, else -1)
    u16 soshiTimer;                      // 0x39E
    s16 arrow;                           // 0x3A0
};

struct TossBoysCue {
    u8 boy;           // 0x0
    u16 arcHeight;    // 0x2
    u16 travelTicks;  // 0x4
    u8 from;          // 0x6
    u8 to;            // 0x7
    s16 drumNote;     // 0x8
    u8 action;        // 0xA
    u8 relaunch;      // 0xB
};


// Engine Definition Data:
extern struct Animation **toss_boys_anim_table[];
extern struct DrumTechInstrument toss_boys_drumtech_bank[];
extern struct CompressedData *toss_boys_buffered_textures[];
extern struct GraphicsTable *toss_boys_gfx_tables[];
extern u16 toss_boys_button_masks[];
extern struct Animation *D_089e8660[][3];
extern s8 D_089e8690[][3];
extern struct Vector2 D_089e869c[];
extern struct Vector2 D_089e86a8[];
extern u8 toss_boys_arrow_anim_ids[];
extern s16 D_089e86bc[][4];
extern struct SongHeader *toss_boys_ball_bounce_sfx[];
extern struct SongHeader *toss_boys_ball_pop_sfx[];
extern s8 toss_boys_miss_anim_ids[];
extern struct SongHeader *toss_boys_ball_miss_sfx[];
extern u8 D_089e8704[][4];
extern s8 toss_boys_hit_anim_ids[][3];
extern s8 toss_boys_barely_anim_ids[][3];
extern s8 toss_boys_ready_anim_ids[][3];
extern s8 toss_boys_ready_anim_playback[][3];
extern s8 toss_boys_beat_anim_ids[][3];
extern s8 toss_boys_beat_anim_playback[][3];


// Functions:
extern void func_0803e824(void);
extern void func_0803e884(void); // Engine Event 0x08 (?)
extern void func_0803e8b4(void);
extern void func_0803e908(void);
extern void func_0803e960(void);
extern struct Animation *toss_boys_get_anim(u32 anim); // Get Animation
extern void func_0803e9b0(u32 drumID); // Engine Event 0x03 (?)
extern void func_0803ea08(u32 drumNote); // Engine Event 0x04 (?)
extern void toss_boys_init_gfx3(void); // Graphics Init. 3
extern void toss_boys_init_gfx2(void); // Graphics Init. 2
extern void toss_boys_init_gfx1(void); // Graphics Init. 1
extern void toss_boys_engine_start(u32 version); // Game Engine Start
extern void toss_boys_engine_event_stub(void); // Engine Event 0x09 (STUB)
extern void func_0803ee18(void);
extern void func_0803ee58(u32 boy, u32 frames);
extern void func_0803eea0(void);
extern void func_0803ef64(void);
extern void func_0803f038(void);
extern void func_0803f0b8(u32 boy); // Engine Event 0x06 (?)
extern void func_0803f12c(void); // Engine Event 0x07 (?)
extern void toss_boys_engine_update(void); // Game Engine Update
extern void toss_boys_engine_stop(void); // Game Engine Stop
extern void func_0803f1bc(void); // Engine Event 0x00 (?)
extern void func_0803f1f4(u32 target, u32 arcHeight, u32 travelTicks, s32 timingOffset);
extern void func_0803f390(u32 arg); // Engine Event 0x01 (?)
extern void func_0803f3b0(u32 arg); // Engine Event 0x02 (?)
extern void func_0803f400(struct TossBoysCue *info, s32 timingOffset);
extern void toss_boys_cue_spawn(struct Cue *, struct TossBoysCue *, u32 param); // Cue - Spawn
extern u32  toss_boys_cue_update(struct Cue *, struct TossBoysCue *, u32 runningTime, u32 duration); // Cue - Update
extern void toss_boys_cue_despawn(struct Cue *, struct TossBoysCue *); // Cue - Despawn
extern void func_0803f59c(struct Cue *cue, struct TossBoysCue *info, u32 barely);
extern void toss_boys_cue_hit(struct Cue *, struct TossBoysCue *, u32 pressed, u32 released); // Cue - Hit
extern void toss_boys_cue_barely(struct Cue *, struct TossBoysCue *, u32 pressed, u32 released); // Cue - Barely
extern void toss_boys_cue_miss(struct Cue *, struct TossBoysCue *); // Cue - Miss
extern void func_0803f9a0(u32 boy, s32 startCel);
extern void func_0803fa64(u32 boy, u32 action);
extern void func_0803fb00(u32 arg); // Engine Event 0x05 (?)
extern void func_0803fb14(void);
extern void toss_boys_input_event(u32 pressed, u32 released); // Input Event
extern void toss_boys_common_beat_animation(void); // Common Event 0 (Beat Animation)
extern void toss_boys_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
extern void toss_boys_common_init_tutorial(void); // Common Event 2 (Init. Tutorial, Unimplemented)
