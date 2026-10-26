#pragma once

#include "global.h"
#include "engines.h"
#include "src/bitmap_font.h"

#include "games/bunny_hop/graphics/bunny_hop_graphics.h"

// Engine Types:
struct BunnyHopRabbit {
    s16 sprite;     // 0x00
    u8  state;      // 0x02 (0 idle, 1 run, 2-9 jumps, 10 sparkle, 11 gone)
    s32 x;          // 0x04 (24.8)
    s32 y;          // 0x08 (24.8)
    s32 time;       // 0x0C (24.8 ticks)
    s32 duration;   // 0x10 (24.8 ticks)
    u16 height;     // 0x14
    s8  affineID;   // 0x16
    s8  rotation;   // 0x17
    u8  spinning;   // 0x18
};

struct BunnyHopPlatform {
    s16 sprite;     // 0x00
    u8  active;     // 0x02
    u8  type;       // 0x03
    s32 x;          // 0x04 (24.8)
    s32 y;          // 0x08 (24.8)
    u16 time;       // 0x0C (frames)
    u8  hit;        // 0x0E
    s16 spout;      // 0x10
    s32 sinkY;      // 0x14 (24.8)
    u16 bounceTime; // 0x18
    s32 bounceY;    // 0x1C (24.8)
};

struct BunnyHopCloud {
    s16 sprite;     // 0x0
    s32 x;          // 0x4 (24.8)
    s32 y;          // 0x8 (24.8)
};

struct BunnyHopParticle {
    s16 sprite;     // 0x00
    u8  active;     // 0x02
    s32 x;          // 0x04 (24.8)
    s32 y;          // 0x08 (24.8)
    s32 velX;       // 0x0C
    s32 velY;       // 0x10
};

struct BunnyHopEngineData {
    u8 version;                                  // 0x000
    struct BunnyHopRabbit rabbit;                // 0x004
    struct BunnyHopPlatform platforms[6];        // 0x020
    s32 scrollY;                                 // 0x0E0 (24.8)
    s32 bg1X;                                    // 0x0E4 (24.8)
    s32 bg2X;                                    // 0x0E8 (24.8)
    s32 bg3X;                                    // 0x0EC (24.8)
    u32 beatTime;                                // 0x0F0 (24.8 ticks)
    u8  resultA;                                 // 0x0F4 (1 hit, 2 barely, 3 miss)
    u8  resultB;                                 // 0x0F5
    u8  pendingJump;                             // 0x0F6
    struct BunnyHopCloud clouds[10];             // 0x0F8
    struct BitmapFontOBJ *objFont;               // 0x170
    s16 textSprite;                              // 0x174
    u8  stopScroll;                              // 0x176
    s16 moon;                                    // 0x178
    s32 moonY;                                   // 0x17C (24.8)
    struct BunnyHopParticle particles[20];       // 0x180
    u8  harmonyIndex;                            // 0x310
    struct SoundPlayer *harmonyPlayer;           // 0x314
    u8  hopCount;                                // 0x318
};

struct BunnyHopCue {
    u8 type;
};


// Engine Data:
extern const char D_0805a8ac[];


// Engine Definition Data:
extern struct CompressedData *bunny_hop_buffered_textures[];
extern struct GraphicsTable *bunny_hop_gfx_tables[];
extern struct SongHeader *bunny_hop_bgm_harmony_parts[];
extern struct SongHeader *bunny_hop_bgm_drum_fills[];
extern Palette *bunny_hop_palettes[];
extern struct Animation *bunny_hop_platform_anim[];


// Functions:
extern void bunny_hop_init_gfx3(void); // Graphics Init. 3
extern void bunny_hop_init_gfx2(void); // Graphics Init. 2
extern void bunny_hop_init_gfx1(void); // Graphics Init. 1
extern void bunny_hop_engine_start(u32 version); // Game Engine Start
extern void bunny_hop_engine_event_stub(void); // Engine Event 00 (STUB)
extern void func_08038248(const char *text); // Engine Event 03 (?)
extern void func_080382ac(void);
extern void func_080382b0(void); // Engine Event 04 (?)
extern void func_080382f4(u32 volume); // Engine Event 05 (?)
extern void func_08038314(void); // Engine Event 06 (?)
extern void func_0803833c(struct SongHeader *song); // Engine Event 07 (?)
extern void bunny_hop_engine_update(void); // Game Engine Update
extern void func_08038384(u32 from, u32 to, u32 frames);
extern void func_080383f0(u32 param); // Engine Event 09 (?)
extern void bunny_hop_engine_stop(void); // Game Engine Stop
extern void bunny_hop_cue_spawn(struct Cue *, struct BunnyHopCue *, u32 param); // Cue - Spawn
extern u32  bunny_hop_cue_update(struct Cue *, struct BunnyHopCue *, u32 runningTime, u32 duration); // Cue - Update
extern void bunny_hop_cue_despawn(struct Cue *, struct BunnyHopCue *); // Cue - Despawn
extern void func_0803843c(void);
extern void func_080384b8(u8 type);
extern void func_08038598(void);
extern void bunny_hop_cue_hit(struct Cue *, struct BunnyHopCue *, u32 pressed, u32 released); // Cue - Hit
extern void bunny_hop_cue_barely(struct Cue *, struct BunnyHopCue *, u32 pressed, u32 released); // Cue - Barely
extern void bunny_hop_cue_miss(struct Cue *, struct BunnyHopCue *); // Cue - Miss
extern void bunny_hop_input_event(u32 pressed, u32 released); // Input Event
extern void bunny_hop_common_beat_animation(void); // Common Event 0 (Beat Animation, Unimplemented)
extern void bunny_hop_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
extern void bunny_hop_common_init_tutorial(struct Scene *); // Common Event 2 (Init. Tutorial)
extern void func_080388d8(struct BunnyHopPlatform *platform);
extern void func_0803899c(u32 type); // Engine Event 01 (?)
extern void func_08038a84(void);
extern void func_08038b98(struct BunnyHopPlatform *platform);
extern void func_08038ce0(u16 ticks);
extern void func_08038d18(struct SpriteHandler *handler, s16 sprite, uintptr_t arg);
extern void func_08038d54(void);
extern void func_08038ef8(void);
extern void func_08038f2c(void);
extern void func_08038fbc(u8 state, u8 success);
extern s32 func_08039128(s32 duration, s32 height, s32 time);
extern void func_08039164(void);
extern void func_0803934c(u32 arg); // Engine Event 02 (?)
extern void func_08039388(void);
extern void func_08039404(void);
extern void func_08039440(void);
extern void func_080394a4(void);
extern void func_080395dc(u32 arg); // Engine Event 08 (?)
extern void func_0803960c(struct BunnyHopCloud *cloud, u8 index);
extern void func_08039698(void);
extern void func_08039738(struct BunnyHopParticle *particle);
extern void func_0803978c(s16 x, s16 y, s32 velX, s32 velY);
extern void func_080397f8(void);
