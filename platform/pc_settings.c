/*
 * PC-only player settings (pc-enhanced branch). See pc_settings.h.
 *
 * settings.ini lives in SDL_GetPrefPath next to the save and is written in
 * full (every key, with its default) the first time it is missing, so the
 * player can see and edit everything there. When RTPC_SAVE is set (test
 * runs) it is neither read nor written unless RTPC_SETTINGS names a file,
 * so automated runs never touch the player's settings.
 *
 * Judge offset
 * ------------
 * The game judges an input by the frame it arrives on, and its hit windows
 * were tuned on a GBA, where a sound triggered by game logic reaches the
 * speaker about 19 ms later: the mixer keeps the DMA ring filled
 * (gMidiSamplesPerFrame + 259) samples ahead of the read position and mixes
 * one frame's worth at each update, so new samples start ~256 samples
 * (19.1 ms at 13379 Hz) after the one playing. The PC plays the same samples
 * from the SDL queue instead, after the queue depth plus the device buffer.
 * A player pressing with the music therefore lands the difference late. The
 * automatic offset is that difference, rounded to whole frames, since
 * judging is per frame. The autoplay (RTPC_AUTO) judges with offset 0 unless
 * RTPC_INPUT_OFFSET is given, since it presses on the target frame itself.
 */
#ifdef PLATFORM_PC

#include "pc_settings.h"
#include "audio_pc.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FRAME_MS        (1000.0 / 59.7275)
#define GBA_AUDIO_MS    (256.0 * 1000.0 / 13379.0)
#define OFFSET_LIMIT    10

// Every key, its default, and the comment written above it. Bindings list
// SDL names separated by commas: keyboard keys as SDL_GetScancodeName spells
// them, pad buttons/axes as SDL_GameControllerGetStringForButton/Axis do.
static const struct { const char *key, *def, *comment; } kDefaults[] = {
    { "input_offset_frames", "auto",
      "Judge timing: auto, or frames to judge presses earlier (F8 auto, F9/F10 -/+)" },
    { "audio_latency", "low",
      "low: small audio buffer that grows by itself if the sound breaks up; normal: the larger fixed buffer" },
    { "audio_quality", "high",
      "high: modern reverb, no clipping, smoother instruments and PSG, mixer at 3x; gba: the original sound" },
    { "reverb_mix", "100", "Reverb level in high quality, percent of the GBA's" },
    { "spatial", "off", "off / headphones (virtual speakers in front) / wide (wider stereo for speakers); F6 cycles" },
    { "spatial_strength", "60", "Spatial effect strength, 0-100" },
    { "fullscreen", "0", "1 = start in fullscreen (F11 or Alt+Enter toggles)" },
    { "scale", "integer", "integer: sharp whole-number scaling; fit: fill the screen, keeping the shape (F7)" },
    { "key_a", "K", "Keyboard (SDL key names)" },
    { "key_b", "J", NULL },
    { "key_select", "Backspace", NULL },
    { "key_start", "Return", NULL },
    { "key_right", "D,Right", NULL },
    { "key_left", "A,Left", NULL },
    { "key_up", "W,Up", NULL },
    { "key_down", "S,Down", NULL },
    { "key_r", "E", NULL },
    { "key_l", "Q", NULL },
    { "pad_a", "a", "Gamepad (SDL names: a b x y back start leftshoulder rightshoulder lefttrigger righttrigger dpup ...)" },
    { "pad_b", "b", NULL },
    { "pad_select", "back", NULL },
    { "pad_start", "start", NULL },
    { "pad_right", "dpright", NULL },
    { "pad_left", "dpleft", NULL },
    { "pad_up", "dpup", NULL },
    { "pad_down", "dpdown", NULL },
    { "pad_r", "rightshoulder,righttrigger", NULL },
    { "pad_l", "leftshoulder,lefttrigger", NULL },
    { "pad_stick", "1", "1 = the left stick also works as the D-pad" },
};
#define NUM_KEYS (sizeof(kDefaults) / sizeof(kDefaults[0]))

static char   s_values[NUM_KEYS][128];
static int    s_auto = 1;          // offset follows the measured latency
static int    s_manual = 0;        // frames, when !s_auto
static int    s_auto_frames = 0;
static double s_latency_ema = -1.0;
static char   s_path[1024];
static char   s_title[256];

static int key_index(const char *key)
{
    unsigned i;
    for (i = 0; i < NUM_KEYS; i++) {
        if (!strcmp(kDefaults[i].key, key)) return (int)i;
    }
    return -1;
}

static void settings_save(void)
{
    FILE *f;
    unsigned i;
    if (!s_path[0]) return;
    f = fopen(s_path, "w");
    if (!f) return;
    fprintf(f, "# Rhythm Tengoku PC settings\n");
    for (i = 0; i < NUM_KEYS; i++) {
        if (kDefaults[i].comment) fprintf(f, "\n# %s\n", kDefaults[i].comment);
        fprintf(f, "%s=%s\n", kDefaults[i].key, s_values[i]);
    }
    fclose(f);
}

const char *pc_settings_get(const char *key)
{
    int i = key_index(key);
    return i < 0 ? "" : s_values[i];
}

void pc_settings_set(const char *key, const char *value)
{
    int i = key_index(key);
    if (i < 0) return;
    snprintf(s_values[i], sizeof(s_values[i]), "%s", value);
    settings_save();
}

