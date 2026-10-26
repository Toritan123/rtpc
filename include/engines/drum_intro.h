#pragma once

#include "global.h"
#include "engines.h"
#include "engines/night_walk.h"

#include "games/drum_intro/graphics/drum_intro_graphics.h"

// Engine Macros/Enums:
enum DrumIntroVersionsEnum {
    ENGINE_VER_DRUM_INTRO_TEACHER,
    ENGINE_VER_DRUM_INTRO_PLAYER,
    ENGINE_VER_DRUM_INTRO_TANUKI_MONKEY,
    ENGINE_VER_DRUM_INTRO_CUTSCENE
};


// Engine Types:
struct DrumIntroEngineData {
    u8 version;                          // 0x000
    struct DrumTechController drumTech;  // 0x004
    struct DrumIntroDrummer {
        s16 head;       // 0x354
        s16 body;       // 0x356
        s16 waist;      // 0x358
        s16 armL;       // 0x35A
        s16 armR;       // 0x35C
        s16 legL;       // 0x35E
        s16 legR;       // 0x360
        s16 originX;    // 0x362
        s16 originY;    // 0x364
        s16 bassL;      // 0x366
        s16 bassR;      // 0x368
        s16 snare;      // 0x36A
        s16 tom;        // 0x36C
        s16 hihat;      // 0x36E
        s16 splash;     // 0x370
        s16 crash;      // 0x372
        s16 seat;       // 0x374
        // Flash timers for the kit pieces: < 0 = always shown,
        // > 0 = frames left before the piece hides again.
        s8 bassLTimer;  // 0x376
        s8 bassRTimer;  // 0x377
        s8 snareTimer;  // 0x378
        s8 tomTimer;    // 0x379
        s8 hihatTimer;  // 0x37A
        s8 splashTimer; // 0x37B
        s8 crashTimer;  // 0x37C
    } drummer;
    u8 inputEnabled;                     // 0x380
    u16 bgWobblePhase;                   // 0x382
    u8 modeID;                           // 0x384
    const char *modeText;                // 0x388
    const struct DrumKitCueData *mode;   // 0x38C
    void *modeData;                      // 0x390
    s16 extraSprite;                     // 0x394
    u16 drumMask;                        // 0x396
    s16 tanuki[4];                       // 0x398
    s16 monkey[4];                       // 0x3A0
    u8 autoCueType;                      // 0x3A8
    u8 textIndex;                        // 0x3A9
    const char **textTable;              // 0x3AC
    const struct Beatscript *endScript;  // 0x3B0
};

struct DrumIntroCue {
    u8 type;
};

// Data for the DrumKitCueData modes with a judgement (D_089e03a4).
struct DrumIntroModeData {
    u16 counter;
    u8 rankA; // Judgement for type 0 cues (0 = perfect, 1 = hit, 2 = barely early, 3 = barely late, 4 = miss).
    u8 rankB; // Judgement for every other cue type.
};

struct DrumKitCueData {
    void (*init)(void *data);
    void (*input)(void *data);
    void (*hit)(void *data, struct DrumIntroCue *info, s32 offset);
    void (*barely)(void *data, struct DrumIntroCue *info, s32 offset);
    void (*miss)(void *data, struct DrumIntroCue *info);
    struct SongHeader *(*getSound)(void *data);
    u32 dataSize;
};


// Engine Data:
extern const char D_08059f94[];
extern const char D_08059f98[];
extern const char D_08059f9c[];
extern const char D_08059fb4[];
extern const char D_08059fd0[];
extern const char D_08059fe8[];
extern const char D_0805a004[];
extern const char D_0805a020[];
extern const char D_0805a038[];
extern const char D_0805a048[];
extern const char D_0805a058[];
extern const char D_0805a06c[];


// Engine Definition Data:
extern struct DrumTechNote *tanuki_and_monkey_bgm_seq_table[];
extern u16 D_089dfe94[];
extern struct DrumTechKit *drum_intro_kits[];
extern struct CompressedData *drum_intro_buffered_textures[];
extern struct GraphicsTable *drum_intro_gfx_tables[];
extern struct Vector2 D_089e0164[];
extern struct Animation *D_089e0170[][4];
extern u16 D_089e01b0[];
extern s8 D_089e01b8[][3];
extern u8 D_089e01c4[];
extern struct DrumKitCueData *D_089e0398[];
extern struct DrumKitCueData D_089e03a4;
extern struct DrumKitCueData D_089e03c0;
extern struct DrumKitCueData D_089e03dc;


