#pragma once
#ifdef PLATFORM_PC

// Initialise SDL2 audio, hooking into the MIDI library's PCM buffers.
// Call after midi_sound_init().
int  audio_pc_init(void);
void audio_pc_destroy(void);

// Call once per frame (after midi_sound_main()) to push newly rendered
// PCM samples from gMidiPCMBufR/L into the SDL2 audio queue.
void audio_pc_push_frame(void);
double audio_pc_queued_ms(void);
unsigned int audio_pc_underruns(void);
void audio_pc_init_dump_only(void);   // headless + RTPC_AUDIO_DUMP
int  audio_pc_dump_only(void);

#endif // PLATFORM_PC