static void parse_offset(const char *v)
{
    while (*v == ' ') v++;
    if (!strncmp(v, "auto", 4)) { s_auto = 1; return; }
    s_auto = 0;
    s_manual = atoi(v);
    if (s_manual >  OFFSET_LIMIT) s_manual =  OFFSET_LIMIT;
    if (s_manual < -OFFSET_LIMIT) s_manual = -OFFSET_LIMIT;
}

static void store_offset(void)
{
    char v[16];
    if (s_auto) snprintf(v, sizeof(v), "auto");
    else        snprintf(v, sizeof(v), "%d", s_manual);
    pc_settings_set("input_offset_frames", v);
}

int pc_audio_hq(void)
{
    static int hq = -1;
    if (hq < 0) {
        const char *e = getenv("RTPC_AUDIO_QUALITY");
        pc_settings_init();
        hq = strcmp(e ? e : pc_settings_get("audio_quality"), "gba") != 0;
    }
    return hq;
}

void pc_settings_init(void)
{
    const char *e = getenv("RTPC_SETTINGS");
    FILE *f;
    char line[256];
    unsigned i;
    int found = 0;
    static int done = 0;

    if (done) return;   // the mixer reads settings before the platform starts
    done = 1;

    for (i = 0; i < NUM_KEYS; i++) {
        snprintf(s_values[i], sizeof(s_values[i]), "%s", kDefaults[i].def);
    }
    if (e) {
        snprintf(s_path, sizeof(s_path), "%s", e);
    } else if (!getenv("RTPC_SAVE")) {
        char *pref = SDL_GetPrefPath(NULL, "Rhythm Tengoku");
        if (pref) {
            snprintf(s_path, sizeof(s_path), "%ssettings.ini", pref);
            SDL_free(pref);
        }
    }
    if (s_path[0] && (f = fopen(s_path, "r")) != NULL) {
        found = 1;
        while (fgets(line, sizeof(line), f)) {
            char *eq = strchr(line, '='), *end;
            int k;
            if (line[0] == '#' || !eq) continue;
            *eq = '\0';
            for (end = eq + 1 + strlen(eq + 1); end > eq + 1 && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '); ) *--end = '\0';
            k = key_index(line);
            if (k >= 0) snprintf(s_values[k], sizeof(s_values[k]), "%s", eq + 1);
        }
        fclose(f);
    }
    if (!found) settings_save();   // first run: show the player every setting

    parse_offset(s_values[key_index("input_offset_frames")]);
    if ((e = getenv("RTPC_INPUT_OFFSET")) != NULL) {
        parse_offset(e);
    } else if (getenv("RTPC_AUTO")) {
        s_auto = 0;
        s_manual = 0;
    }
}

void pc_settings_frame(void)
{
    double q = audio_pc_queued_ms();
    double extra, f;

    if (q <= 0.0) return;   // no device (headless) or not started
    // The queue is quantised by the device's pulls, so smooth over seconds.
    q += audio_pc_device_ms();
    s_latency_ema = (s_latency_ema < 0.0) ? q : s_latency_ema * 0.995 + q * 0.005;
    {
        static int n = 0;
        if (++n % 1800 == 600)
            fprintf(stderr, "[SETTINGS] audio latency %.1f ms (GBA %.1f), auto judge offset %d frames, %u underruns\n",
                    s_latency_ema, GBA_AUDIO_MS, s_auto_frames, audio_pc_underruns());
    }
    extra = s_latency_ema - GBA_AUDIO_MS;
    f = extra / FRAME_MS;
    // Hysteresis: move only when the estimate is well past the boundary, so
    // the judgement does not flip back and forth mid-song.
    if (f > s_auto_frames + 0.75 || f < s_auto_frames - 0.75) {
        s_auto_frames = (int)lround(f);
        if (s_auto_frames < 0) s_auto_frames = 0;
        if (s_auto_frames > OFFSET_LIMIT) s_auto_frames = OFFSET_LIMIT;
        fprintf(stderr, "[SETTINGS] audio latency %.1f ms (GBA %.1f) -> auto judge offset %d frames\n",
                s_latency_ema, GBA_AUDIO_MS, s_auto_frames);
    }
}

int pc_judge_offset_frames(void)
{
    return s_auto ? s_auto_frames : s_manual;
}

int pc_settings_handle_key(int key)
{
    int cur = pc_judge_offset_frames();
    switch (key) {
        case SDLK_F9:  s_auto = 0; s_manual = cur > -OFFSET_LIMIT ? cur - 1 : cur; break;
        case SDLK_F10: s_auto = 0; s_manual = cur <  OFFSET_LIMIT ? cur + 1 : cur; break;
        case SDLK_F8:  s_auto = 1; break;
        default: return 0;
    }
    store_offset();
    fprintf(stderr, "[SETTINGS] judge offset %s %d frames\n", s_auto ? "auto" : "manual",
            pc_judge_offset_frames());
    return 1;
}

const char *pc_settings_title(void)
{
    int n = pc_judge_offset_frames();
    snprintf(s_title, sizeof(s_title),
             "Rhythm Tengoku – PC Port   判定補正 %s%+d (%+.0f ms) [F8 自動 F9/F10 調整]   立体音響 %s [F6]   %s [F7]   F11 全画面",
             s_auto ? "自動 " : "", n, n * FRAME_MS,
             !strcmp(pc_settings_get("spatial"), "headphones") ? "ヘッドホン"
             : !strcmp(pc_settings_get("spatial"), "wide") ? "ワイド" : "オフ",
             strcmp(pc_settings_get("scale"), "fit") ? "整数倍" : "画面いっぱい");
    return s_title;
}

#endif
