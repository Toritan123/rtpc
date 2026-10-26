#include "engines/bunny_hop.h"
#include "src/scenes/gameplay.h"
#include "src/code_08001360.h"
#include "src/code_08007468.h"
#include "src/code_0800b778.h"
#include "src/task_pool.h"
#include "src/lib_0804ca80.h"
#include "src/audio.h"
#include "src/palette.h"
#include "src/text_printer.h"
#include "src/bitmap_font.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\""); // Temporary
#endif

// For readability.
#define gBunnyHop ((struct BunnyHopEngineData *)gCurrentEngineData)


/* BUNNY HOP */


#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08037f84.s"
#else
// Graphics Init. 3
void bunny_hop_init_gfx3(void) {
    func_0800c604(0);
    gameplay_start_screen_fade_in();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08037f94.s"
#else
// Graphics Init. 2
void bunny_hop_init_gfx2(void) {
    s32 task;

    func_0800c604(0);
    task = func_08002ee0(get_current_mem_id(), bunny_hop_gfx_tables[gBunnyHop->version], 0x2000);
    run_func_after_task(task, bunny_hop_init_gfx3, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08037fd4.s"
#else
// Graphics Init. 1
void bunny_hop_init_gfx1(void) {
    s32 task;

    func_0800c604(0);
    task = start_new_texture_loader(get_current_mem_id(), bunny_hop_buffered_textures);
    run_func_after_task(task, bunny_hop_init_gfx2, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038000.s"
#else
// Game Engine Start
void bunny_hop_engine_start(u32 version) {
    struct TextPrinter *textPrinter;
    u8 i;

    gBunnyHop->version = version;
    bunny_hop_init_gfx1();
    scene_show_obj_layer();
    scene_set_bg_layer_display(BG_LAYER_1, TRUE, 0, 0, 0, 28, 0x8001);
    scene_set_bg_layer_display(BG_LAYER_2, TRUE, 0, 0, 0, 30, 0x8003);
    scene_set_bg_layer_display(BG_LAYER_3, TRUE, 0, 0, 0, 26, 0x4002);

    gBunnyHop->objFont = scene_create_obj_font_printer(0x340, 2);
    gBunnyHop->textSprite = sprite_create(gSpriteHandler, bmp_font_obj_print_c(gBunnyHop->objFont, D_0805a8ac, 0, 0)->frames, 0, 80, 80, 0, 0, 0, 0);
    gameplay_set_input_buttons(A_BUTTON, 0);

    gBunnyHop->scrollY = 0;
    gBunnyHop->bg1X = 0;
    gBunnyHop->bg2X = 0;
    gBunnyHop->bg3X = 0;
    gBunnyHop->beatTime = 0;
    gBunnyHop->resultA = 0;
    gBunnyHop->resultB = 0;
    gBunnyHop->pendingJump = 0;
    gBunnyHop->stopScroll = FALSE;
    gBunnyHop->harmonyIndex = 0;
    gBunnyHop->hopCount = 0;

    func_08038f2c();
    for (i = 0; i < 6; i++) {
        func_080388d8(&gBunnyHop->platforms[i]);
    }
    for (i = 0; i < 10; i++) {
        func_0803960c(&gBunnyHop->clouds[i], i);
    }
    for (i = 0; i < 20; i++) {
        func_08039738(&gBunnyHop->particles[i]);
    }

    gBunnyHop->moonY = -0x1c000;
    gBunnyHop->moon = sprite_create(gSpriteHandler, anim_bunny_hop_moon, 0, 64, 64, 0x8006, 0, 0, 0);
    sprite_set_x_y(gSpriteHandler, gBunnyHop->moon, 64, (s16)(gBunnyHop->moonY >> 8));

    if (gBunnyHop->version == 2) {
        func_0803899c(1);
        func_08038ce0(0x18);
        func_0803899c(1);
        func_08038ce0(0xc);
        gBunnyHop->beatTime += 0x2400;
        func_08038fbc(3, TRUE);
    }

    textPrinter = text_printer_create_new(get_current_mem_id(), 2, 240, 30);
    text_printer_set_x_y(textPrinter, 0, 8);
    text_printer_set_layer(textPrinter, 0x800);
    text_printer_center_by_content(textPrinter, FALSE);
    text_printer_set_palette(textPrinter, 8);
    text_printer_set_colors(textPrinter, 0);
    text_printer_set_line_spacing(textPrinter, 80);
    gameplay_set_text_printer(textPrinter);
    gameplay_enable_cue_input_overlap(TRUE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038244.s"
#else
// Engine Event 0x00 (STUB)
void bunny_hop_engine_event_stub(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038248.s"
#else
// Engine Event 0x03 (Print OBJ Text)
void func_08038248(const char *text) {
    struct PrintedTextAnim *anim;

    delete_bmp_font_obj_text_anim(gBunnyHop->objFont, gBunnyHop->textSprite);
    anim = bmp_font_obj_print_c(gBunnyHop->objFont, text, 1, 12);
    sprite_set_anim(gSpriteHandler, gBunnyHop->textSprite, anim->frames, 0, 1, 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080382ac.s"
#else
// (STUB)
void func_080382ac(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080382b0.s"
#else
// Engine Event 0x04 (Play Next Harmony Part)
void func_080382b0(void) {
    struct SoundPlayer *player;

    player = play_sound(bunny_hop_bgm_harmony_parts[gBunnyHop->harmonyIndex]);
    gBunnyHop->harmonyPlayer = player;
    set_soundplayer_speed(player, 0x100);
    gBunnyHop->harmonyIndex++;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080382f4.s"
#else
// Engine Event 0x05 (Set Harmony Volume)
void func_080382f4(u32 volume) {
    set_soundplayer_volume(gBunnyHop->harmonyPlayer, (u16)volume);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038314.s"
#else
// Engine Event 0x06 (Play Random Drum Fill)
void func_08038314(void) {
    set_soundplayer_speed(play_sound(bunny_hop_bgm_drum_fills[agb_random(4)]), 0x100);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803833c.s"
#else
// Engine Event 0x07 (Play Sound at Normal Speed)
void func_0803833c(struct SongHeader *song) {
    set_soundplayer_speed(play_sound(song), 0x100);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038350.s"
#else
// Game Engine Update
void bunny_hop_engine_update(void) {
    gBunnyHop->beatTime += func_0800c398();
    func_08039440();
    func_08038ef8();
    func_080394a4();
    func_08039698();
    func_080397f8();
    func_080382ac();
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038384.s"
#else
// Fade Between Palettes (BG and OBJ)
void func_08038384(u32 from, u32 to, u32 frames) {
    palette_fade_to(get_current_mem_id(), (u8)frames, 9, (const u16 *)bunny_hop_palettes[from], (const u16 *)bunny_hop_palettes[to], (u16 *)D_03004b10.bgPalette);
    palette_fade_to(get_current_mem_id(), (u8)frames, 9, (const u16 *)bunny_hop_palettes[from], (const u16 *)bunny_hop_palettes[to], (u16 *)D_03004b10.objPalette);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080383f0.s"
#else
// Engine Event 0x09 (Palette Fade: from | to << 4 | ticks << 8)
void func_080383f0(u32 param) {
    u32 from = param & 0xf;
    u32 to = (param >> 4) & 0xf;

    func_08038384(from, to, ticks_to_frames(param >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038414.s"
#else
// Game Engine Stop
void bunny_hop_engine_stop(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038418.s"
#else
// Cue - Spawn
void bunny_hop_cue_spawn(struct Cue *cue, struct BunnyHopCue *info, u32 param) {
    info->type = param;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803841c.s"
#else
// Cue - Update
u32 bunny_hop_cue_update(struct Cue *cue, struct BunnyHopCue *info, u32 runningTime, u32 duration) {
    return (runningTime > ticks_to_frames(0x78));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038438.s"
#else
// Cue - Despawn
void bunny_hop_cue_despawn(struct Cue *cue, struct BunnyHopCue *info) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803843c.s"
#else
// Judge a Two-Cue Hop
void func_0803843c(void) {
    u8 a = gBunnyHop->resultA;
    u8 b = gBunnyHop->resultB;

    if ((a == 0) || (b == 0)) {
        return;
    }

    if ((a == 3) || (b == 3)) {
        play_sound(&s_f_rabbit_miss_seqData);
        func_08038fbc(4, FALSE);
    } else if ((a == 2) || (b == 2)) {
        play_sound(&s_witch_donats_seqData);
        func_08038fbc(4, FALSE);
    } else {
        play_sound_w_pitch_volume(&s_BD4_seqData, 0x180, 0);
        func_08038fbc(4, TRUE);
        func_08038a84();
    }

    gBunnyHop->resultA = 0;
    gBunnyHop->resultB = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080384b8.s"
#else
// Judge a Two-Cue Jump
void func_080384b8(u8 type) {
    u8 a, b;

    if (type == 4) {
        gBunnyHop->pendingJump = 6;
    } else if (type == 5) {
        gBunnyHop->pendingJump = 7;
    } else if (type == 6) {
        gBunnyHop->pendingJump = 8;
    } else if (type == 8) {
        gBunnyHop->pendingJump = 9;
    }

    a = gBunnyHop->resultA;
    b = gBunnyHop->resultB;
    if ((a == 0) || (b == 0)) {
        return;
    }

    if ((a == 3) || (b == 3)) {
        play_sound(&s_f_rabbit_miss_seqData);
        func_08038fbc(gBunnyHop->pendingJump, FALSE);
        func_08038d54();
    } else if ((a == 2) || (b == 2)) {
        play_sound(&s_witch_donats_seqData);
        func_08038fbc(gBunnyHop->pendingJump, FALSE);
        func_08038d54();
    } else {
        play_sound(&s_rabbit_HC_seqData);
        func_08038fbc(gBunnyHop->pendingJump, TRUE);
        func_08038d54();
    }

    gBunnyHop->resultA = 0;
    gBunnyHop->resultB = 0;
    gBunnyHop->pendingJump = 0;
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038598.s"
#else
// Play the Landing Drum (kick on even beats, snare on odd)
void func_08038598(void) {
    u32 beats = gBunnyHop->beatTime >> 8;
    u32 beat = beats / 24;

    if ((beats % 24) > 12) {
        beat++;
    }

    if ((beat & 1) == 0) {
        play_sound_w_pitch_volume(&s_BD5_seqData, 0x180, 0);
    } else {
        play_sound_w_pitch_volume(&s_SD5_seqData, 0x180, 0);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080385f4.s"
#else
// Cue - Hit
void bunny_hop_cue_hit(struct Cue *cue, struct BunnyHopCue *info, u32 pressed, u32 released) {
    switch (info->type) {
        case 0:
            func_08038fbc(3, TRUE);
            func_08038a84();
            func_08038598();
            break;
        case 3:
            func_08038fbc(5, TRUE);
            func_08038a84();
            func_08038598();
            break;
        case 1:
            gBunnyHop->resultA = 1;
            func_0803843c();
            break;
        case 2:
            gBunnyHop->resultB = 1;
            func_0803843c();
            break;
        case 4:
        case 5:
        case 6:
        case 8:
            gBunnyHop->resultA = 1;
            func_080384b8(info->type);
            break;
        case 7:
            gBunnyHop->resultB = 1;
            func_080384b8(7);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080386e0.s"
#else
// Cue - Barely
void bunny_hop_cue_barely(struct Cue *cue, struct BunnyHopCue *info, u32 pressed, u32 released) {
    switch (info->type) {
        case 0:
            func_08038fbc(3, FALSE);
            break;
        case 3:
            func_08038fbc(5, FALSE);
            break;
        case 1:
            gBunnyHop->resultA = 2;
            func_0803843c();
            break;
        case 2:
            gBunnyHop->resultB = 2;
            func_0803843c();
            break;
        case 4:
        case 5:
        case 6:
        case 8:
            gBunnyHop->resultA = 2;
            func_080384b8(info->type);
            break;
        case 7:
            gBunnyHop->resultB = 2;
            func_080384b8(7);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080387c4.s"
#else
// Cue - Miss
void bunny_hop_cue_miss(struct Cue *cue, struct BunnyHopCue *info) {
    switch (info->type) {
        case 0:
            func_08038fbc(3, FALSE);
            break;
        case 3:
            func_08038fbc(5, FALSE);
            break;
        case 1:
            gBunnyHop->resultA = 3;
            func_0803843c();
            break;
        case 2:
            gBunnyHop->resultB = 3;
            func_0803843c();
            break;
        case 4:
        case 5:
        case 6:
        case 8:
            gBunnyHop->resultA = 3;
            func_080384b8(info->type);
            break;
        case 7:
            gBunnyHop->resultB = 3;
            func_080384b8(7);
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080388a8.s"
#else
// Input Event
void bunny_hop_input_event(u32 pressed, u32 released) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080388ac.s"
#else
// Common Event 0 (Beat Animation, Unimplemented)
void bunny_hop_common_beat_animation(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080388b0.s"
#else
// Common Event 1 (Display Text, Unimplemented)
void bunny_hop_common_display_text(void) {
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080388b4.s"
#else
// Common Event 2 (Init. Tutorial)
void bunny_hop_common_init_tutorial(struct Scene *skipDestination) {
    if (skipDestination != NULL) {
        gameplay_enable_tutorial(TRUE);
        gameplay_set_skip_destination(skipDestination);
    } else {
        gameplay_enable_tutorial(FALSE);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080388d8.s"
#else
// Init. Platform
void func_080388d8(struct BunnyHopPlatform *platform) {
    platform->active = FALSE;
    platform->x = 0;
    platform->y = 192 << 8;

    platform->sprite = sprite_create(gSpriteHandler, anim_bunny_hop_1beat_turtle, 0, 192, 128, 0x800a, 0, 0, 0);
    sprite_set_x_y(gSpriteHandler, platform->sprite, (s16)(platform->x >> 8), (s16)(platform->y >> 8));
    sprite_set_visible(gSpriteHandler, platform->sprite, FALSE);
    platform->hit = FALSE;

    platform->spout = sprite_create(gSpriteHandler, anim_bunny_hop_water_spout_start, 0, 66, 128, 0x8009, 0, 0, 0);
    sprite_set_x_y(gSpriteHandler, platform->spout, (s16)(platform->x >> 8), (s16)(platform->y >> 8));
    sprite_set_visible(gSpriteHandler, platform->spout, FALSE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803899c.s"
#else
// Engine Event 0x01 (Spawn Platform)
void func_0803899c(u32 type) {
    struct BunnyHopPlatform *platform = gBunnyHop->platforms;
    u8 i;

    for (i = 0; i < 6; i++, platform++) {
        if (!platform->active) {
            break;
        }
    }
    if (i > 5) {
        return;
    }

    platform->active = TRUE;
    platform->type = type;
    if (((u8)type == 0) || ((u8)type == 2)) {
        sprite_set_base_palette(gSpriteHandler, platform->sprite, 0);
    } else {
        sprite_set_base_palette(gSpriteHandler, platform->sprite, 2);
    }

    platform->time = 0;
    platform->x = -20 * 256;
    platform->y = 128 << 8;
    platform->hit = FALSE;
    platform->sinkY = 0;
    platform->bounceTime = 0;
    platform->bounceY = 0;
    sprite_set_anim(gSpriteHandler, platform->sprite, bunny_hop_platform_anim[platform->type], 0, 1, 0, 0);
    sprite_set_x_y(gSpriteHandler, platform->sprite, (s16)(platform->x >> 8), (s16)(platform->y >> 8));
    sprite_set_visible(gSpriteHandler, platform->sprite, TRUE);
    sprite_set_anim(gSpriteHandler, platform->spout, anim_bunny_hop_water_spout_start, 0, 0, 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038a84.s"
#else
// Bounce the Platform Under the Rabbit, Splash
void func_08038a84(void) {
    struct BunnyHopPlatform *platform = gBunnyHop->platforms;
    s32 nearest = 0;
    s32 bestDistance = 1000;
    s32 distance;
    s16 x;
    s32 velX, velY;
    u8 i;

    for (i = 0; i < 6; i++, platform++) {
        distance = ((platform->x + ((90 * platform->time) << 8) / (s32)ticks_to_frames(0x18)) >> 8) - 160;
        if (ABS(distance) < bestDistance) {
            bestDistance = ABS(distance);
            nearest = i;
        }
    }
    gBunnyHop->platforms[nearest].bounceTime = 1;

    for (i = 1; i < 5; i++) {
        x = agb_random(4) + (i * 4) + 158;
        velX = (i << 7) + agb_random(0x40) - 0x20;
        velY = agb_random(0x80) - 0x100 - (i << 6) - 0x40;
        func_0803978c(x, 144, velX, velY);

        x = agb_random(4) - (i * 4) + 158;
        velX = -(i << 7) + agb_random(0x40) - 0x20;
        velY = agb_random(0x80) - 0x100 - (i << 6) - 0x40;
        func_0803978c(x, 144, velX, velY);
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038b98.s"
#else
// Update Platform
void func_08038b98(struct BunnyHopPlatform *platform) {
    s32 offset;
    u16 t;

    if (!gBunnyHop->stopScroll) {
        platform->time++;
    }
    offset = ((90 * platform->time) << 8) / (s32)ticks_to_frames(0x18);

    if (((platform->x + offset) >> 8) > 303) {
        platform->active = FALSE;
        sprite_set_x_y(gSpriteHandler, platform->sprite, 0, 192);
        sprite_set_visible(gSpriteHandler, platform->sprite, FALSE);
        sprite_set_x_y(gSpriteHandler, platform->spout, 0, 192);
        sprite_set_visible(gSpriteHandler, platform->spout, FALSE);
        return;
    }

    if (platform->hit) {
        if (platform->type == 3) {
            platform->sinkY -= 0x2800;
            if ((platform->sinkY >> 8) <= -40) {
                platform->sinkY = -0x2800;
            }
        }
        if (platform->type == 6) {
            sprite_set_x_y(gSpriteHandler, platform->spout, (s16)((platform->x + offset) >> 8),
                           (s16)((platform->y - gBunnyHop->scrollY) >> 8));
        }
    }

    t = platform->bounceTime;
    if ((u16)(t - 1) <= 15) {
        platform->bounceY = (((-(t << 13)) / 16) * t) / 16 + (t << 9);
        platform->bounceTime = t + 1;
    }

    sprite_set_x_y(gSpriteHandler, platform->sprite, (s16)((platform->x + offset) >> 8),
                   (s16)((platform->y - gBunnyHop->scrollY + platform->sinkY + platform->bounceY) >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038ce0.s"
#else
// Advance Active Platforms
void func_08038ce0(u16 ticks) {
    struct BunnyHopPlatform *platform = gBunnyHop->platforms;
    u8 i;

    for (i = 0; i < 6; i++, platform++) {
        if (platform->active == TRUE) {
            platform->time += ticks_to_frames(ticks);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038d18.s"
#else
// Water Spout Callback (loop once the start animation ends)
void func_08038d18(struct SpriteHandler *handler, s16 sprite, uintptr_t arg) {
    func_0800c604(0);
    sprite_set_anim(gSpriteHandler, sprite, anim_bunny_hop_water_spout_loop, 0, 1, 0, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038d54.s"
#else
// React Platforms to a Jump
void func_08038d54(void) {
    struct BunnyHopPlatform *platform = gBunnyHop->platforms;
    s16 x;
    s32 velX, velY;
    u8 i, j;

    for (i = 0; i < 6; i++, platform++) {
        if (!platform->active) {
            continue;
        }

        switch (platform->type) {
            case 3:
                break;
            case 4:
                sprite_set_anim(gSpriteHandler, platform->sprite, anim_bunny_hop_4beat_whale_bounce, 2, 1, 0x7f, 0);
                break;
            case 5:
                sprite_set_anim(gSpriteHandler, platform->sprite, anim_bunny_hop_8beat_whale_bounce, 1, 1, 0x7f, 0);
                break;
            case 6:
                sprite_set_visible(gSpriteHandler, platform->spout, TRUE);
                sprite_set_anim_cel(gSpriteHandler, platform->spout, 1);
                sprite_set_playback(gSpriteHandler, platform->spout, 1, 0, 4);
                sprite_set_callback(gSpriteHandler, platform->spout, func_08038d18, 0);
                break;
            default:
                continue;
        }

        for (j = 1; j < 5; j++) {
            x = agb_random(4) + (j * 4) + 158;
            velX = (j << 7) + agb_random(0x40) - 0x20;
            velY = agb_random(0x80) - 0x400 - (j << 7) - 0x40;
            func_0803978c(x, 144, velX, velY);

            x = agb_random(4) - (j * 4) + 158;
            velX = -(j << 7) + agb_random(0x40) - 0x20;
            velY = agb_random(0x80) - 0x400 - (j << 7) - 0x40;
            func_0803978c(x, 144, velX, velY);
        }
        platform->hit = TRUE;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038ef8.s"
#else
// Update All Platforms
void func_08038ef8(void) {
    struct BunnyHopPlatform *platform = gBunnyHop->platforms;
    u8 i;

    for (i = 0; i < 6; i++, platform++) {
        if (platform->active == TRUE) {
            func_08038b98(platform);
        }
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038f2c.s"
#else
// Init. Rabbit
void func_08038f2c(void) {
    struct BunnyHopRabbit *rabbit = &gBunnyHop->rabbit;

    rabbit->state = 1;
    rabbit->time = 0;
    rabbit->duration = 0;
    rabbit->x = 160 << 8;
    rabbit->y = 128 << 8;
    rabbit->sprite = sprite_create(gSpriteHandler, anim_bunny_hop_run, 0, 196, 128, 0x8005, 1, 0, 0);
    rabbit->affineID = scene_affine_group_alloc();
    assign_sprite_affine_param(rabbit->sprite, rabbit->affineID);
    rabbit->rotation = 0;
    rabbit->spinning = FALSE;
    sprite_set_x_y(gSpriteHandler, rabbit->sprite, (s16)(rabbit->x >> 8), (s16)(rabbit->y >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08038fbc.s"
#else
// Start a Rabbit Jump
void func_08038fbc(u8 state, u8 success) {
    struct BunnyHopRabbit *rabbit = &gBunnyHop->rabbit;
    u32 beatTime;
    s32 divisor;

    rabbit->state = state;
    beatTime = gBunnyHop->beatTime;
    // Snap to the nearest beat: time runs from -half a beat to +half.
    if ((u16)((beatTime >> 8) % 24) <= 11) {
        rabbit->time = beatTime % 0x1800;
    } else {
        rabbit->time = (beatTime % 0x1800) - 0x1800;
    }

    divisor = success ? 1 : 2;

    switch (state) {
        case 2:
            rabbit->duration = 0x1800;
            rabbit->height = gBunnyHop->hopCount * 20;
            break;
        case 3:
        case 4:
        case 5:
            rabbit->duration = 0x1800;
            rabbit->height = 80 / divisor;
            break;
        case 6:
            rabbit->duration = 0x3000;
            rabbit->height = 160 / divisor;
            break;
        case 7:
            rabbit->duration = 0x6000;
            rabbit->height = 320 / divisor;
            break;
        case 8:
            rabbit->duration = 0xc000;
            rabbit->height = 640 / divisor;
            break;
        case 9:
            rabbit->duration = 0xc000;
            rabbit->height = 1280;
            gBunnyHop->stopScroll = TRUE;
            break;
    }

    if (success) {
        sprite_set_anim(gSpriteHandler, rabbit->sprite, anim_bunny_hop_jump, 2, 1, 0x7f, 0);
        rabbit->spinning = FALSE;
        rabbit->rotation = 0;
        set_affine_scale_rotation(rabbit->affineID, 0x100, 0);
    } else {
        sprite_set_anim(gSpriteHandler, rabbit->sprite, anim_bunny_hop_miss, 0, 1, 0x7f, 0);
        rabbit->spinning = TRUE;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039128.s"
#else
// Jump Arc: 4h * t/d * (1 - t/d)
s32 func_08039128(s32 duration, s32 height, s32 time) {
    if (time == 0) {
        return 0;
    }
    return (((-4 * time) * height / duration) * time / duration) + (height * (time * 4) / duration);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039164.s"
#else
// Update Rabbit Jump
void func_08039164(void) {
    struct BunnyHopRabbit *rabbit = &gBunnyHop->rabbit;
    s32 y, time;
    s16 px, py;

    rabbit->time += func_0800c398();
    y = (0x80 - func_08039128(rabbit->duration, rabbit->height, rabbit->time)) * 256;
    rabbit->y = y;

    time = rabbit->time;
    if (rabbit->state == 4) {
        rabbit->y = y - ((time * 64) / 24);
    } else if (rabbit->state == 5) {
        rabbit->y = (y - 0x4000) + ((time * 64) / 24);
    }

    if (rabbit->spinning) {
        rabbit->rotation += 8;
    }

    if (time >= rabbit->duration) {
        rabbit->y = (rabbit->state == 4) ? 0x4000 : 0x8000;
        rabbit->state = 0;
        sprite_set_anim_cel(gSpriteHandler, rabbit->sprite, 0);
        sprite_set_playback(gSpriteHandler, rabbit->sprite, 0, 0, 0);
        rabbit->rotation = 0;
        rabbit->spinning = FALSE;
    }

    if ((rabbit->state > 5) && ((rabbit->y >> 8) <= 80)) {
        gBunnyHop->scrollY = rabbit->y - 0x5000;
        rabbit->y = 0x5000;

        // The big jump: carry on upwards past the screen and turn into a
        // sparkle that flies to the moon.
        if (rabbit->state > 8) {
            s32 half = gBunnyHop->scrollY / 2;

            if ((half >> 8) <= -352) {
                rabbit->y = 0x5000 + 0x16000 + half;
                gBunnyHop->scrollY = -0x2c000;
                if (((rabbit->y >> 8) <= -32) && (((rabbit->time >> 8) % 24) == 0)) {
                    rabbit->time = 0;
                    rabbit->state = 10;
                    sprite_set_anim(gSpriteHandler, rabbit->sprite, anim_bunny_hop_sparkle, 0, 1, 0, 0);
                    rabbit->x = 0x4000;
                    rabbit->y = -0x800;
                    sprite_set_x(gSpriteHandler, rabbit->sprite, 64);
                }
            }
        }
    } else {
        gBunnyHop->scrollY = 0;
    }

    if (rabbit->state > 5) {
        time = rabbit->time;
        if ((((time >> 8) % 6) == 0) && (time < (rabbit->duration / 2))) {
            px = agb_random(0x20) + 144;
            py = ((rabbit->y + gBunnyHop->scrollY) >> 8) - agb_random(0x20);
            func_0803978c(px, py, 0, 0);
        }
    }

    sprite_set_y(gSpriteHandler, rabbit->sprite, (s16)(rabbit->y >> 8));
    set_affine_scale_rotation(rabbit->affineID, 0x100, rabbit->rotation);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803934c.s"
#else
// Engine Event 0x02 (Rabbit Hops on the Spot)
void func_0803934c(u32 arg) {
    if (arg == 1) {
        func_08038fbc(3, TRUE);
    } else {
        gBunnyHop->hopCount++;
        func_08038fbc(2, TRUE);
    }
    play_sound(&s_f_rabbit_ready_seqData);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039388.s"
#else
// Update Sparkle (flies up to the moon)
void func_08039388(void) {
    struct BunnyHopRabbit *rabbit = &gBunnyHop->rabbit;

    rabbit->time += func_0800c398();
    rabbit->y += func_0800c398() / 3;

    if ((rabbit->time >> 8) > 119) {
        play_sound(&s_rabbit_moon_seqData);
        sprite_set_visible(gSpriteHandler, rabbit->sprite, FALSE);
        sprite_set_anim_cel(gSpriteHandler, gBunnyHop->moon, 1);
        rabbit->state = 11;
    }

    sprite_set_y(gSpriteHandler, rabbit->sprite, (s16)(rabbit->y >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039404.s"
#else
// Update Running Rabbit
void func_08039404(void) {
    struct BunnyHopRabbit *rabbit = &gBunnyHop->rabbit;

    rabbit->time += func_0800c398();
    sprite_set_anim_cel(gSpriteHandler, rabbit->sprite, (u8)((rabbit->time >> 8) % 12) >> 2);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039440.s"
#else
// Update Rabbit
void func_08039440(void) {
    switch (gBunnyHop->rabbit.state) {
        case 1:
            func_08039404();
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            func_08039164();
            break;
        case 10:
            func_08039388();
            break;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080394a4.s"
#else
// Update Background Scrolling
void func_080394a4(void) {
    s32 y;

    if (!gBunnyHop->stopScroll) {
        gBunnyHop->bg1X -= 0x5a00 / (s32)ticks_to_frames(0x18);
        if ((gBunnyHop->scrollY >> 8) >= -32) {
            gBunnyHop->bg2X -= 0x2d00 / (s32)ticks_to_frames(0x18);
        }
        gBunnyHop->bg3X -= 0x5a00 / (s32)ticks_to_frames(0x18);
    }

    y = gBunnyHop->scrollY >> 8;
    if (y < -352) {
        y = -352;
    }
    scene_set_bg_layer_pos(BG_LAYER_1, (s16)(gBunnyHop->bg1X >> 8), (s16)(y + 352));

    y = (gBunnyHop->scrollY / 2) >> 8;
    if (y < -352) {
        y = -352;
    }
    scene_set_bg_layer_pos(BG_LAYER_2, (s16)(gBunnyHop->bg2X >> 8), (s16)(y + 352));

    y = (-gBunnyHop->scrollY / 2) >> 8;
    if (y > 352) {
        y = 352;
    }
    sprite_set_y(gSpriteHandler, gBunnyHop->moon, (s16)(y - 320));

    scene_set_bg_layer_pos(BG_LAYER_3, (s16)clamp_int32(gBunnyHop->bg3X >> 8, 0, 256), 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080395dc.s"
#else
// Engine Event 0x08 (Set Ground Position)
void func_080395dc(u32 arg) {
    if (arg != 0) {
        gBunnyHop->bg3X = ((arg - 1) * 90 + 96) << 8;
    } else {
        gBunnyHop->bg3X = 0;
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803960c.s"
#else
// Init. Cloud
void func_0803960c(struct BunnyHopCloud *cloud, u8 index) {
    s8 cel;
    u16 z;

    cel = agb_random(2);
    z = agb_random(2) ? 0x400a : 0x800a;
    cloud->sprite = sprite_create(gSpriteHandler, anim_bunny_hop_cloud, cel, 64, 64, z, 0, 0, 0);
    cloud->x = (index * 3) << 11;
    cloud->y = (-agb_random(480) - 32) * 256;
    sprite_set_x_y(gSpriteHandler, cloud->sprite, (s16)(cloud->x >> 8), (s16)(cloud->y >> 8));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039698.s"
#else
// Update Clouds
void func_08039698(void) {
    struct BunnyHopCloud *cloud = gBunnyHop->clouds;
    u8 i;

    for (i = 0; i < 10; i++, cloud++) {
        if (!gBunnyHop->stopScroll) {
            cloud->x += ((func_0800c398() * 45) << 9) / 0x1800;
        }
        if ((cloud->x >> 8) > 271) {
            cloud->x -= 0x13000;
            cloud->y = (-agb_random(480) - 32) * 256;
        }
        sprite_set_x_y(gSpriteHandler, cloud->sprite, (s16)(cloud->x >> 8), (s16)((cloud->y - gBunnyHop->scrollY) >> 8));
    }
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_08039738.s"
#else
// Init. Splash Particle
void func_08039738(struct BunnyHopParticle *particle) {
    s8 cel = agb_random(2);

    particle->sprite = sprite_create(gSpriteHandler, anim_bunny_hop_splash_particle, cel, 64, 64, 0x4002, 0, 0, 0);
    particle->active = FALSE;
    sprite_set_visible(gSpriteHandler, particle->sprite, FALSE);
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_0803978c.s"
#else
// Spawn Splash Particle
void func_0803978c(s16 x, s16 y, s32 velX, s32 velY) {
    struct BunnyHopParticle *particle = gBunnyHop->particles;
    u32 i;

    for (i = 0; i < 20; i++, particle++) {
        if (!particle->active) {
            break;
        }
    }
    if (i > 19) {
        return;
    }

    particle->active = TRUE;
    particle->x = x * 256;
    particle->y = y * 256;
    particle->velX = velX;
    particle->velY = velY;
    sprite_set_anim_cel(gSpriteHandler, particle->sprite, (s8)agb_random(2));
}
#endif

#ifndef PLATFORM_PC
#include "asm/engines/bunny_hop/asm_080397f8.s"
#else
// Update Splash Particles
void func_080397f8(void) {
    struct BunnyHopParticle *particle = gBunnyHop->particles;
    u32 i;

    for (i = 0; i < 20; i++, particle++) {
        if (!particle->active) {
            continue;
        }

        particle->x += particle->velX;
        if (!gBunnyHop->stopScroll) {
            particle->x += ((func_0800c398() * 45) << 9) / 0x1800;
        }
        particle->velY += 0x30;
        particle->y += particle->velY;

        if ((particle->y >> 8) > 167) {
            particle->active = FALSE;
            sprite_set_visible(gSpriteHandler, particle->sprite, FALSE);
        } else {
            sprite_set_x_y(gSpriteHandler, particle->sprite, (s16)(particle->x >> 8), (s16)((particle->y - gBunnyHop->scrollY) >> 8));
            sprite_set_visible(gSpriteHandler, particle->sprite, TRUE);
        }
    }
}
#endif
