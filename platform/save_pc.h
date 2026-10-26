/*
 * Save-file persistence for the PC port.
 *
 * The GBA keeps its save in battery-backed cartridge SRAM. Here `gba_sram` is
 * just a 64 KB array, so without this it starts blank on every launch and the
 * game asks you to sit through the rhythm test again each time.
 */
#pragma once

#ifdef PLATFORM_PC

// Fill gba_sram from disk. Leaves it zeroed when there is no save yet.
void rtpc_sram_load(void);

// Note that gba_sram changed; the next rtpc_sram_flush() will write it out.
void rtpc_sram_mark_dirty(void);

// Write gba_sram to disk if it changed since the last flush. Cheap when clean.
void rtpc_sram_flush(void);

#endif
