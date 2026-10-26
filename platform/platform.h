#pragma once
#ifdef PLATFORM_PC

#include <SDL2/SDL.h>

// Scale factor for the 240×160 GBA display.
// 3× → 720×480 window; change to taste.
#define GBA_SCALE 3

// Initialise SDL2, create window + renderer, init PPU/input/audio.
// Returns 0 on success.
int  platform_init(void);
void platform_destroy(void);

// Process all pending SDL events (input + quit).
// Returns 1 if the user requested exit, 0 otherwise.
int  platform_poll_events(void);

// Wait until the next 60 Hz tick, render the frame, and present it.
// Also updates REG_KEY and pushes audio.
// This replaces the GBA's VBlank wait.
void platform_frame_sync(void);

// Set from the VBlank-equivalent so the game loop can detect a new frame.
extern volatile int gPlatformVBlankFlag;

#endif // PLATFORM_PC
