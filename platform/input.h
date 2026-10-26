#pragma once
#ifdef PLATFORM_PC

// RTPC_AUTO=4 (perfect autoplay): TRUE when enabled; the game side requests
// key presses with pc_autoplay_press, applied from the next REG_KEY update.
int  pc_autoplay_enabled(void);
void pc_autoplay_press(unsigned buttons, int holdFrames);     // hold at least this long
void pc_autoplay_set_hold(unsigned buttons, int holdFrames);  // hold exactly this long
void pc_autoplay_set_idle_limit(unsigned frames);             // A-tap fallback delay
unsigned pc_autoplay_held(void);                              // buttons autoplay is holding

// D_030053c0.runningTime, for RTPC_KEYLOG (defined in src/code_0800b778.c).
int pc_script_clock(void);

#include <SDL2/SDL.h>
#include <stdint.h>

// Initialise the input layer.
void input_init(void);

// Process one SDL_Event.  Returns 0 normally, 1 if the user closed the window.
int  input_handle_event(const SDL_Event *ev);

// Rebuild REG_KEY from the current SDL keyboard state.
// GBA: bit = 0 when key is PRESSED.
void input_update_reg_key(void);

#endif // PLATFORM_PC
