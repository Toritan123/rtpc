#ifdef PLATFORM_PC
#include "input.h"
#include "gba_mem.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

void input_init(void)
{
    // All buttons released → REG_KEY = 0x03FF
    *(volatile uint16_t *)(gba_io + IO_KEY) = 0x03FF;
}

int input_handle_event(const SDL_Event *ev)
{
    if (ev->type == SDL_QUIT) return 1;
    if (ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_ESCAPE) return 1;
    return 0;
}

void input_update_reg_key(void)
{
    const uint8_t *ks = SDL_GetKeyboardState(NULL);
    uint16_t reg = 0x03FF; // start with all released

    // Keyboard mapping
    //   K / J     → A / B
    //   W A S D   → D-pad (arrow keys also work)
    //   Q / E     → L / R shoulder
    //   Enter     → START,  Backspace → SELECT
    if (ks[SDL_SCANCODE_K])          reg &= ~GBA_A;
    if (ks[SDL_SCANCODE_J])          reg &= ~GBA_B;
    if (ks[SDL_SCANCODE_BACKSPACE])  reg &= ~GBA_SELECT;
    if (ks[SDL_SCANCODE_RETURN])     reg &= ~GBA_START;
    if (ks[SDL_SCANCODE_D] || ks[SDL_SCANCODE_RIGHT]) reg &= ~GBA_RIGHT;
    if (ks[SDL_SCANCODE_A] || ks[SDL_SCANCODE_LEFT])  reg &= ~GBA_LEFT;
    if (ks[SDL_SCANCODE_W] || ks[SDL_SCANCODE_UP])    reg &= ~GBA_UP;
    if (ks[SDL_SCANCODE_S] || ks[SDL_SCANCODE_DOWN])  reg &= ~GBA_DOWN;
    if (ks[SDL_SCANCODE_E])          reg &= ~GBA_R;
    if (ks[SDL_SCANCODE_Q])          reg &= ~GBA_L;

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
