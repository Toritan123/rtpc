#ifdef PLATFORM_PC
#include "input.h"
#include "gba_mem.h"
#include "pc_settings.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// GBA REG_KEY offset inside IO array
#define IO_KEY 0x130

// GBA key bit positions (active-low in REG_KEY)
#define GBA_A       (1 << 0)
#define GBA_B       (1 << 1)
#define GBA_SELECT  (1 << 2)
#define GBA_START   (1 << 3)
#define GBA_RIGHT   (1 << 4)
#define GBA_LEFT    (1 << 5)
#define GBA_UP      (1 << 6)
#define GBA_DOWN    (1 << 7)
#define GBA_R       (1 << 8)
#define GBA_L       (1 << 9)

// RTPC_AUTO=4: hold times requested by the game side (pc_autoplay_press),
// one countdown per GBA key bit.
static int s_autoplay_hold[10];
static uint32_t s_autoplay_idle;
static uint32_t s_autoplay_idle_limit = 300;

int pc_autoplay_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("RTPC_AUTO");
        on = (e && atoi(e) == 4);
    }
    return on;
}

void pc_autoplay_press(unsigned buttons, int holdFrames)
{
    for (int i = 0; i < 10; i++) {
        if ((buttons & (1u << i)) && s_autoplay_hold[i] < holdFrames) {
            s_autoplay_hold[i] = holdFrames;
        }
    }
    s_autoplay_idle = 0;
}

// Buttons the autoplay is holding (bit mask): still counting down, or down
// in the REG_KEY value the game read this frame. A press requested now only
// shows up next frame, so pressing one of these again would not be an edge.
static unsigned s_autoplay_down;
unsigned pc_autoplay_held(void)
{
    unsigned m = s_autoplay_down;
    for (int i = 0; i < 10; i++) {
        if (s_autoplay_hold[i] > 0) m |= 1u << i;
    }
    return m;
}

// frames == 0: gameplay has its play inputs open. A stray press there is
// judged -- tanuki_and_monkey takes it as "do it again", rat_race marks it
// down -- so the fallback stays off unless RTPC_AUTO_IDLE=<frames> asks for
// it. The drum lessons need that: they wait for the player to start
// drumming on their own ("好きなタイミングでどうぞ").
void pc_autoplay_set_idle_limit(unsigned frames)
{
    static int open_limit = -1;
    if (open_limit < 0) {
        const char *e = getenv("RTPC_AUTO_IDLE");
        open_limit = e ? atoi(e) : 0;
    }
    if (frames == 0) {
        frames = open_limit ? (unsigned)open_limit : 0xffffffffu;
    }
    s_autoplay_idle_limit = frames;
}

void pc_autoplay_set_hold(unsigned buttons, int holdFrames)
{
    for (int i = 0; i < 10; i++) {
        if (buttons & (1u << i)) {
            s_autoplay_hold[i] = holdFrames;
        }
    }
    s_autoplay_idle = 0;
}

// pc-enhanced: keyboard and gamepad bindings come from settings.ini
// (key_<button> / pad_<button>, comma-separated SDL names; see
// platform/pc_settings.c for the defaults). Bit i of REG_KEY is button i.
#define MAX_BIND 6
#define MAX_PADS 4
#define AXIS_PRESS 16000   // half travel, for triggers and the stick
static const char *const kButtonNames[10] = {
    "a", "b", "select", "start", "right", "left", "up", "down", "r", "l",
};
static SDL_Scancode s_keys[10][MAX_BIND];
static int s_nkeys[10];
static struct { int is_axis, id; } s_padbind[10][MAX_BIND];
static int s_npad[10];
static int s_pad_stick;
static SDL_GameController *s_pads[MAX_PADS];

