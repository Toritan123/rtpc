/*
 * Definitions for game-state global variables that live in EWRAM/IWRAM on GBA
 * but have no C definition in the decompiled source.  On PC they are simply
 * zero-initialised static data.
 */
#ifdef PLATFORM_PC

#include <stdio.h>
#include "global.h"
#include "include/types.h"
#include "include/scenes.h"
#include "src/code_0800b778.h"
#include "src/code_08001360.h"
#include "src/scenes/cafe.h"
#include "src/scenes/riq_main_scene.h"
#include "src/scenes/warning.h"
#include "src/scenes/title.h"
#include "src/code_080068f8.h"
#include "include/sequence_data.h"
#include "src/scenes/main_menu.h"
#include "src/scenes/game_select.h"
#include "src/scenes/options.h"
#include "src/scenes/data_room.h"
#include "src/scenes/gameplay.h"
#include "src/scenes/results.h"
#include "src/memory.h"
#include "include/levels.h"

// Forward-declare game engines (defined in games/*/engine.c).
extern struct GameEngine karate_man_engine;

// [0x030055A0] Play Session info – written by cafe_scene_init_memory
// moved to src/global.c upstream: struct PlaySessionInfo gSessionInfo = {{0}};

// Pause menu state – written by the pause handler
// moved to src/global.c upstream: struct PauseMenu gPauseMenu = {0};

// Scene descriptors – used as targets for set_next_scene().

// ─── Beatscript data stubs ────────────────────────────────────────────────────
// On GBA these are ROM arrays; on PC we provide immediate-STOP sentinels.
struct Beatscript epilogue_end_script[1]            = {{ BS_CMD_STOP }};
// [D_089cfda4] Generic gameplay fade-out sequence. The beatscript engine jumps
// here (func_0801d968) when a game ends; the previous const u8[4] definition
// meant the engine then executed whatever globals followed it in memory.
struct Beatscript D_089cfda4[1]                     = {{ BS_CMD_STOP }};
// [D_089e2ad4] / [D_089e2b04] – Drum Studio scripts. Previously defined as
// const u8[4] while declared as struct Beatscript[]: any walk of these would
// have read past the end of the object.
struct Beatscript D_089e2ad4[1]                     = {{ BS_CMD_STOP }};
struct Beatscript D_089e2b04[1]                     = {{ BS_CMD_STOP }};


// The beatscript data stubs above must be strong *data* definitions.
// auto_stubs.c declares them weak *functions*; when one of those won, the
// scene manager read the function's instruction bytes as a struct Scene and
// crashed.


// ─── PC Beatscript Helpers ────────────────────────────────────────────────────
// Macros for constructing PC-native beatscript arrays.
// The Beatscript struct fields:
//   command (u8), param1 (u8), src (const void*), param3 (u32)
// Commands:
//   0x00 REST N      — wait N beat-ticks (1 beat = 24 ticks)
//   0x01 STOP        — end thread
//   0x03 RUN fn,arg  — call fn(arg);  fn stored in src, arg in param3
//   0x0D CALL scr    — push currentCmd+1, jump to scr (stored in src)
//   0x0E RETURN      — pop from jump stack, resume caller
//   0x0F GOTO scr    — unconditional jump to scr (no push)
//   0x10 LOOP_START  — mark loop start
//   0x11 LOOP_END    — jump to loop start, or exit if bypassLoops is set
//   0x4C TEMPO bpm   — call set_beatscript_tempo(bpm)

// Beatscript struct on PC: { command:8|param1:24, param2 (const void*), param3 (uintptr_t) }
#define PC_BS_REST(n)       { BS_CMD_REST, 0, NULL, (n) }
#define PC_BS_STOP          { BS_CMD_STOP, 0, NULL, 0 }
#define PC_BS_RUN(fn, arg)  { 0x03, 0, (const void*)(fn), (uintptr_t)(arg) }
#define PC_BS_CALL(scr)     { 0x0D, 0, (const void*)(scr), 0 }
#define PC_BS_RETURN        { 0x0E, 0, NULL, 0 }
#define PC_BS_GOTO(scr)     { 0x0F, 0, (const void*)(scr), 0 }
#define PC_BS_LOOP_START    { 0x10, 0, NULL, 0 }
#define PC_BS_LOOP_END      { 0x11, 0, NULL, 0 }
#define PC_BS_TEMPO(bpm)    { 0x4C, 0, NULL, (bpm) }




#endif // PLATFORM_PC



