#include "global.h"
#include "lib_sram.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\"");//Temporary
#endif

u16 verify_sram_fast_code[0x50] __attribute__((nocommon));    // Static Copy of verify_sram_fast_rom()
u16 read_sram_fast_code[0x40] __attribute__((nocommon));      // Static Copy of read_sram_fast_rom()
u32 write_int_sram_fast_code[0x0C] __attribute__((nocommon)); // Static Copy of write_int_sram_fast_rom()

extern void (*write_int_sram_fast_rom)(const u8 *src, u8 *dest, u32 size); // ARM Function


/* SRAM Library */


// (https://decomp.me/scratch/twvo4)
#ifndef PLATFORM_PC
#include "asm/lib_sram/asm_0804c870.s"
#endif

// (https://decomp.me/scratch/pFWEv)
#ifndef PLATFORM_PC
#include "asm/lib_sram/asm_0804c8b0.s"
#endif

// ...
#ifndef PLATFORM_PC
#include "asm/lib_sram/asm_0804c920.s"
#endif

// (https://decomp.me/scratch/qFu1X)
#ifndef PLATFORM_PC
#include "asm/lib_sram/asm_0804c96c.s"
#endif

// ...
#ifndef PLATFORM_PC
#include "asm/lib_sram/asm_0804ca40.s"
#endif
