/*
 * Modern reverb for audio_quality=high (pc-enhanced branch).
 *
 * The GBA's "reverb" (pc_midi_update_scratch) is a single feedback echo:
 * the finished DirectSound mix, read back from the output ring ~117 ms
 * later, high-passed (~130 Hz), low-passed (~530 Hz) and added in at
 * wet/128. This replaces it with a feedback delay network: a pre-delay,
 * four input diffusers (Dattorro's lengths) and eight damped delay lines
 * mixed by a Householder matrix, giving a dense tail instead of a train of
 * echoes.
 *
 * Its two parameters come from the game's own settings, so a game that
 * asks for more reverb (karate_man, space_dance, the studio) still gets
 * more:
 *  - decay: the GBA echo loses -20 log10(g) dB per pass of D seconds, with
 *    g = ((wet >> decay) << decay) / 128 and D the ring delay; RT60 is the
 *    time that takes to fall 60 dB.
 *  - level: chosen so the reverb carries the same energy, relative to the
 *    dry signal, as the GBA's does on pink noise (a stand-in for music,
 *    whose energy also falls with frequency). Both are measured by running
 *    the two reverbs on the same noise whenever the settings change.
 * reverb_mix (percent, settings.ini) scales the level on top.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern u32 gMidiReverb1Wet, gMidiReverb2Phase, gMidiReverb3Decay, gMidiReverb4LowCut;
extern const char *pc_settings_get(const char *key);

#define N_LINES   8
#define MAX_DELAY 4096
#define N_DIFF    4
#define PREDELAY_MAX 2048

static const int kLineLen[N_LINES] = { 1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116 };
static const int kDiffLen[N_DIFF] = { 210, 159, 562, 410 };      /* Dattorro's, at 44.1 kHz */
static const float kDiffG[N_DIFF] = { 0.75f, 0.75f, 0.625f, 0.625f };

struct Fdn {
    float line[N_LINES][MAX_DELAY];
    int   pos[N_LINES];
    float damp_state[N_LINES];
    float gain[N_LINES];
    float diff[N_DIFF][1024];
    int   dpos[N_DIFF];
    float pre[PREDELAY_MAX];
    int   ppos, predelay;
    float hp_state, lp_state, hp_a, lp_a, damp;
    float wet;
};

static struct Fdn s_fdn;
static u32 s_key = 0xFFFFFFFFu;
static int s_rate = 0;

static void fdn_reset(struct Fdn *f)
{
    memset(f->line, 0, sizeof(f->line));
    memset(f->pos, 0, sizeof(f->pos));
    memset(f->damp_state, 0, sizeof(f->damp_state));
    memset(f->diff, 0, sizeof(f->diff));
    memset(f->dpos, 0, sizeof(f->dpos));
    memset(f->pre, 0, sizeof(f->pre));
    f->ppos = 0;
    f->hp_state = f->lp_state = 0.0f;
}

static void fdn_setup(struct Fdn *f, double rt60, int rate)
{
    int i;
    double scale = rate / 44100.0;
    for (i = 0; i < N_LINES; i++) {
        double len = kLineLen[i] * scale;
        f->gain[i] = (float)pow(10.0, -3.0 * len / (rt60 * rate));
    }
    f->predelay = (int)(0.012 * rate);
    if (f->predelay >= PREDELAY_MAX) f->predelay = PREDELAY_MAX - 1;
    // Input band: high-pass ~150 Hz (no boomy tail), low-pass ~7 kHz; in the
    // loop, a gentle low-pass so highs die faster than lows, as in a room.
    f->hp_a = (float)exp(-2.0 * M_PI * 150.0 / rate);
    f->lp_a = (float)exp(-2.0 * M_PI * 7000.0 / rate);
    f->damp = 0.25f;
}

