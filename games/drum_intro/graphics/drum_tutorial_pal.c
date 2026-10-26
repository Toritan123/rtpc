#include "global.h"
#include "graphics.h"

// [D_088c2fe0] Drum Intro (Player Scene) Palette (BG & OBJ)
Palette drum_intro_play_pal[] = {
    /* PALETTE 00 */ {
        /* 00 */ TO_RGB555(0xF8F8F8),
        /* 01 */ TO_RGB555(0x302020),
        /* 02 */ TO_RGB555(0x586058),
        /* 03 */ TO_RGB555(0x98A098),
        /* 04 */ TO_RGB555(0xD8D0D8),
        /* 05 */ TO_RGB555(0xF8F8F8),
        /* 06 */ TO_RGB555(0x000000),
        /* 07 */ TO_RGB555(0x000000),
        /* 08 */ TO_RGB555(0x000000),
        /* 09 */ TO_RGB555(0x000000),
        /* 10 */ TO_RGB555(0x000000),
        /* 11 */ TO_RGB555(0x000000),
        /* 12 */ TO_RGB555(0x000000),
        /* 13 */ TO_RGB555(0x000000),
        /* 14 */ TO_RGB555(0x000000),
        /* 15 */ TO_RGB555(0x000000)
    },
    /* PALETTE 01 */ {
        /* 00 */ TO_RGB555(0xF8F8F8),
        /* 01 */ TO_RGB555(0xF8B860),
        /* 02 */ TO_RGB555(0x989090),
        /* 03 */ TO_RGB555(0xB07838),
        /* 04 */ TO_RGB555(0xF8F8F8),
        /* 05 */ TO_RGB555(0xF83028),
        /* 06 */ TO_RGB555(0x981818),
        /* 07 */ TO_RGB555(0xF87030),
        /* 08 */ TO_RGB555(0x48F820),
        /* 09 */ TO_RGB555(0xF8F810),
        /* 10 */ TO_RGB555(0x00A808),
        /* 11 */ TO_RGB555(0x00F818),
        /* 12 */ TO_RGB555(0x00E8F8),
        /* 13 */ TO_RGB555(0xF80000),
        /* 14 */ TO_RGB555(0xF8F8F8),
        /* 15 */ TO_RGB555(0x000030)
    },
    /* PALETTE 02 */ {
        /* 00 */ TO_RGB555(0xA8A8A8),
        /* 01 */ TO_RGB555(0x302020),
        /* 02 */ TO_RGB555(0x586058),
        /* 03 */ TO_RGB555(0xF83028),
        /* 04 */ TO_RGB555(0x981818),
        /* 05 */ TO_RGB555(0xF8F8F8),
        /* 06 */ TO_RGB555(0xB8B0B0),
        /* 07 */ TO_RGB555(0xB07838),
        /* 08 */ TO_RGB555(0xF8F810),
        /* 09 */ TO_RGB555(0xF87030),
        /* 10 */ TO_RGB555(0x000000),
        /* 11 */ TO_RGB555(0x000000),
        /* 12 */ TO_RGB555(0x00E8F8),
        /* 13 */ TO_RGB555(0x000000),
        /* 14 */ TO_RGB555(0xF8F8F8),
        /* 15 */ TO_RGB555(0x000000)
    },
#ifdef PLATFORM_PC // ROM bytes past this palette (tools/gen_pal_overread.py)
    /* PALETTE 03 */ { 0x0002, 0x80e7, 0x8008, 0x001c, 0x80e7, 0x4018, 0x008e, 0x0001, 0x00c5, 0x8003, 0x1018, 0x0001, 0x00c4, 0x8004, 0x1018, 0x0001 },
    /* PALETTE 04 */ { 0x00c3, 0x8005, 0x1018, 0x0002, 0x00da, 0x4011, 0x11c4, 0x40ea, 0x0011, 0x1220, 0x0002, 0x00d9, 0x4011, 0x11c4, 0x40e9, 0x0011 },
    /* PALETTE 05 */ { 0x1220, 0x0002, 0x00d8, 0x4011, 0x11c4, 0x40e8, 0x0011, 0x1220, 0x0004, 0x00eb, 0x4002, 0x119c, 0x80eb, 0x0012, 0x11da, 0x40fb },
    /* PALETTE 06 */ { 0x0002, 0x121e, 0x00fb, 0x0012, 0x11f8, 0x0004, 0x00f3, 0x4002, 0x10c6, 0x00e3, 0x400a, 0x119a, 0x00eb, 0x0002, 0x10a6, 0x80f3 },
    /* PALETTE 07 */ { 0x0012, 0x11d9, 0x0003, 0x00e8, 0x400b, 0x119c, 0x40f8, 0x000b, 0x121e, 0x80f0, 0x001b, 0x11d8, 0x0002, 0x80e0, 0x800b, 0x1086 },
    /* PALETTE 08 */ { 0x80f0, 0x001b, 0x11d9, 0x0002, 0x40e9, 0x0011, 0x1220, 0x00e6, 0x4011, 0x11cc, 0x0002, 0x40e9, 0x0010, 0x1220, 0x00e6, 0x4011 },
    /* PALETTE 09 */ { 0x11c8, 0x0002, 0x40ea, 0x0010, 0x1220, 0x00e6, 0x4011, 0x11c8, 0x0003, 0x00fc, 0x4005, 0x119e, 0x0001, 0x0003, 0x1238, 0x00fb },
#endif // PLATFORM_PC
};
