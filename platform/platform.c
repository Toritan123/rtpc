#ifdef PLATFORM_PC
#include "platform.h"
#include "save_pc.h"
#include "ppu.h"
#include "input.h"
#include "audio_pc.h"
#include "gba_mem.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Target frame duration: 1/59.7275 s ≈ 16742 µs (GBA refresh rate)
#define FRAME_US 16742u

static SDL_Window   *s_win      = NULL;
static SDL_Renderer *s_renderer = NULL;
static uint64_t      s_last_tick = 0;

// RTPC_HEADLESS=1 runs the game with no window, renderer or audio device.
// Everything above the platform layer still runs — scene flow, the beatscript
// VM, the MIDI mixer — so CI can check the engine on a machine with no display.
// SDL's dummy video driver is not enough on its own: it cannot create a
// renderer, and the game then blocks instead of reporting anything.
static int           s_headless = 0;

volatile int gPlatformVBlankFlag = 0;


int platform_init(void)
{
    {
        const char *e = getenv("RTPC_HEADLESS");
        s_headless = (e && atoi(e)) ? 1 : 0;
    }

    if (s_headless) {
        if (SDL_Init(SDL_INIT_TIMER) != 0) {
            fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
            return -1;
        }
        input_init();
        audio_pc_init_dump_only();   // RTPC_AUDIO_DUMP still records, no device
        fprintf(stderr, "[PLATFORM] headless: no window, renderer or audio\n");
        fflush(stderr);
        s_last_tick = SDL_GetTicks64();
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    s_win = SDL_CreateWindow(
        "Rhythm Tengoku – PC Port",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        240 * GBA_SCALE, 160 * GBA_SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!s_win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return -1;
    }

    s_renderer = SDL_CreateRenderer(s_win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer) {
        // Fallback to software renderer
        s_renderer = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_SOFTWARE);
        if (!s_renderer) {
            fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
            return -1;
        }
    }

    // Bring the game window to the front and give it keyboard focus, so that
    // keystrokes go to the game instead of piling up in the launching terminal
    // (where they would be echoed and eventually run as shell input).
    // Measured: SDL already makes the process frontmost on macOS even for a
    // non-bundled, terminal-launched binary, so this is the whole fix needed.
    SDL_RaiseWindow(s_win);

    // Keep integer scaling so pixels stay sharp
    SDL_RenderSetLogicalSize(s_renderer, 240, 160);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest-neighbour

    if (ppu_init(s_renderer) != 0) {
        fprintf(stderr, "ppu_init failed: %s\n", SDL_GetError());
        return -1;
    }

    input_init();

    if (audio_pc_init() != 0) {
        fprintf(stderr, "audio_pc_init failed (no audio): %s\n", SDL_GetError());
        // Non-fatal: continue without audio
    }

    s_last_tick = SDL_GetTicks64();
    return 0;
}

void platform_destroy(void)
{
    rtpc_sram_flush();
    if (s_headless) { SDL_Quit(); return; }
    audio_pc_destroy();
    ppu_destroy();
    if (s_renderer) { SDL_DestroyRenderer(s_renderer); s_renderer = NULL; }
    if (s_win)      { SDL_DestroyWindow(s_win);        s_win      = NULL; }
    SDL_Quit();
}

int platform_poll_events(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_WINDOWEVENT && getenv("RTPC_WINLOG")) {
            fprintf(stderr, "[WIN] t=%llu event %d\n", (unsigned long long)SDL_GetTicks64(), ev.window.event);
        }
        if (input_handle_event(&ev)) return 1;
    }
    return 0;
}

/* TEMP profiler: set RTPC_PROF=1 to log per-frame phase timings.
 * "logic" = everything between the end of one frame_sync and the start of the
 * next (process_scenes, midi_sound_main, the beatscript engine, …).          */
