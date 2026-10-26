#include "gba_mem.h"

uint8_t gba_ewram[0x40000];
uint8_t gba_iwram[0x8000];
uint8_t gba_io[0x400];
uint8_t gba_palette[0x400];
uint8_t gba_vram[0x18000];
uint8_t gba_oam[0x400];
uint8_t gba_sram[0x10000];

// Four zero bytes – GameROMBase random reads land here harmlessly.
const uint8_t gba_rom_dummy[4] = {0, 0, 0, 0};
