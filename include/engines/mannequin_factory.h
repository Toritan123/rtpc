#pragma once

#include "global.h"
#include "engines.h"
#include "engines/night_walk.h"   // struct DrumTechController

#include "games/mannequin_factory/graphics/mannequin_factory_graphics.h"

// Engine Types:

// Recovered from the engine's own assembly (asm/engines/mannequin_factory/);
// the comments give GBA offsets. The pointers at 0x08C and 0x410 (and those
// inside the DrumTech controller) move everything after them on a 64-bit
// host, so do not reuse the raw numbers.
struct MannequinFactoryEngineData {
    u8  version;                    // 0x000
    u16 inputButtons;               // 0x002  buttons currently accepted
    s16 handLSprite;                // 0x004
    s16 handRSprite;                // 0x006
    s16 stampRSprite;               // 0x008
    s16 stampLSprite;               // 0x00A
    u16 buttonCooldowns[4];         // 0x00C  frames until each button is re-enabled
    struct Mannequin {
        u8  state;                  // 0x00  0 = free
        u8  pad01;
        s16 headSprite;             // 0x02
        s16 eyeRSprite;             // 0x04
        s16 eyeLSprite;             // 0x06
        s16 dashSprite;             // 0x08
        u8  kind;                   // 0x0A  head cel: 1 or 2, turned by slaps
        u8  eyeRVisible;            // 0x0B
        u8  eyeLVisible;            // 0x0C
        s8  position;               // 0x0D  station on the belt, 0-4
        u16 xOffset;                // 0x0E
        u8  advanced;               // 0x10  moved on this station already
        u8  pad11;
        s16 velocity;               // 0x12  slap recoil, 8.8
    } mannequins[6];                // 0x014
    struct Mannequin *current;      // 0x08C  the one func_080228d8 spawned a cue for
    struct MannequinFinishEffect {
        u8  state;                  // 0x00
        u8  pad01;
        s16 sprite;                 // 0x02
        u16 timer;                  // 0x04
        u16 duration;               // 0x06
    } effects[4];                   // 0x090
    u16 tempo;                      // 0x0B0
    u8  slowMotion;                 // 0x0B2
    u32 slowTimer;                  // 0x0B4
    u32 slowDuration;               // 0x0B8
    struct DrumTechController drumTech; // 0x0BC
    u8  stopped;                    // 0x40C  set by a game over
    const struct Beatscript *gameOverScript; // 0x410
    u8  gameOver;                   // 0x414  waiting for A on the game-over screen
    u32 score;                      // 0x418
    s16 scoreSprites[4];            // 0x41C
    u32 highScore;                  // 0x424
    s16 highScoreSprites[4];        // 0x428
    u8  mode;                       // 0x430  0/1 = fixed head, 2 = random
};

struct MannequinFactoryCue {
    struct Mannequin *mannequin;    // 0x00
    u8 type;                        // 0x04  0-3 = the four buttons, 5 = none
};


// Engine Definition Data:
extern struct Animation *mannequin_finish_anim[];
extern u16 mannequin_input_buttons[];
extern struct CompressedData *mannequin_buffered_textures[];
extern struct GraphicsTable mannequin_gfx_table[];
extern struct SongHeader *D_089df404[];


// Functions:
extern void func_08022244(void); // Clear the finish effects
extern void func_08022268(struct MannequinFinishEffect *effect); // Update a finish effect
extern void func_080223ac(void); // Update the finish effects
extern void func_080223d0(u32 kind, s32 offset); // Start a finish effect
extern void func_080224a8(void); // Init. the mannequins
extern s32 func_080225bc(struct Mannequin *mannequin); // Mannequin screen x
extern void func_08022614(struct Mannequin *mannequin); // Update a mannequin
extern void func_080226a0(void); // Update the mannequins
extern void func_080226d4(void); // Engine Event 0x00 (Send In a Mannequin)
extern void func_08022894(struct Mannequin *mannequin); // Remove a mannequin
extern void func_080228d8(struct Mannequin *mannequin); // Spawn a mannequin's cue
extern void func_080229bc(void); // Engine Event 0x01 (Spawn the Mannequins' Cues)
extern void func_080229f0(struct Mannequin *mannequin); // Move a mannequin on
extern void func_08022a7c(struct Mannequin *mannequin, s32 direction); // Slap a mannequin
extern void func_08022b0c(struct Mannequin *mannequin, u32 side); // Stamp a mannequin
extern void func_08022ba0(u32 mode); // Engine Event 0x06 (Set the Head Mode)
extern void func_08022bb4(void); // Init. the hands and stamps
extern void func_08022ca0(void); // Count down the button cooldowns
extern void func_08022ce8(u32 button); // Press a button
extern void mannequin_init_gfx3(void); // Graphics Init. 3
extern void mannequin_init_gfx2(void); // Graphics Init. 2
extern void mannequin_init_gfx1(void); // Graphics Init. 1
extern void mannequin_engine_start(u32); // Game Engine Start
extern void mannequin_engine_event_stub(); // Engine Event 0x07 (STUB)
extern void func_08022f00(u32 tempo); // Engine Event 0x02 (Set the Tempo)
extern void func_08022f1c(u32 factor); // Engine Event 0x03 (Scale the Tempo)
extern void func_08022f4c(void); // Start slow motion
extern void func_08022fb8(void); // Update slow motion
extern void mannequin_engine_update(void); // Game Engine Update
extern void func_0802308c(u32 arg); // Play a factory sound
extern void func_080230cc(u32 sound, u32 volume, u32 delay); // Schedule a factory sound
extern void func_0802310c(void); // Game over
extern void func_08023150(const struct Beatscript *script); // Engine Event 0x04 (Set the Game-Over Script)
extern void func_08023164(void); // Engine Event 0x05 (Show Game Over)
extern void func_080231c8(void); // Show the scores
extern void func_0802327c(void); // Init. the scores
extern void func_080233b4(u32 points); // Add points
extern void mannequin_engine_stop(void); // Game Engine Stop
extern void mannequin_cue_spawn(struct Cue *, struct MannequinFactoryCue *, u32 arg); // Cue - Spawn
extern u32  mannequin_cue_update(struct Cue *, struct MannequinFactoryCue *, u32 runningTime, u32 duration); // Cue - Update
extern void mannequin_cue_despawn(struct Cue *, struct MannequinFactoryCue *); // Cue - Despawn
extern void mannequin_cue_hit(struct Cue *, struct MannequinFactoryCue *, u32 pressed, u32 released); // Cue - Hit
extern void mannequin_cue_barely(struct Cue *, struct MannequinFactoryCue *, u32 pressed, u32 released); // Cue - Barely
extern void mannequin_cue_miss(struct Cue *, struct MannequinFactoryCue *); // Cue - Miss
extern void mannequin_input_event(u32 pressed, u32 released); // Input Event
extern void mannequin_common_beat_animation(void); // Common Event 0 (Beat Animation)
extern void mannequin_common_display_text(void); // Common Event 1 (Display Text)
extern void mannequin_common_init_tutorial(void); // Common Event 2 (Init. Tutorial)