// One mono-in, stereo-out sample through the network (without the wet gain).
static inline void fdn_tick(struct Fdn *f, float in, float *ol, float *or_, int rate)
{
    float x, out[N_LINES], sum = 0.0f;
    double scale = rate / 44100.0;
    int i;

    // pre-delay
    f->pre[f->ppos] = in;
    x = f->pre[(f->ppos - f->predelay + PREDELAY_MAX) % PREDELAY_MAX];
    f->ppos = (f->ppos + 1) % PREDELAY_MAX;
    // band-limit the send
    f->hp_state = f->hp_a * f->hp_state + (1.0f - f->hp_a) * x;
    x -= f->hp_state;
    f->lp_state = f->lp_a * f->lp_state + (1.0f - f->lp_a) * x;
    x = f->lp_state;
    // input diffusion (Schroeder allpasses)
    for (i = 0; i < N_DIFF; i++) {
        int len = (int)(kDiffLen[i] * scale);
        int rp = (f->dpos[i] - len + 1024) % 1024;
        float d = f->diff[i][rp];
        float v = x + kDiffG[i] * d;
        f->diff[i][f->dpos[i]] = v;
        f->dpos[i] = (f->dpos[i] + 1) % 1024;
        x = d - kDiffG[i] * v;
    }
    // delay lines: read, damp, mix (Householder), write back with the input
    for (i = 0; i < N_LINES; i++) {
        int len = (int)(kLineLen[i] * scale);
        int rp = (f->pos[i] - len + MAX_DELAY) % MAX_DELAY;
        float v = f->line[i][rp];
        f->damp_state[i] = f->damp * f->damp_state[i] + (1.0f - f->damp) * v;
        out[i] = f->damp_state[i] * f->gain[i];
        sum += out[i];
    }
    sum *= 2.0f / N_LINES;
    for (i = 0; i < N_LINES; i++) {
        f->line[i][f->pos[i]] = out[i] - sum + x;
        f->pos[i] = (f->pos[i] + 1) % MAX_DELAY;
    }
    *ol  = out[0] - out[2] + out[4] - out[6];
    *or_ = out[1] - out[3] + out[5] - out[7];
}

// Pink noise (Paul Kellet's filter on white noise), deterministic.
static float pink(unsigned *seed, float *b)
{
    float w;
    *seed = *seed * 1664525u + 1013904223u;
    w = ((float)(*seed >> 8) / 8388608.0f) - 1.0f;
    b[0] = 0.99886f * b[0] + w * 0.0555179f;
    b[1] = 0.99332f * b[1] + w * 0.0750759f;
    b[2] = 0.96900f * b[2] + w * 0.1538520f;
    b[3] = 0.86650f * b[3] + w * 0.3104856f;
    b[4] = 0.55000f * b[4] + w * 0.5329522f;
    b[5] = -0.7616f * b[5] - w * 0.0168980f;
    {
        float p = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f;
        b[6] = w * 0.115926f;
        return p * 0.11f;
    }
}

// Energy of the GBA echo's output relative to its dry input, on pink noise:
// out = dry + g * LP(HP(out delayed by D)), at 13379 Hz, with the same
// one-pole sections as pc_midi_update_scratch.
static double gba_reverb_ratio(double g, int D, int lowCut, int decay)
{
    static float ring[4096];
    unsigned seed = 1, n, N = 13379 * 5 / 2;
    float b[7] = {0};
    double a1 = 1.0 - ldexp(1.0, -lowCut), a2 = 1.0 - ldexp(1.0, -decay);
    double lp1 = 0, lp2 = 0, ed = 0, er = 0;
    memset(ring, 0, sizeof(ring));
    for (n = 0; n < N; n++) {
        double dry = pink(&seed, b), x = ring[(n - D + 4096) % 4096], r;
        lp1 = a1 * lp1 + (1 - a1) * x;
        lp2 = a2 * lp2 + (1 - a2) * (x - lp1);
        r = g * lp2;
        ring[n % 4096] = (float)(dry + r);
        if (n > 13379 / 2) { ed += dry * dry; er += r * r; }
    }
    return ed > 0 ? er / ed : 0;
}

