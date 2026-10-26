#pragma once

#include "global.h"
#include "engines.h"

#include "games/rat_race/graphics/rat_race_graphics.h"

// Engine Types:
struct Rat {
    s16 ratSprite;
    s16 sweatSprite;
    u8 unk4;        // state: 0 entering, 1 running, 2 stopped, 3/4 collided, 5 angry, 6 crouched, 7 cheering
    u8 unk5;        // 0 = the player's rat
    s32 unk8;       // 16.8 track position. Signed: the code shifts it with ASRS and
                    // compares it with BLT/BGT, and it starts negative in version 0.
    u32 unkC;       // follow-up for func_0803aef4; only ever accessed as a byte
};

// One of the nine dash-dust puffs. 8 bytes.
struct RatRaceDashParticle {
    s16 sprite;             // 0x00
    u8  active;             // 0x02
    u8  unk03;
    s32 x;                  // 0x04  16.8 fixed point
};

// One of the six plates on the conveyor. 8 bytes.
struct RatRacePlate {
    s16 sprite;             // 0x00
    u8  active;             // 0x02
    u8  unk03;
    s32 x;                  // 0x04  16.8 fixed point
};

// Field offsets recovered from the engine's own assembly
// (asm/engines/rat_race/). sizeof() is 0x120 on the GBA, as the placeholder
// this replaces asserted. On a 64-bit host the pointer at 0x004 moves to 8
// for alignment and is 8 bytes wide, so everything after it sits 8 bytes
// later (and unk0E8, also a pointer, adds more) -- do not reuse the raw
// numbers in scripts or tables.
struct RatRaceEngineData {
    u8  version;                                // 0x000
    u8  unk001[3];
    struct BitmapFontOBJ *objFont;              // 0x004
    s16 textSprite;                             // 0x008
    u8  unk00A[2];
    s32 textX;                                  // 0x00C
    u8  unk010;                                 // 0x010
    u8  unk011;
    s16 bubbleSprite;                           // 0x012
    u8  unk014;                                 // 0x014
    u8  unk015[3];
    s32 unk018;                                 // 0x018
    u8  unk01C;                                 // 0x01C
    u8  unk01D[3];
    s32 unk020;                                 // 0x020
    s32 unk024;                                 // 0x024
    s32 unk028;                                 // 0x028
    u8  unk02C;                                 // 0x02C
    u8  unk02D[3];
    s32 unk030;                                 // 0x030
    s32 unk034;                                 // 0x034
    u8  unk038;                                 // 0x038
    u8  unk039[3];
    struct Rat rats[3];                         // 0x03C
    s16 playerLabelSprite;                      // 0x06C
    u8  unk06E[2];
    s32 unk070;                                 // 0x070
    s16 catPupilsSprite;                        // 0x074
    s16 catEyelidsSprite;                       // 0x076
    s16 catPawSprite;                           // 0x078
    s16 catPawMirrorSprite;                     // 0x07A
    u8  unk07C;                                 // 0x07C
    u8  unk07D[3];
    s32 unk080;                                 // 0x080
    s32 catY;                                   // 0x084  16.8 fixed point
    struct RatRaceDashParticle particles[9];    // 0x088
    s16 unk0D0;                                 // 0x0D0
    u8  unk0D2;                                 // 0x0D2
    u8  unk0D3;                                 // 0x0D3
    s32 unk0D4;                                 // 0x0D4
    s16 blankSprite;                            // 0x0D8
    u8  unk0DA;                                 // 0x0DA
    u8  unk0DB;
    s16 trafficLightSprite;                     // 0x0DC
    u8  unk0DE;                                 // 0x0DE  sign state (func_0803ad60)
    s8  affineGroup;                            // 0x0DF
    s8  unk0E0;                                 // 0x0E0  sign rotation, starts at 0x40
    u8  unk0E1;
    u16 unk0E2;                                 // 0x0E2  sign hold timer
    s16 unk0E4;                                 // 0x0E4  shake amount after a collision
    u8  unk0E6[2];
    struct SongHeader *unk0E8;                  // 0x0E8  sound for the next sign change (event 10)
    struct RatRacePlate plates[6];              // 0x0EC
    u8  unk11C;                                 // 0x11C
    u8  unk11D;                                 // 0x11D
    u8  unk11E;                                 // 0x11E
    u8  unk11F;
};

struct RatRaceCue {
    s16 sprite;     // 0x00
    u8  kind;       // 0x02  indexes D_089e66bc for the spawn offset
    u8  unk03;
    s32 x;          // 0x04  16.8 fixed point
};


// Engine Data:
extern const char D_0805a8b4[];


// Engine Definition Data:
extern struct CompressedData *rat_race_buffered_textures[];
extern struct GraphicsTable rat_race_gfx_table[];
extern u32 D_089e66bc[];
extern u8 D_089e6834[];
extern u8 D_089e684c[];
extern struct Rat rat_race_init_rat_data[];
extern s32 D_089e68ac[];


