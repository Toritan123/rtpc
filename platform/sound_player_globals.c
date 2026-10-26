/*
 * Definitions for MIDI sound player pool structures.
 * audio/sound_players.inc.c declares these as extern; auto_stubs.c
 * incorrectly defined them as function stubs. Provide correct data here.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "src/midi/midi.h"

#define N_TRACKS_MUS0   15
#define N_TRACKS_MUS1   12
#define N_TRACKS_MUS2   12
#define N_TRACKS_SFX    5

struct MidiChannel sMusicPlayer0Channels[N_TRACKS_MUS0] = {{0}};
struct MidiBus     sMusicPlayer0MidiBus                 = {0};
struct MidiTrackStream sMusicPlayer0Streams[N_TRACKS_MUS0] = {{0}};

struct MidiChannel sMusicPlayer1Channels[N_TRACKS_MUS1] = {{0}};
struct MidiBus     sMusicPlayer1MidiBus                 = {0};
struct MidiTrackStream sMusicPlayer1Streams[N_TRACKS_MUS1] = {{0}};

struct MidiChannel sMusicPlayer2Channels[N_TRACKS_MUS2] = {{0}};
struct MidiBus     sMusicPlayer2MidiBus                 = {0};
struct MidiTrackStream sMusicPlayer2Streams[N_TRACKS_MUS2] = {{0}};

struct MidiChannel sSfxPlayer0Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer0MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer0Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer1Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer1MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer1Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer2Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer2MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer2Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer3Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer3MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer3Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer4Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer4MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer4Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer5Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer5MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer5Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer6Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer6MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer6Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer7Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer7MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer7Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer8Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer8MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer8Streams[N_TRACKS_SFX] = {{0}};

struct MidiChannel sSfxPlayer9Channels[N_TRACKS_SFX] = {{0}};
struct MidiBus     sSfxPlayer9MidiBus                 = {0};
struct MidiTrackStream sSfxPlayer9Streams[N_TRACKS_SFX] = {{0}};

#endif // PLATFORM_PC