static void parse_bindings(void)
{
    for (int b = 0; b < 10; b++) {
        char name[16], list[128], *tok, *save;
        snprintf(name, sizeof(name), "key_%s", kButtonNames[b]);
        snprintf(list, sizeof(list), "%s", pc_settings_get(name));
        s_nkeys[b] = 0;
        for (tok = strtok_r(list, ",", &save); tok && s_nkeys[b] < MAX_BIND; tok = strtok_r(NULL, ",", &save)) {
            SDL_Scancode sc;
            while (*tok == ' ') tok++;
            sc = SDL_GetScancodeFromName(tok);
            if (sc != SDL_SCANCODE_UNKNOWN) s_keys[b][s_nkeys[b]++] = sc;
            else fprintf(stderr, "[INPUT] %s: unknown key name \"%s\"\n", name, tok);
        }
        snprintf(name, sizeof(name), "pad_%s", kButtonNames[b]);
        snprintf(list, sizeof(list), "%s", pc_settings_get(name));
        s_npad[b] = 0;
        for (tok = strtok_r(list, ",", &save); tok && s_npad[b] < MAX_BIND; tok = strtok_r(NULL, ",", &save)) {
            SDL_GameControllerButton btn;
            SDL_GameControllerAxis axis;
            while (*tok == ' ') tok++;
            btn = SDL_GameControllerGetButtonFromString(tok);
            axis = SDL_GameControllerGetAxisFromString(tok);
            if (btn != SDL_CONTROLLER_BUTTON_INVALID) {
                s_padbind[b][s_npad[b]].is_axis = 0; s_padbind[b][s_npad[b]++].id = btn;
            } else if (axis != SDL_CONTROLLER_AXIS_INVALID) {
                s_padbind[b][s_npad[b]].is_axis = 1; s_padbind[b][s_npad[b]++].id = axis;
            } else {
                fprintf(stderr, "[INPUT] %s: unknown pad name \"%s\"\n", name, tok);
            }
        }
    }
    s_pad_stick = atoi(pc_settings_get("pad_stick")) != 0;
    if (getenv("RTPC_INPUT_DUMP")) {   // test aid: show the parsed bindings
        for (int b = 0; b < 10; b++) {
            fprintf(stderr, "[INPUT] %-6s keys", kButtonNames[b]);
            for (int k = 0; k < s_nkeys[b]; k++) fprintf(stderr, " %s", SDL_GetScancodeName(s_keys[b][k]));
            fprintf(stderr, "  pad");
            for (int k = 0; k < s_npad[b]; k++)
                fprintf(stderr, " %s", s_padbind[b][k].is_axis
                        ? SDL_GameControllerGetStringForAxis(s_padbind[b][k].id)
                        : SDL_GameControllerGetStringForButton(s_padbind[b][k].id));
            fprintf(stderr, "\n");
        }
    }
}

void input_init(void)
{
    // All buttons released → REG_KEY = 0x03FF
    *(volatile uint16_t *)(gba_io + IO_KEY) = 0x03FF;
    parse_bindings();
}

int input_handle_event(const SDL_Event *ev)
{
    if (ev->type == SDL_QUIT) return 1;
    if (ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_ESCAPE) return 1;
    // SDL sends CONTROLLERDEVICEADDED for pads already plugged in at start
    // too, so this is the only place pads are opened.
    if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        for (int i = 0; i < MAX_PADS; i++) {
            if (!s_pads[i]) {
                s_pads[i] = SDL_GameControllerOpen(ev->cdevice.which);
                if (s_pads[i]) fprintf(stderr, "[INPUT] gamepad: %s\n", SDL_GameControllerName(s_pads[i]));
                break;
            }
        }
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED) {
        for (int i = 0; i < MAX_PADS; i++) {
            if (s_pads[i] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_pads[i])) == ev->cdevice.which) {
                SDL_GameControllerClose(s_pads[i]);
                s_pads[i] = NULL;
            }
        }
    }
    return 0;
}