// Functions:
extern void rat_race_init_gfx3(void); // Graphics Init. 3
extern void rat_race_init_gfx2(void); // Graphics Init. 2
extern void rat_race_init_gfx1(void); // Graphics Init. 1
extern void rat_race_engine_start(u32 version); // Game Engine Start
extern void rat_race_engine_event_stub(); // Engine Event 00 (STUB)
extern void func_0803a158(u32 mode); // Engine Event 02 (Set the Track Mode)
extern void func_0803a164(void); // Decay the run speed
extern void func_0803a198(void); // Advance the track
extern void func_0803a1d4(u32 boost); // Engine Event 09 (Set the Next Speed Boost)
extern void func_0803a1e4(void); // Engine Event 0A (Fill the Speed Budget)
extern void func_0803a1f8(s32 volume); // Engine Event 03 (Set the Target Music Volume)
extern void func_0803a204(void); // Engine Event 04 (Fade the Music to the Target Volume)
extern void func_0803a230(void *unused, s16 spriteId, const char *string); // Speech bubble opened
extern void func_0803a2a8(const char *string); // Engine Event 06 (A Rat Speaks)
extern void func_0803a350(const char *string); // Engine Event 07 (Close the Speech Bubble)
extern void func_0803a3b8(u32 enable); // Engine Event 08 (Enable the Speech Bubble and Sign)
extern void func_0803a3c4(void); // Keep the speech bubble over its rat
extern void func_0803a41c(void); // Engine Event 0C (Release the Player From the Pace Line)
extern void func_0803a434(void); // Engine Event 0F (Hide the Player Label)
extern void rat_race_engine_update(void); // Game Engine Update
extern void func_0803a47c(void); // Engine Event 11 (Stop Letting the Player Start Running)
extern void func_0803a490(u32 index); // Engine Event 12 (Set the Cue Result Index for Collisions)
extern void rat_race_engine_stop(void); // Game Engine Stop
extern s32 func_0803a4a8(u32 kind); // Where a crockery cue starts
extern void rat_race_cue_spawn_stop(struct Cue *, struct RatRaceCue *, u32 param); // Cue - Spawn (Stop)
extern u32  rat_race_cue_update_stop(struct Cue *, struct RatRaceCue *, u32 runningTime, u32 duration); // Cue - Update (Stop)
extern void rat_race_cue_despawn_stop(struct Cue *, struct RatRaceCue *); // Cue - Despawn (Stop)
extern void rat_race_cue_spawn_dash(struct Cue *, struct RatRaceCue *, u32 param); // Cue - Spawn (Dash)
extern u32  rat_race_cue_update_dash(struct Cue *, struct RatRaceCue *, u32 runningTime, u32 duration); // Cue - Update (Dash)
extern void rat_race_cue_despawn_dash(struct Cue *, struct RatRaceCue *); // Cue - Despawn (Dash)
extern void rat_race_cue_hit_stop(struct Cue *, struct RatRaceCue *, u32 pressed, u32 released); // Cue - Hit (Stop)
extern void rat_race_cue_hit_dash(struct Cue *, struct RatRaceCue *, u32 pressed, u32 released); // Cue - Hit (Dash)
extern void rat_race_cue_barely(struct Cue *, struct RatRaceCue *, u32 pressed, u32 released); // Cue - Barely
extern void rat_race_cue_miss(struct Cue *, struct RatRaceCue *); // Cue - Miss
extern void rat_race_input_event(u32 pressed, u32 released); // Input Event
extern void rat_race_common_beat_animation(void); // Common Event 0 (Beat Animation, Unimplemented)
extern void rat_race_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
extern void rat_race_common_init_tutorial(struct Scene *); // Common Event 2 (Init. Tutorial)
extern void func_0803a678(void); // Init. the Cat
extern void func_0803a798(u32 state); // Engine Event 05 (Set the Cat State)
extern void func_0803a8e4(void); // Update the cat
extern void func_0803aa58(void); // Engine Event 0B (Bring Up the Goal)
extern void func_0803aa9c(void); // Scroll the backgrounds, and spot the goal
extern void func_0803aba4(struct Rat *rat, u32 index); // Init. one Rat
extern void func_0803ac98(u32 cel); // Engine Event 0D (Set the Sign)
extern void func_0803ad50(struct SongHeader *sound); // Engine Event 10 (Set the Sound for the Next Sign Change)
extern void func_0803ad60(void); // Animate the sign and its shake
extern void func_0803aef4(void *unused, s16 spriteId, struct Rat *rat); // Rat animation finished
extern void func_0803b034(u32 gait); // Engine Event 01 (Set the Pack Gait)
extern s32 func_0803b1ac(s32 x); // One frame of running, pulled toward the pace line
extern void func_0803b1e8(void); // Update whether the player is clear of the pace line
extern s32 func_0803b230(struct Rat *rat); // A rat screen x, 16.8
extern void func_0803b258(struct Rat *rat); // Player runs into the rat ahead
extern void func_0803b37c(void); // Update the rats
extern void func_0803b924(void); // The player stops
extern void func_0803b9fc(void); // The player dashes
extern void func_0803baa0(struct RatRaceDashParticle *particle); // Init. one dust puff
extern void func_0803baf8(void *unused, s16 spriteId, struct RatRaceDashParticle *particle); // Dust puff finished
extern void func_0803bb2c(s32 x); // Kick up one dust puff
extern void func_0803bbd8(struct RatRaceDashParticle *particle); // Scroll one dust puff
extern void func_0803bc08(void); // Scroll the dust puffs
extern void func_0803bc40(struct RatRacePlate *plate); // Init. one plate
extern void func_0803bc98(void); // Engine Event 0E (Send In a Plate)
extern void func_0803bd0c(struct RatRacePlate *plate); // Scroll one plate
extern void func_0803bd58(void); // Scroll the plates
