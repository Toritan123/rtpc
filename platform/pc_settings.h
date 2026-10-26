/*
 * PC-only player settings (pc-enhanced branch): things the GBA never had,
 * kept in settings.ini next to the save.
 */
#pragma once

#ifdef PLATFORM_PC

// Read settings.ini (and RTPC_INPUT_OFFSET). Safe to call more than once;
// the first call does the work.
void pc_settings_init(void);

// audio_quality=high (default) or gba; RTPC_AUDIO_QUALITY overrides.
int pc_audio_hq(void);

// Raw access to settings.ini entries. get returns the default when the file
// does not set the key; set updates the value and rewrites the file.
const char *pc_settings_get(const char *key);
void pc_settings_set(const char *key, const char *value);

// Frames to subtract from a cue's running time when judging an input, so a
// player who presses in time with what they hear is judged on time even
// though the PC plays the sound later than the GBA did. Positive = the
// player is judged as having pressed earlier.
int pc_judge_offset_frames(void);

// Called once per frame: refreshes the automatic offset from the measured
// audio latency.
void pc_settings_frame(void);

// F8 = automatic offset, F9 / F10 = one frame less / more. Returns 1 if the
// key was one of these.
int pc_settings_handle_key(int sdl_keycode);

// Window title: judge offset, scaling and the hotkeys.
const char *pc_settings_title(void);

#endif
