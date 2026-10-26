#pragma once
#ifdef PLATFORM_PC

#include <SDL2/SDL.h>
#include <stdint.h>

#define GBA_W 240
#define GBA_H 160

// Initialise the PPU renderer with a given SDL_Renderer.
// Creates an internal 240×160 SDL_Texture for scanline output.
int  ppu_init(SDL_Renderer *renderer);
void ppu_destroy(void);

// Compose one frame from GBA memory (VRAM/OAM/Palette/IO registers)
// and upload it to the SDL_Texture.  Call once per VBlank.
void ppu_render_frame(void);

// Present the rendered frame to the screen (scaled to the window).
void ppu_present(SDL_Renderer *renderer);

#endif // PLATFORM_PC
