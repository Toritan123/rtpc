#pragma once

#include "global.h"
#include "engines.h"

#include "games/showtime/graphics/showtime_graphics.h"

// Engine Macros/Enums:
#define SHOWTIME_SUB_AMOUNT 2
#define SHOWTIME_SUB2_AMOUNT 8

enum ShowtimeVersionsEnum {
    SHOWTIME_VER_0,
    SHOWTIME_VER_REMIX_3
};


struct ShowtimeEngineData {
    struct BitmapFontOBJ *objFont;       // 0x000
    s16 textSprite;                      // 0x004
    u8 version;                          // 0x006
    struct ShowtimeBlock {
        s16 sprite;
        u32 state;                       // 1 = bouncing
        s32 time;
    } blocks[SHOWTIME_SUB_AMOUNT];       // 0x008
    struct ShowtimePenguin {
        s16 sprite;                      // 0x00
        u32 state;                       // 0x04 (0 free, 1-8 hop/jump/slide, 9-11 parachute)
        s32 time;                        // 0x08 (24.8 ticks)
        s32 x;                           // 0x0C
        s32 y;                           // 0x10
        s32 startX;                      // 0x14
        s32 startY;                      // 0x18
        s32 landX;                       // 0x1C (0 = caught, 1 = falls in, else slide start x)
        u32 type;                        // 0x20
        u32 barely;                      // 0x24
    } penguins[8];                       // 0x020
    s16 monkey;                          // 0x160
    s16 launcher;                        // 0x162
    s16 ball;                            // 0x164
    u32 monkeyState;                     // 0x168 (0 idle, 2 swinging)
    u32 unk16C;                          // 0x16C
    u16 monkeyTimer;                     // 0x170
    struct ShowtimeBall {
        s16 sprite;
        u32 state;                       // 0 free, 1 stray throw, 2 thrown at a penguin, 3 carried, 4 into the water
        s32 time;
        s32 target;                      // penguin index, -1 = none
    } balls[SHOWTIME_SUB2_AMOUNT];       // 0x174
    s32 waveIndex;                       // 0x1F4
    s32 waveTimer;                       // 0x1F8
    s32 waveScroll;                      // 0x1FC
    struct ShowtimeBubble {
        u32 active;
        s32 sprite;
        s32 time;
        s32 x;                           // 24.8
        s32 y;                           // 24.8
        s32 velX;
        s32 velY;
    } bubbles[16];                       // 0x200
    s32 inputCooldown;                   // 0x3C0
    u32 blendLevel;                      // 0x3C4
    s32 splashX[2];                      // 0x3C8 (ball, penguin; 0 = none)
    u8 crowd;                            // 0x3D0 (event 0)
};

struct ShowtimeCue {
    u32 unk0;
    s32 unk4; // penguin index
};


// Engine Data:
extern const char D_0805a3cc[];


// Engine Definition Data:
extern struct CompressedData *showtime_buffered_textures[]; // Buffered Textures List
extern struct GraphicsTable *showtime_gfx_tables[]; // Graphics Table Index
extern struct Animation *showtime_penguin_beat_anim[];
extern struct Animation *showtime_penguin_jump_prepare_anim[];
extern struct Animation *showtime_penguin_jump_anim[];
extern struct Animation *showtime_penguin_slide_anim[];
extern u32 D_089e3b14[][5];


