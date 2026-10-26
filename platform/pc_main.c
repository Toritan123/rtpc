#ifdef PLATFORM_PC
#include "platform.h"
#include "gba_mem.h"
#include "save_pc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#if defined(__APPLE__) || defined(__linux__)
#  define RTPC_HAVE_BACKTRACE 1
#  include <execinfo.h>
#endif

static void crash_handler(int sig) {
    fprintf(stderr, "[CRASH] signal %d\n", sig);
#ifdef RTPC_HAVE_BACKTRACE
    {
        void *bt[32];
        int n = backtrace(bt, 32);
        backtrace_symbols_fd(bt, n, 2);
    }
#endif
    _Exit(139);   // async-signal-safe; C99 <stdlib.h>, unlike POSIX _exit
}

// Declared in src/main.c / src/main.h
extern void agb_main(void);
// Declared in platform/midi_globals.c
extern void midi_globals_init(void);
// Declared in platform/gfx_data.c — sets rleSize for all CompressedData objects
// so pc_rle_decompress can copy the correct number of bytes.
extern void gfx_data_init_sizes(void);

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
#ifdef SIGBUS   /* not defined by the Windows CRT */
    signal(SIGBUS,  crash_handler);
#endif

    // Zero out all GBA memory regions
    memset(gba_ewram,   0, sizeof(gba_ewram));
    memset(gba_iwram,   0, sizeof(gba_iwram));
    memset(gba_io,      0, sizeof(gba_io));
    memset(gba_palette, 0, sizeof(gba_palette));
    memset(gba_vram,    0, sizeof(gba_vram));
    memset(gba_oam,     0, sizeof(gba_oam));
    memset(gba_sram,    0, sizeof(gba_sram));
    rtpc_sram_load();   // restore the save, if there is one

    midi_globals_init();
    gfx_data_init_sizes();  // set rleSize on all CompressedData objects

    if (platform_init() != 0) {
        fprintf(stderr, "platform_init failed\n");
        return 1;
    }

    // The game loop lives inside agb_main().
    // platform_frame_sync() is called from the patched func_080013a8()
    // in src/code_08001360.c whenever the game waits for VBlank.
    agb_main();

    platform_destroy();
    return 0;
}

#endif // PLATFORM_PC
