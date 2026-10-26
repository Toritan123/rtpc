#pragma once
#ifdef PLATFORM_PC

#include <stdint.h>
#include <string.h>

// GBA memory regions as static arrays
extern uint8_t gba_ewram[0x40000];   // 256 KB External Work RAM (0x02000000)
extern uint8_t gba_iwram[0x8000];    // 32 KB Internal Work RAM  (0x03000000)
extern uint8_t gba_io[0x400];        // 1 KB  IO Registers        (0x04000000)
extern uint8_t gba_palette[0x400];   // 1 KB  Palette RAM         (0x05000000)
extern uint8_t gba_vram[0x18000];    // 96 KB Video RAM           (0x06000000)
extern uint8_t gba_oam[0x400];       // 1 KB  Object Attr Memory  (0x07000000)
extern uint8_t gba_sram[0x10000];    // 64 KB Save RAM            (0x0E000000)

// The ROM stays at its link-time address (embedded in the binary).
// We just need a base pointer so GameROMBase arithmetic doesn't
// dereference arbitrary addresses.  We point it at a dummy read-only
// page filled with zeros that is big enough for any random-number read.
extern const uint8_t gba_rom_dummy[4];

// Override the GBA address-space base macros so that all hardware
// register and VRAM/OAM/palette accesses go through our arrays.
#undef ExternWorkRAMBase
#undef InternWorkRAMBase
#undef IORAMBase
#undef PaletteRAMBase
#undef VRAMBase
#undef OAMBase
#undef CartRAMBase
#undef GameROMBase
#undef VideoBuffer
#undef BackBuffer
#undef BGPaletteMem
#undef OBJPaletteMem
#undef D_03007F00
#undef D_03007FA0
#undef REG_INTERRUPT
#undef D_0E000000
#undef D_0E000001
#undef D_0E000002
#undef D_0E000003

#define ExternWorkRAMBase ((uintptr_t)gba_ewram)
#define InternWorkRAMBase ((uintptr_t)gba_iwram)
#define IORAMBase         ((uintptr_t)gba_io)
#define PaletteRAMBase    ((uintptr_t)gba_palette)
#define VRAMBase          ((uintptr_t)gba_vram)
#define OAMBase           ((uintptr_t)gba_oam)
#define CartRAMBase       ((uintptr_t)gba_sram)
// Point GameROMBase at an innocuous dummy buffer so random reads don't segfault.
#define GameROMBase       ((uintptr_t)gba_rom_dummy)

#define VideoBuffer    (VRAMBase + 0x0)
#define BackBuffer     (VRAMBase + 0xA000)
#define BGPaletteMem   (PaletteRAMBase + 0x0)
#define OBJPaletteMem  (PaletteRAMBase + 0x200)

#define D_03007F00    (InternWorkRAMBase + 0x7F00)
#define D_03007FA0    (InternWorkRAMBase + 0x7FA0)
// On PC the interrupt vector slot is just a pointer-sized field in IWRAM.
#define REG_INTERRUPT *(volatile void **)(gba_iwram + 0x7FFC)

#define D_0E000000 *(volatile uint8_t *)(gba_sram + 0)
#define D_0E000001 *(volatile uint8_t *)(gba_sram + 1)
#define D_0E000002 *(volatile uint8_t *)(gba_sram + 2)
#define D_0E000003 *(volatile uint8_t *)(gba_sram + 3)

#endif // PLATFORM_PC