// Functions:
extern void func_080239a0(u32 index); // Engine Event 0x08 (?)
extern void func_080239bc(s32 x); // Engine Event 0x09 (?)
extern void func_080239ec(s16 sprite);
extern void func_08023a18(void);
extern void func_08023bb8(const char **textTable); // Engine Event 0x0B (?)
extern void func_08023bcc(void); // Engine Event 0x0C (?)
extern void func_08023bf4(void); // Engine Event 0x0D (?)
extern void func_08023c0c(void); // Engine Event 0x0E (?)
extern void func_08023c44(const struct Beatscript *script); // Engine Event 0x0F (?)
extern void func_08023c58(u32 type); // Engine Event 0x0A (?)
extern void drum_intro_cue_spawn_auto(struct Cue *, struct DrumIntroCue *, u32 buttonMask); // Cue - Spawn (Auto-Input)
extern u32  drum_intro_cue_update_auto(struct Cue *, struct DrumIntroCue *, u32 runningTime, u32 duration); // Cue - Update (Auto-Input)
extern void drum_intro_cue_despawn_auto(struct Cue *, struct DrumIntroCue *); // Cue - Despawn (Auto-Input)
extern void drum_intro_cue_hit_auto(struct Cue *, struct DrumIntroCue *, u32 pressed, u32 released); // Cue - Hit (Auto-Input)
extern void drum_intro_cue_barely_auto(struct Cue *, struct DrumIntroCue *, u32 pressed, u32 released); // Cue - Barely (Auto-Input)
extern void drum_intro_cue_miss_auto(struct Cue *, struct DrumIntroCue *); // Cue - Miss (Auto-Input)
extern void func_08023d78(s16 sprite, s8 *timer);
extern void func_08023da0(void);
extern void func_08023da4(); // Drum Kit Event - D-Pad Left
extern void func_08023df8(); // Drum Kit Event - B Button
extern void func_08023e4c(void);
extern void func_08023e50(); // Drum Kit Event - D-Pad Right
extern void func_08023edc(); // Drum Kit Event - A Button
extern void func_08023f68(); // Drum Kit Event - STUB
extern void func_08023f6c(); // Drum Kit Event - D-Pad Down
extern void func_08023ffc(); // Drum Kit Event - D-Pad Up (Drum Kit 2)
extern void func_080240a4(); // Drum Kit Event - L Button
extern void func_08024134(); // Drum Kit Event - R Button
extern void func_080241c0(); // Drum Kit Event - D-Pad Up (Drum Kit 1)
extern void drum_intro_init_gfx3(void); // Graphics Init. 3
extern void drum_intro_init_gfx2(void); // Graphics Init. 2
extern void drum_intro_init_gfx1(void); // Graphics Init. 1
extern void drum_intro_engine_start(u32 version); // Game Engine Start
extern void drum_intro_engine_event_stub(); // Engine Event 0x10 (STUB)
extern void func_0802497c(const char **texts); // Engine Event 0x00 (?)
extern void func_080249c0(u32 buttons); // Engine Event 0x01 (?)
extern void func_080249f0(void);
extern void func_08024a4c(u32 anim); // Engine Event 0x02 (?)
extern void func_08024ae4(u32 state); // Engine Event 0x03 (?)
extern void func_08024ba0(u32 enable); // Engine Event 0x04 (?)
extern u32  func_08024bd0(void);
extern void func_08024be8(u32 mask); // Engine Event 0x07 (?)
extern void func_08024bfc(s16 sprite, s8 *timer);
extern void func_08024c2c(void);
extern void drum_intro_engine_update(void); // Game Engine Update
extern void drum_intro_engine_stop(void); // Game Engine Stop
extern void drum_intro_cue_spawn(struct Cue *, struct DrumIntroCue *, u32 type); // Cue - Spawn
extern u32  drum_intro_cue_update(struct Cue *, struct DrumIntroCue *, u32 runningTime, u32 duration); // Cue - Update
extern void drum_intro_cue_despawn(struct Cue *, struct DrumIntroCue *); // Cue - Despawn
extern void drum_intro_cue_hit(struct Cue *, struct DrumIntroCue *, u32 pressed, u32 released); // Cue - Hit
extern void drum_intro_cue_barely(struct Cue *, struct DrumIntroCue *, u32 pressed, u32 released); // Cue - Barely
extern void drum_intro_cue_miss(struct Cue *, struct DrumIntroCue *); // Cue - Miss
extern void drum_intro_input_event(u32 pressed, u32 released); // Input Event
extern void drum_intro_common_beat_animation(void); // Common Event 0 (Beat Animation)
extern void drum_intro_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
extern void func_08024ed0(void);
extern void func_08024ef4(u32 mode); // Engine Event 0x05 (?)
extern void func_08024f64(void); // Engine Event 0x06 (?)
extern void func_08024fb4(void *data); // Drum Kit Cue Func. 0
extern void func_08024fbc(void *data); // Drum Kit Cue Func. 1
extern void func_08024fc4(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 2
extern void func_08024ff4(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 3
extern void func_08025020(void *data, struct DrumIntroCue *info); // Drum Kit Cue Func. 4
extern struct SongHeader *func_08025038(void *data); // Drum Kit Cue Func. 5
extern void func_080251d0(void *data); // Drum Kit Cue Func. 0
extern void func_080251d8(void *data); // Drum Kit Cue Func. 1
extern void func_080251e8(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 2
extern void func_080251ec(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 3
extern void func_080251f0(void *data, struct DrumIntroCue *info); // Drum Kit Cue Func. 4
extern struct SongHeader *func_080251fc(void *data); // Drum Kit Cue Func. 5
extern void func_08025204(void *data); // Drum Kit Cue Func. 0
extern void func_0802520c(void *data); // Drum Kit Cue Func. 1
extern void func_08025214(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 2
extern void func_08025218(void *data, struct DrumIntroCue *info, s32 offset); // Drum Kit Cue Func. 3
extern void func_0802521c(void *data, struct DrumIntroCue *info); // Drum Kit Cue Func. 4
extern struct SongHeader *func_08025220(void *data); // Drum Kit Cue Func. 5