// Functions:
extern void showtime_init_gfx3(void); // Graphics Init. 3
extern void showtime_init_gfx2(void); // Graphics Init. 2
extern void showtime_init_gfx1(void); // Graphics Init. 1
extern void showtime_engine_start(u32 version); // Game Engine Start
extern void showtime_engine_event_stub(void); // Engine Event 1 (STUB)
extern void showtime_engine_update(void); // Game Engine Update
extern void func_0802be10(u32 crowd); // Engine Event 0 (Set Crowd)
extern void showtime_engine_stop(void); // Game Engine Stop
extern void showtime_cue_spawn_gray(struct Cue *, struct ShowtimeCue *, u32);
extern u32  showtime_cue_update_gray(struct Cue *, struct ShowtimeCue *, u32 runningTime, u32 duration);
extern void showtime_cue_despawn_gray(struct Cue *, struct ShowtimeCue *);
extern void showtime_cue_spawn_black(struct Cue *, struct ShowtimeCue *, u32);
extern u32  showtime_cue_update_black(struct Cue *, struct ShowtimeCue *, u32 runningTime, u32 duration);
extern void showtime_cue_despawn_black(struct Cue *, struct ShowtimeCue *);
extern void showtime_cue_spawn_white_fast(struct Cue *, struct ShowtimeCue *, u32);
extern u32  showtime_cue_update_white_fast(struct Cue *, struct ShowtimeCue *, u32 runningTime, u32 duration);
extern void showtime_cue_despawn_white_fast(struct Cue *, struct ShowtimeCue *);
extern void showtime_cue_spawn_white_fast_swing(struct Cue *, struct ShowtimeCue *, u32);
extern u32  showtime_cue_update_white_fast_swing(struct Cue *, struct ShowtimeCue *, u32 runningTime, u32 duration);
extern void showtime_cue_despawn_white_fast_swing(struct Cue *, struct ShowtimeCue *);
extern void showtime_cue_spawn_white(struct Cue *, struct ShowtimeCue *, u32);
extern u32  showtime_cue_update_white(struct Cue *, struct ShowtimeCue *, u32 runningTime, u32 duration);
extern void showtime_cue_despawn_white(struct Cue *, struct ShowtimeCue *);
extern void showtime_cue_hit(struct Cue *, struct ShowtimeCue *, u32 pressed, u32 released); // Cue - Hit
extern void showtime_cue_barely(struct Cue *, struct ShowtimeCue *, u32 pressed, u32 released); // Cue - Barely
extern void showtime_cue_miss(struct Cue *, struct ShowtimeCue *); // Cue - Miss
extern void showtime_input_event(u32 pressed, u32 released); // Input Event
extern void showtime_common_beat_animation(); // Common Event 0 (Beat Animation)
extern void showtime_common_display_text(const char *text); // Common Event 1 (Display Text)
extern void showtime_common_init_tutorial(struct Scene *skipDestination); // Common Event 2 (Init. Tutorial)
extern void func_0802c1f0(u32, s16, u32);
extern void func_0802c23c(void);
extern void func_0802c334(void);
extern void func_0802c36c(u32 block);
extern u32 func_0802c3d0(u32);
extern void func_0802c40c(void);
extern void func_0802c4b0(void);
extern void func_0802c4c0(u32 type);
extern void func_0802c4f4(u32 type);
extern void func_0802c528(u32 type);
extern void func_0802c55c(s32 x0, s32 y0, s32 x1, s32 y1, s32 height, s32 time, s32 duration, s32 *outX, s32 *outY);
extern void func_0802c5c8(void);
extern s32 func_0802ce70(s32 type);
extern void func_0802cf8c(s32 penguin);
extern void func_0802cfa4(u32);
extern void func_0802cfc8(s32 penguin);
extern void func_0802cfe0(s32 penguin);
extern u32 func_0802d068(u32);
extern u32 func_0802d080(u32 arg0);
extern void func_0802d0b8(void);
extern void func_0802d0dc(u32, s16);
extern void func_0802d104(void);
extern void func_0802d250(void);
extern void func_0802d2bc(void);
extern void func_0802d38c(void);
extern void func_0802d394(void);
extern void func_0802d43c(void);
extern void func_0802d81c(s32 penguin);
extern void func_0802d8bc(u32);
extern void func_0802d918(s32 target);
extern void func_0802d96c(void);
extern void func_0802d9fc(void);
extern void func_0802da84(void);
extern void func_0802db08(void);
extern void func_0802dc54(s32 x, s32 y);