static double fdn_ratio(double rt60, int rate)
{
    static struct Fdn f;
    unsigned seed = 1, n, N = (unsigned)rate * 5 / 2;
    float b[7] = {0};
    double ed = 0, er = 0;
    fdn_reset(&f);
    fdn_setup(&f, rt60, rate);
    for (n = 0; n < N; n++) {
        float dry = pink(&seed, b), l, r;
        fdn_tick(&f, dry, &l, &r, rate);
        if (n > (unsigned)rate / 2) { ed += dry * dry; er += l * l; }
    }
    return ed > 0 ? er / ed : 0;
}

// configure() costs a few ms; games switch between a handful of settings,
// so remember the results.
static struct { u32 key; int rate; double rt60; float wet; } s_cache[8];
static int s_ncache;

static void configure(int rate)
{
    int decay = (int)gMidiReverb3Decay, lowCut = (int)gMidiReverb4LowCut;
    double g = (double)((gMidiReverb1Wet >> decay) << decay) / 128.0;
    int D = 1568 - 4 * (int)gMidiReverb2Phase;
    double mix = atof(pc_settings_get("reverb_mix")) / 100.0;
    double rt60, target, have;

    if (mix < 0) mix = 0;
    if (g <= 0.0 || D <= 0) { s_fdn.wet = 0.0f; return; }
    if (g > 0.95) g = 0.95;
    {
        u32 key = gMidiReverb1Wet | (gMidiReverb2Phase << 8) | (gMidiReverb3Decay << 16) | (gMidiReverb4LowCut << 24);
        int i;
        for (i = 0; i < s_ncache; i++) {
            if (s_cache[i].key == key && s_cache[i].rate == rate) {
                fdn_setup(&s_fdn, s_cache[i].rt60, rate);
                s_fdn.wet = (float)(s_cache[i].wet * mix);
                return;
            }
        }
    }
    rt60 = (D / 13379.0) * 3.0 / -log10(g);
    if (rt60 < 0.3) rt60 = 0.3;
    if (rt60 > 4.0) rt60 = 4.0;
    target = gba_reverb_ratio(g, D, lowCut, decay);
    have = fdn_ratio(rt60, rate);
    fdn_setup(&s_fdn, rt60, rate);
    s_fdn.wet = (float)(have > 0 ? sqrt(target / have) * mix : 0.0);
    if (s_ncache < 8) {
        s_cache[s_ncache].key  = gMidiReverb1Wet | (gMidiReverb2Phase << 8) | (gMidiReverb3Decay << 16) | (gMidiReverb4LowCut << 24);
        s_cache[s_ncache].rate = rate;
        s_cache[s_ncache].rt60 = rt60;
        s_cache[s_ncache].wet  = (float)(have > 0 ? sqrt(target / have) : 0.0);
        s_ncache++;
    }
    fprintf(stderr, "[REVERB] wet=%u phase=%u -> RT60 %.2f s, level %.1f dB below dry (x%.2f mix)\n",
            gMidiReverb1Wet, gMidiReverb2Phase, rt60, -10.0 * log10(target > 0 ? target : 1e-9), mix);
}

/* Add the reverb of buf (interleaved stereo, in place). */
void reverb_pc_process(float *buf, unsigned frames, int rate)
{
    u32 key = gMidiReverb1Wet | (gMidiReverb2Phase << 8) | (gMidiReverb3Decay << 16) | (gMidiReverb4LowCut << 24);
    unsigned i;
    if (rate != s_rate) { fdn_reset(&s_fdn); s_rate = rate; s_key = 0xFFFFFFFFu; }
    if (key != s_key) { configure(rate); s_key = key; }
    if (s_fdn.wet == 0.0f) return;
    for (i = 0; i < frames; i++) {
        float l, r;
        fdn_tick(&s_fdn, 0.5f * (buf[2 * i] + buf[2 * i + 1]), &l, &r, rate);
        buf[2 * i]     += s_fdn.wet * l;
        buf[2 * i + 1] += s_fdn.wet * r;
    }
}

#endif
