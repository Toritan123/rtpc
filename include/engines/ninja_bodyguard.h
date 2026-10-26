#pragma once

#include "global.h"
#include "engines.h"
#include "src/affine_sprite.h"

#include "games/ninja_bodyguard/graphics/ninja_bodyguard_graphics.h"

// Engine Macros/Enums:
enum NinjaBodyguardVersionsEnum {
    ENGINE_VER_NINJA_BODYGUARD,
    ENGINE_VER_NINJA_REINCARNATE
};

enum NinjaBodyguardAnimationsEnum {
    NINJA_ANIM_LORD_BLINK,
    NINJA_ANIM_LORD_SCARED1,
    NINJA_ANIM_LORD_SCARED2,
    NINJA_ANIM_LORD_WALK,
    NINJA_ANIM_HEART_EYES,
    NINJA_ANIM_ARCHER_DRAW,
    NINJA_ANIM_ARROW_TO_WALL,
    NINJA_ANIM_ARROW_IN_WALL1,
    NINJA_ANIM_ARROW_IN_WALL2,
    NINJA_ANIM_ARROW_IN_WALL3,
    NINJA_ANIM_ARROW_DEFLECT_L,
    NINJA_ANIM_ARROW_DEFLECT_R,
    NINJA_ANIM_ARCHER_FIRE,
    NINJA_ANIM_ARROW_PIECES,
    NINJA_ANIM_ARROW_TO_NINJA,
    NINJA_ANIM_SWING_R,
    NINJA_ANIM_SWING_L,
    NINJA_ANIM_SLICE_R,
    NINJA_ANIM_SLICE_L,
    NINJA_ANIM_APPEAR,
    NINJA_ANIM_RAISE_SWORD,
    NINJA_ANIM_BUTTON_INDICATOR,
    NINJA_ANIM_CUTSCENE_ARROW
};


// Engine Types:
struct NinjaArrowPiece {
    struct AffineSprite *sprite; // 0x00
    s32 x;              // 0x04 (24.8)
    s32 y;              // 0x08 (24.8)
    s32 velX;           // 0x0C
    s32 velY;           // 0x10
    s32 gravity;        // 0x14
    s16 rotation;       // 0x18
    s16 rotationSpeed;  // 0x1A
};

struct NinjaBodyguardEngineData {
    u8 version;                          // 0x000
    u8 currentScene;                     // 0x001 (event 0x0B)
    s16 ninja;                           // 0x004
    u8 swingSide;                        // 0x006 (0 = next swing on A, 1 = on the D-Pad)
    s16 arrows[16];                      // 0x008 (deflected/missed arrows)
    u16 nextArrow;                       // 0x028
    struct NinjaArrowPiece pieces[24];   // 0x02C (sliced arrow halves)
    u16 nextPiece;                       // 0x2CC
    u8 barelyCount;                      // 0x2CE
    u8 missCount;                        // 0x2CF
    s16 lord;                            // 0x2D0
    s16 wallArrow;                       // 0x2D2
    s16 heartEyes;                       // 0x2D4
    u16 archerCount;                     // 0x2D8
    u16 nextArcher;                      // 0x2DA
    s16 archers[8];                      // 0x2DC
    s16 buttonIndicator;                 // 0x2EC
    u8 buttonIndicatorVisible;           // 0x2EE
    u16 cueVolume;                       // 0x2F0
    struct AffineSprite *cutsceneArrow;  // 0x2F4
    s32 cutsceneArrowTime;               // 0x2F8
    s32 cutsceneArrowDuration;           // 0x2FC
};

struct NinjaBodyguardCue {
    u16 volume;
};

struct SpriteVector3 {
    s16 x;
    s16 y;
    s16 z;
};


// Engine Definition Data:
extern struct Animation **ninja_bodyguard_anim_table[];
extern struct SpriteVector3 D_089e69cc[];
extern struct CompressedData *ninja_bodyguard_buffered_textures[];
extern struct GraphicsTable *ninja_bodyguard_gfx_tables[];


