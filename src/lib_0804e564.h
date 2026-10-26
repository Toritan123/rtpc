#pragma once

#include "global.h"

// Gyro/Rumble Library

struct struct_0300443c { // Gyro Status?
    u32 unk0;
    s32 unk4;
    u32 unk8;
    s32 unkC;
    u16 unk10;
    u16 unk12;
    u32 unk14;
    u32 unk18;
    u32 unk1C;
    u8 unk20;
    u8 unk21;
    u16 unk22;
};

#define REG_GPIO_DATA *(volatile u16 *)(GameROMBase + 0xc4)
#define REG_GPIO_DIR *(volatile u16 *)(GameROMBase + 0xc6)
#define REG_GPIO_CNT *(volatile u16 *)(GameROMBase + 0xc8)

extern volatile u16 D_03004438; // GPIO Data Reserve
extern volatile u16 D_0300443a; // GPIO Direction Reserve
extern struct struct_0300443c *D_0300443c; // Gyro Status
extern u8 D_03004440; // Rumble Enabled
extern u32 D_03004448[6]; // Unused?

extern s32 (*D_030064d4)(void); // Read Gyro Data Function?

#ifdef PLATFORM_PC
// These three hold cartridge GPIO addresses (GameROMBase + 0xC4/C6/C8) — the
// rumble/gyro pak's registers.  A PC has no such hardware and the ROM address
// is unmapped, so writing through them faults; the reset-button combo in
// agb_main reaches func_08009548 -> the rumble write, which is why holding
// A+B+START+SELECT together used to crash the game.  Redirect the writes to a
// dummy word.  (The symbols cannot simply be given PC values: the same
// auto-generated names are already defined as graphics constants in
// graphics/gameplay/gameplay_unused_warioware_graphics.c, and the linker
// merges them.)
extern volatile u16 gPcGpioDummy[3];
#define D_08bd0cc8 (&gPcGpioDummy[0])
#define D_08bd0ccc (&gPcGpioDummy[1])
#define D_08bd0cd0 (&gPcGpioDummy[2])
#else
extern volatile u16 *D_08bd0cc8; // GPIO Data Pointer
extern volatile u16 *D_08bd0ccc; // GPIO Direction Pointer
extern volatile u16 *D_08bd0cd0; // GPIO Control Pointer
#endif
extern u8 D_08bd0cd4[4];


extern void func_0804e564(void); // Initialise GPIO
extern s32 func_0804e598(void); // Read Gyro Data
extern void func_0804e618(u32); // Enable Rumble
extern void func_0804e640(struct struct_0300443c *); // Initialise Gyro/Rumbl
extern void func_0804e690(u32);
extern void func_0804e6c4(u32);
extern void func_0804e6e4(void);
extern void func_0804e77c(void);
extern u32 func_0804e834(void);
extern u32 func_0804e870(u32 *);
extern void func_0804e8bc(s32 *);
extern void func_0804e8cc(u32);
extern void func_0804e8f8(u32); // Toggle Rumble
extern void func_0804e914(u32); // Turn off current Rumble, toggle Rumble
extern u32 func_0804e92c(void);

// ARM 
extern volatile s32 func_0804e938(volatile u16 *);