void input_update_reg_key(void)
{
    const uint8_t *ks = SDL_GetKeyboardState(NULL);
    uint16_t reg = 0x03FF; // start with all released

    for (int b = 0; b < 10; b++) {
        int down = 0;
        for (int k = 0; k < s_nkeys[b] && !down; k++) down = ks[s_keys[b][k]];
        for (int p = 0; p < MAX_PADS && !down; p++) {
            if (!s_pads[p]) continue;
            for (int k = 0; k < s_npad[b] && !down; k++) {
                if (s_padbind[b][k].is_axis)
                    down = SDL_GameControllerGetAxis(s_pads[p], s_padbind[b][k].id) > AXIS_PRESS;
                else
                    down = SDL_GameControllerGetButton(s_pads[p], s_padbind[b][k].id);
            }
        }
        if (down) reg &= ~(1u << b);
    }
    if (s_pad_stick) {
        for (int p = 0; p < MAX_PADS; p++) {
            int x, y;
            if (!s_pads[p]) continue;
            x = SDL_GameControllerGetAxis(s_pads[p], SDL_CONTROLLER_AXIS_LEFTX);
            y = SDL_GameControllerGetAxis(s_pads[p], SDL_CONTROLLER_AXIS_LEFTY);
            if (x >  AXIS_PRESS) reg &= ~GBA_RIGHT;
            if (x < -AXIS_PRESS) reg &= ~GBA_LEFT;
            if (y >  AXIS_PRESS) reg &= ~GBA_DOWN;
            if (y < -AXIS_PRESS) reg &= ~GBA_UP;
        }
    }

    // TEMP (RTPC_AUTO=1): tap A every 30 frames — walks the menus and then
    // hammers cues during gameplay so the hit-detection path gets exercised.
    // RTPC_AUTO=2 additionally taps SELECT every 4 seconds, which skips the
    // tutorial loop so the main chart (and the results screen after it) is
    // reachable without a human player.
    {
        static int auto_on = -1;
        static uint32_t f = 0;
        if (auto_on < 0) {
            const char *e = getenv("RTPC_AUTO");
            auto_on = e ? atoi(e) : 0;
            if (auto_on) {
                // Loud on purpose: synthetic input looks like the game playing
                // itself, which is confusing if RTPC_AUTO was left set by
                // accident.
                fprintf(stderr,
                    "[INPUT] RTPC_AUTO=%d - AUTOPILOT ON, key presses are synthetic\n",
                    auto_on);
                fflush(stderr);
            }
        }
        if (auto_on == 4) {
            // RTPC_AUTO=4: press exactly what the live cues ask for, on time
            // (see gameplay_update_all_cues). After 5 s without a live cue
            // it falls back to tapping A every 30 frames, so menus and text
            // still advance -- but not while gameplay has its play inputs
            // open, unless RTPC_AUTO_IDLE says so (pc_autoplay_set_idle_limit).
            if (++s_autoplay_idle > s_autoplay_idle_limit && (s_autoplay_idle % 30) < 2) reg &= ~GBA_A;
            s_autoplay_down = 0;
            for (int i = 0; i < 10; i++) {
                if (s_autoplay_hold[i] > 0) {
                    reg &= ~(1u << i);
                    s_autoplay_down |= 1u << i;
                    s_autoplay_hold[i]--;
                }
            }
        } else if (auto_on) {
            f++;
            if ((f % 30) < 2) reg &= ~GBA_A;
            if (auto_on >= 2 && (f % 240) < 2) reg &= ~GBA_SELECT;
            // RTPC_AUTO=3: mash every button on a fixed pseudo-random schedule,
            // to reproduce crashes that only happen while keys are held.
            // Deterministic, so a failure can be replayed.
            if (auto_on >= 3) {
                static uint32_t r = 0x1234567u;
                r = r * 1103515245u + 12345u;
                reg &= ~((r >> 9) & 0x3FF);
            }
        }
    }

    // RTPC_KEYLOG=<file> (test aid): record "frame keys epoch clock since"
    // whenever the pressed set changes, frames counted from boot, so a run
    // can be replayed in mGBA (tools/mgba_ref.c) and compared frame by frame.
    // clock is D_030053c0.runningTime when the keys are read, epoch counts
    // its restarts (scene changes) and since counts the frames it has stood
    // still (paused script); mgba_ref replays on these, because the ROM's
    // frame count drifts with load times.
    {
        static FILE *log = NULL;
        static int opened = 0;
        static uint32_t frame = 0;
        static uint16_t last = 0xFFFF;
        static int lastClock = 0, epoch = 0, since = 0;
        int clock = pc_script_clock();
        if (clock < lastClock) epoch++; // a new scene restarted the clock
        since = (clock == lastClock) ? since + 1 : 0;
        lastClock = clock;
        if (!opened) {
            const char *e = getenv("RTPC_KEYLOG");
            if (e && *e) log = fopen(e, "w");
            opened = 1;
        }
        if (log) {
            uint16_t keys = (uint16_t)(~reg & 0x3FF);
            if (keys != last) {
                fprintf(log, "%u 0x%03x %d %d %d\n", frame, keys, epoch, clock, since);
                fflush(log);
                last = keys;
            }
        }
        frame++;
    }

    *(volatile uint16_t *)(gba_io + IO_KEY) = reg;
}

#endif // PLATFORM_PC