// Functions:
extern struct Animation *ninja_get_anim(u32 anim); // Get Animation
extern void func_0803bda8(void); // Create Cutscene Arrow
extern void func_0803be04(u32 show); // Engine Event 10 (?)
extern void func_0803be44(void); // Update Cutscene Arrow Orbit
extern void func_0803be88(u32 ticks); // Engine Event 11 (?)
extern void func_0803bec4(void); // Update Cutscene Arrow
extern void func_0803bf14(void); // Create Archers
extern void func_0803bf74(u32 count); // Engine Event 07 (?)
extern void func_0803c034(void); // Engine Event 08 (?)
extern void func_0803c08c(u32 keepDrawn); // Engine Event 09 (?)
extern void func_0803c190(void);
extern void func_0803c20c(void);
extern void func_0803c260(void);
extern void func_0803c28c(void);
extern void func_0803c2b8(u32 scene); // Engine Event 0B (?)
extern void func_0803c2f4(void);
extern void func_0803c3c4(void);
extern void func_0803c400(void); // Engine Event 0C (?)
extern void func_0803c43c(void);
extern void func_0803c52c(void);
extern void func_0803c5c0(void);
extern void func_0803c5cc(void); // Engine Event 03 (?)
extern void func_0803c5f8(void); // Engine Event 04 (?)
extern void func_0803c638(u32 side); // Engine Event 05 (?)
extern void func_0803c6fc(u32 visible); // Engine Event 0D (?)
extern void func_0803c710(u32 type);
extern void func_0803c834(u32 cel, s32 x, s32 velX, s32 velY, s16 rotationSpeed);
extern void func_0803c8c4(void);
extern void func_0803c960(void); // Engine Event 06 (?)
extern void func_0803c964(void);
extern void func_0803c9f4(void);
extern void func_0803c9f8(u32 ticks); // Engine Event 00 (?)
extern void func_0803ca8c(void); // Engine Event 01 (?)
extern void func_0803cad0(void); // Engine Event 02 (?)
extern void func_0803cb0c(void); // Engine Event 0A (?)
extern void ninja_bodyguard_init_gfx3(void); // Graphics Init. 3
extern void ninja_bodyguard_init_gfx2(void); // Graphics Init. 2
extern void ninja_bodyguard_init_gfx1(void); // Graphics Init. 1
extern void ninja_bodyguard_engine_start(u32 version); // Game Engine Start
extern void ninja_bodyguard_engine_event_stub(void); // Engine Event 12 (STUB)
extern void func_0803ccb4(u32 ticks); // Engine Event 0E (?)
extern void func_0803cce0(u32 volume); // Engine Event 0F (?)
extern void ninja_bodyguard_engine_update(void); // Game Engine Update
extern void ninja_bodyguard_engine_stop(void); // Game Engine Stop
extern void ninja_bodyguard_cue_spawn(struct Cue *, struct NinjaBodyguardCue *, u32 unused); // Cue - Spawn
extern u32  ninja_bodyguard_cue_update(struct Cue *, struct NinjaBodyguardCue *, u32 runningTime, u32 duration); // Cue - Update
extern void ninja_bodyguard_cue_despawn(struct Cue *, struct NinjaBodyguardCue *); // Cue - Despawn
extern void ninja_bodyguard_cue_hit(struct Cue *, struct NinjaBodyguardCue *, u32 pressed, u32 released); // Cue - Hit
extern void ninja_bodyguard_cue_barely(struct Cue *, struct NinjaBodyguardCue *, u32 pressed, u32 released); // Cue - Barely
extern void ninja_bodyguard_cue_miss(struct Cue *, struct NinjaBodyguardCue *); // Cue - Miss
extern void ninja_bodyguard_input_event(u32 pressed, u32 released); // Input Event
extern void ninja_bodyguard_common_beat_animation(void); // Common Event 0 (Beat Animation, Unimplemented)
extern void ninja_bodyguard_common_display_text(void); // Common Event 1 (Display Text, Unimplemented)
