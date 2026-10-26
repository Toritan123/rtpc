/*
 * Replacement samples (pc-enhanced branch). See sample_replace.c.
 */
#pragma once

#ifdef PLATFORM_PC

#include <stdint.h>

struct PcReplacement {
    int16_t *data;          // mono, plus 4 zero samples of padding
    uint32_t length;        // samples
    uint32_t rate;          // Hz
    uint32_t loopStart;     // samples; loopEnd == loopStart: no loop
    uint32_t loopEnd;
    double   ratio;         // rate / the original's sampleRate
    int      fbits;         // fraction bits of the stream position (14, fewer
                            // for files too long for Q18.14)
};

// Load the replacement folder (and handle RTPC_SAMPLE_EXPORT). Call once,
// before the sound engine starts.
void pc_samples_init(void);

// The replacement for this struct SampleData, or NULL.
const struct PcReplacement *pc_sample_replacement(const void *sample_data);

#endif
