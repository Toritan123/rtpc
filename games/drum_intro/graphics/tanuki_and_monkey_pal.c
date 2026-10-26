#include "global.h"
#include "graphics.h"

// [D_08914ab0] Tanuki & Monkey OBJ Palette
Palette tanuki_and_monkey_obj_pal[] = {
    /* PALETTE 00 */ {
        /* 00 */ TO_RGB555(0x2878B0),
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
        /* 00 */ TO_RGB555(0xB09048),
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
        /* 00 */ TO_RGB555(0x58B028),
        /* 01 */ TO_RGB555(0xF8F8F8),
        /* 02 */ TO_RGB555(0xA8A8A8),
        /* 03 */ TO_RGB555(0x6098A8),
        /* 04 */ TO_RGB555(0x70B0C8),
        /* 05 */ TO_RGB555(0x90E0F8),
        /* 06 */ TO_RGB555(0x000000),
        /* 07 */ TO_RGB555(0x000000),
        /* 08 */ TO_RGB555(0x000000),
        /* 09 */ TO_RGB555(0x000000),
        /* 10 */ TO_RGB555(0xF87828),
        /* 11 */ TO_RGB555(0x000000),
        /* 12 */ TO_RGB555(0x000000),
        /* 13 */ TO_RGB555(0xF80000),
        /* 14 */ TO_RGB555(0xF8B0B0),
        /* 15 */ TO_RGB555(0x000000)
    },
    /* PALETTE 03 */ {
        /* 00 */ TO_RGB555(0x8038B0),
        /* 01 */ TO_RGB555(0x302020),
        /* 02 */ TO_RGB555(0x586058),
        /* 03 */ TO_RGB555(0x98A098),
        /* 04 */ TO_RGB555(0xD8D0D8),
        /* 05 */ TO_RGB555(0xF83838),
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
    /* PALETTE 04 */ {
        /* 00 */ TO_RGB555(0x28A0B0),
        /* 01 */ TO_RGB555(0x302020),
        /* 02 */ TO_RGB555(0x586058),
        /* 03 */ TO_RGB555(0x98A098),
        /* 04 */ TO_RGB555(0xD8D0D8),
        /* 05 */ TO_RGB555(0xE8F800),
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
#ifdef PLATFORM_PC // ROM bytes past this palette (tools/gen_pal_overread.py)
    /* PALETTE 05 */ { 0x12c0, 0x1086, 0x2d8b, 0x4e93, 0x6f5b, 0x7fff, 0x239f, 0x1f3b, 0x1ef9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
#endif // PLATFORM_PC
};

// [D_08914b50] Tanuki & Monkey BG Palette
Palette tanuki_and_monkey_bg_pal[] = {
    /* PALETTE 00 */ {
        /* 00 */ TO_RGB555(0x00B020),
        /* 01 */ TO_RGB555(0x302020),
        /* 02 */ TO_RGB555(0x586058),
        /* 03 */ TO_RGB555(0x98A098),
        /* 04 */ TO_RGB555(0xD8D0D8),
        /* 05 */ TO_RGB555(0xF8F8F8),
        /* 06 */ TO_RGB555(0xF8E040),
        /* 07 */ TO_RGB555(0xD8C838),
        /* 08 */ TO_RGB555(0xC8B838),
        /* 09 */ TO_RGB555(0x000000),
        /* 10 */ TO_RGB555(0x000000),
        /* 11 */ TO_RGB555(0x000000),
        /* 12 */ TO_RGB555(0x000000),
        /* 13 */ TO_RGB555(0x000000),
        /* 14 */ TO_RGB555(0x000000),
        /* 15 */ TO_RGB555(0x000000)
    },
#ifdef PLATFORM_PC // ROM bytes past this palette (tools/gen_pal_overread.py)
    /* PALETTE 01 */ { 0x000b, 0x00db, 0x41f0, 0x0204, 0x40eb, 0x01f0, 0x02fc, 0x00db, 0x5000, 0x0204, 0x40eb, 0x1000, 0x02fc, 0x00e8, 0x81e8, 0x0088 },
    /* PALETTE 02 */ { 0x80e8, 0x8008, 0x010e, 0x0000, 0x41e0, 0x020a, 0x0010, 0x01e8, 0x0327, 0x0010, 0x01d8, 0x0327, 0x4000, 0x01e9, 0x02f6, 0x00f8 },
    /* PALETTE 03 */ { 0x01d8, 0x0327, 0x0007, 0x00db, 0x41f0, 0x0204, 0x40eb, 0x01f0, 0x02fc, 0x00db, 0x5000, 0x0204, 0x40eb, 0x1000, 0x02fc, 0x00e8 },
    /* PALETTE 04 */ { 0x81e8, 0x0088, 0x80e8, 0x8008, 0x010e, 0x4000, 0x01e9, 0x02f6, 0x0007, 0x00db, 0x41f0, 0x0218, 0x40eb, 0x01f0, 0x02fa, 0x00db },
    /* PALETTE 05 */ { 0x5000, 0x0218, 0x40eb, 0x1000, 0x02fa, 0x00e8, 0x81e8, 0x0088, 0x80e8, 0x8008, 0x010e, 0x4000, 0x01e9, 0x02f6, 0x0006, 0x00d9 },
    /* PALETTE 06 */ { 0x41f1, 0x0204, 0x40e9, 0x01f1, 0x02fc, 0x00d9, 0x5001, 0x0204, 0x40e9, 0x1001, 0x02fc, 0x00e0, 0x81e8, 0x0084, 0x80e0, 0x8008 },
    /* PALETTE 07 */ { 0x0182, 0x0006, 0x00d9, 0x41f1, 0x0218, 0x40e9, 0x01f1, 0x02fa, 0x00d9, 0x5001, 0x0218, 0x40e9, 0x1001, 0x02fa, 0x00e0, 0x81e8 },
    /* PALETTE 08 */ { 0x0084, 0x80e0, 0x8008, 0x0182, 0x0006, 0x00d8, 0x41f1, 0x0204, 0x40e8, 0x01f1, 0x02fc, 0x00d8, 0x5001, 0x0204, 0x40e8, 0x1001 },
    /* PALETTE 09 */ { 0x02fc, 0x00e0, 0x81e8, 0x000c, 0x80e0, 0x8008, 0x0180, 0x0006, 0x00d8, 0x41f1, 0x0218, 0x40e8, 0x01f1, 0x02fa, 0x00d8, 0x5001 },
#endif // PLATFORM_PC
};