static double prof_ms(uint64_t a, uint64_t b)
{
    return (double)(b - a) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

void platform_frame_sync(void)
{
    static int      prof = -1;
    static uint64_t t_prev_end = 0;
    static uint64_t t_prev_t0 = 0;
    static uint64_t s_next_frame = 0;   // performance-counter deadline
    uint64_t t0, t_poll, t_render, t_audio, t_wait;
    uint64_t freq, period, now;
    if (prof < 0) { const char *e = getenv("RTPC_PROF"); prof = e ? atoi(e) : 0; }

    t0 = SDL_GetPerformanceCounter();

    // At most one 64 KB write per frame, and only when the game actually
    // touched SRAM, so this costs nothing on the frames that did not save.
    rtpc_sram_flush();

    {   // RTPC_TEST_MINIMIZE=<frame> (test aid): minimise the window at that
        // frame, to watch what a hidden or covered window does to pacing.
        static long at = -2, n = 0;
        if (at == -2) { const char *e = getenv("RTPC_TEST_MINIMIZE"); at = e ? atol(e) : -1; }
        if (!s_headless && at >= 0 && ++n == at) SDL_MinimizeWindow(s_win);
    }
    {   // RTPC_TEST_COVER=<frame> (test aid): open an opaque window of our own
        // exactly over the game window at that frame and close it 600 frames
        // later, to reproduce the covered-window present stall.
        static long at = -2, n = 0;
        static SDL_Window *cover = NULL;
        if (at == -2) { const char *e = getenv("RTPC_TEST_COVER"); at = e ? atol(e) : -1; }
        if (!s_headless && at >= 0) {
            n++;
            if (n == at) {
                int x, y, w, h;
                SDL_GetWindowPosition(s_win, &x, &y);
                SDL_GetWindowSize(s_win, &w, &h);
                cover = SDL_CreateWindow("cover", x, y - 28, w, h + 28, SDL_WINDOW_SHOWN);
                if (cover) {
                    SDL_Surface *sf = SDL_GetWindowSurface(cover);
                    if (sf) { SDL_FillRect(sf, NULL, 0); SDL_UpdateWindowSurface(cover); }
                }
                fprintf(stderr, "[WIN] cover window %s at frame %ld\n", cover ? "opened" : "failed", n);
            } else if (n == at + 600 && cover) {
                SDL_DestroyWindow(cover); cover = NULL;
                fprintf(stderr, "[WIN] cover window closed at frame %ld\n", n);
            }
        }
    }
    {   // RTPC_TEST_OFFSCREEN=<frame> (test aid): move the window off every
        // screen at that frame -- not visible, like a fully covered window.
        static long at = -2, n = 0;
        if (at == -2) { const char *e = getenv("RTPC_TEST_OFFSCREEN"); at = e ? atol(e) : -1; }
        if (!s_headless && at >= 0 && ++n == at) SDL_SetWindowPosition(s_win, -20000, -20000);
    }
    if (s_headless) {
        // Still build the frame when a test aid wants pictures of it
        // (RTPC_SHOTS, RTPC_FRAMEDUMP, RTPC_OAMDUMP): the PPU writes those
        // from its own framebuffer, no window needed.
        static int want = -1;
        if (want < 0) {
            want = getenv("RTPC_SHOTS") || getenv("RTPC_FRAMEDUMP") || getenv("RTPC_OAMDUMP");
        }
        if (want) ppu_render_frame();
    } else {
    // Render GBA frame to texture and present.  This shows the frame the game
    // just finished building, so it happens first — presenting any later would
    // add display latency for no benefit.
    ppu_render_frame();
    SDL_RenderClear(s_renderer);
    ppu_present(s_renderer);
    SDL_RenderPresent(s_renderer);
    }

    t_render = SDL_GetPerformanceCounter();

    // Push audio samples for this frame
    if (!s_headless || audio_pc_dump_only()) audio_pc_push_frame();

    t_audio = SDL_GetPerformanceCounter();

    // ── Pace to 59.7275 Hz ────────────────────────────────────────────────
    // Deadline-based on the performance counter rather than SDL_GetTicks64:
    // millisecond ticks quantise the frame period to whole milliseconds, which
    // measured 16.95 ms against a 16.742 ms target — the whole game ran ~1.2%
    // slow.  Sleep for the bulk of the wait and spin the last stretch, since
    // SDL_Delay only resolves to milliseconds.
    freq   = SDL_GetPerformanceFrequency();
    period = (freq * FRAME_US) / 1000000u;
    now    = SDL_GetPerformanceCounter();
    if (s_next_frame == 0) {
        s_next_frame = now;
    }
    s_next_frame += period;
    if ((int64_t)(s_next_frame - now) > 0) {
        double remain_ms = (double)(int64_t)(s_next_frame - now) * 1000.0 / (double)freq;
        if (remain_ms > 2.0) {
            SDL_Delay((uint32_t)(remain_ms - 1.0));
        }
        while ((int64_t)(s_next_frame - SDL_GetPerformanceCounter()) > 0) {
            /* spin out the sub-millisecond remainder */
        }
    } else if ((int64_t)(now - s_next_frame) > (int64_t)(period * 4)) {
        // Far behind (a stall, or the window was dragged): resync instead of
        // trying to catch up frame by frame.
        s_next_frame = now;
    }

    t_wait = SDL_GetPerformanceCounter();

    // ── Sample input LAST ─────────────────────────────────────────────────
    // agb_main reads REG_KEY through update_key_listener() immediately after
    // this function returns.  Sampling before the pacing wait (as this used to)
    // meant the game acted on a snapshot taken a whole sleep earlier —
    // measured at ~12.3 ms, three quarters of a frame of dead input latency on
    // a rhythm game.  Reading it here makes the snapshot as fresh as possible.
    if (!s_headless) {
        if (platform_poll_events() != 0) {
            SDL_Quit();
            exit(0);
        }
    }
    input_update_reg_key();

    t_poll = SDL_GetPerformanceCounter();

    if (prof) {
        static uint32_t fn = 0;
        double logic = t_prev_end ? prof_ms(t_prev_end, t0) : 0.0;
        double work  = logic + prof_ms(t0, t_audio) + prof_ms(t_wait, t_poll);
        uint16_t keys = (uint16_t)~(*(volatile uint16_t *)(gba_io + 0x130)) & 0x3FF;
        fn++;
        // Log every frame whose real work exceeds ~80% of the 16.7 ms budget,
        // plus one heartbeat per second for a baseline.
        if (prof >= 2 || work > 13.0 || (fn % 60) == 0) {
            fprintf(stderr,
                "[PROF] f=%u keys=%03x iv=%6.2f work=%5.2f | logic=%5.2f poll=%4.2f render=%5.2f audio=%5.2f aq=%6.2f\n",
                fn, keys, t_prev_t0 ? prof_ms(t_prev_t0, t0) : 0.0, work, logic,
                prof_ms(t_wait, t_poll), prof_ms(t0, t_render), prof_ms(t_render, t_audio),
                audio_pc_queued_ms());
            fflush(stderr);
        }
        t_prev_t0 = t0;
    }

    s_last_tick = SDL_GetTicks64();
    t_prev_end = SDL_GetPerformanceCounter();

    // Signal that VBlank occurred
    gPlatformVBlankFlag = 1;
}

#endif // PLATFORM_PC
