/*
 * GBA PSG sound channels (PC).
 *
 * src/midi/psg.c drives the GBA's four Game-Boy-derived channels by writing
 * hardware registers at IORAMBase+0x60..0x84.  On PC those writes land in
 * plain memory and nothing ever read them, so every PSG note the game played
 * was silent.  This synthesises them from the register state.
 *
 * Channel map, as the GBA lays it out:
 *   1  square with frequency sweep   SOUND1CNT_L/H/X
 *   2  square                        SOUND2CNT_L/H
 *   3  4-bit wavetable               SOUND3CNT_L/H/X + wave RAM
 *   4  LFSR noise                    SOUND4CNT_L/H
 *
 * Sampled once per frame rather than per register write: the driver runs from
 * midi_sound_main, i.e. once per frame, so nothing is missed.  A note restart
 * is detected as "bit 15 set and the register value changed", because the
 * reset bit is write-only on hardware but sticks in memory here — without the
 * change test every frame would retrigger.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "gba_mem.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PSG_REG(off) (*(volatile u16 *)(gba_io + (off)))

#define R_S1CNT_L 0x60
#define R_S1CNT_H 0x62
#define R_S1CNT_X 0x64
#define R_S2CNT_L 0x68
#define R_S2CNT_H 0x6C
#define R_S3CNT_L 0x70
#define R_S3CNT_H 0x72
#define R_S3CNT_X 0x74
#define R_S4CNT_L 0x78
#define R_S4CNT_H 0x7C
#define R_SNDCNT_L 0x80
#define R_SNDCNT_H 0x82
#define R_SNDCNT_X 0x84
#define R_WAVE_RAM 0x90

// The GBA clocks the PSG from the same 16.78 MHz system clock as the DMG.
#define PSG_SQUARE_HZ(f)  (131072.0 / (2048.0 - (double)(f)))
#define PSG_WAVE_HZ(f)    (2097152.0 / (2048.0 - (double)(f)))

// 12.5 / 25 / 50 / 75 % duty, as eight-step patterns.
static const u8 kDuty[4][8] = {
    {0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {0,1,1,1,1,1,1,0},
};

struct PsgSquare {
    double   phase;        // 0..1 through the duty pattern
    u16      prev_freq_reg;
    u16      prev_env_reg;
    int      on;
    int      volume;       // current envelope level, 0..15
    int      env_dir;      // +1 / -1 / 0
    int      env_period;   // envelope steps, in 1/64 s units
    double   env_timer;    // seconds until the next envelope step
    double   length_left;  // seconds remaining when length is enabled
    int      length_on;
    // channel 1 only
    double   sweep_timer;
    int      sweep_period, sweep_dir, sweep_shift;
    int      sweep_freq;
};

struct PsgWave {
    double phase;
    u16    prev_freq_reg;
    int    on;
    int    volume_shift;   // 4 = mute, 0 = full, etc.
    double length_left;
    int    length_on;
};

struct PsgNoise {
    double phase_acc;
    u16    prev_freq_reg;
    u16    prev_env_reg;
    int    on;
    u16    lfsr;
    int    width7;
    double period;         // seconds per LFSR step
    int    volume, env_dir, env_period;
    double env_timer, length_left;
    int    length_on;
    int    out;            // last LFSR output, 0/1
};

static struct PsgSquare s_sq[2];
static struct PsgWave   s_wave;
static struct PsgNoise  s_noise;
static int              s_enabled = -1;
static double           s_psg_gain = 1.0;  // RTPC_PSG_VOLUME/100

void psg_pc_reset(void)
{
    memset(s_sq, 0, sizeof(s_sq));
    memset(&s_wave, 0, sizeof(s_wave));
    memset(&s_noise, 0, sizeof(s_noise));
    s_noise.lfsr = 0x7FFF;
    s_sq[0].prev_freq_reg = s_sq[1].prev_freq_reg = 0xFFFF;
    s_sq[0].prev_env_reg  = s_sq[1].prev_env_reg  = 0xFFFF;
    s_wave.prev_freq_reg  = 0xFFFF;
    s_noise.prev_freq_reg = s_noise.prev_env_reg = 0xFFFF;
}

// Envelope/length fields are laid out identically for channels 1, 2 and 4.
static void load_envelope(int env_reg, int *volume, int *dir, int *period)
{
    *volume = (env_reg >> 12) & 0xF;
    *dir    = (env_reg & (1 << 11)) ? +1 : -1;
    *period = (env_reg >> 8) & 0x7;
}

static void square_update_regs(struct PsgSquare *c, int env_reg, int freq_reg,
                               int is_ch1, int sweep_reg)
{
    int restarted = (freq_reg & 0x8000) && (freq_reg != c->prev_freq_reg);

    if (env_reg != c->prev_env_reg) {
        load_envelope(env_reg, &c->volume, &c->env_dir, &c->env_period);
        c->env_timer = (c->env_period ? c->env_period : 8) / 64.0;
        // Volume zero with a decreasing envelope means the channel is off.
        if (((env_reg >> 11) & 0x1F) == 0) c->on = 0;
        c->prev_env_reg = (u16)env_reg;
    }

    if (restarted) {
        load_envelope(env_reg, &c->volume, &c->env_dir, &c->env_period);
        c->env_timer   = (c->env_period ? c->env_period : 8) / 64.0;
        c->length_on   = (freq_reg & 0x4000) != 0;
        c->length_left = (64 - (env_reg & 0x3F)) / 256.0;
        c->phase       = 0.0;
        c->on          = 1;
        if (is_ch1) {
            c->sweep_period = (sweep_reg >> 4) & 0x7;
            c->sweep_dir    = (sweep_reg & 0x8) ? -1 : +1;
            c->sweep_shift  = sweep_reg & 0x7;
            c->sweep_freq   = freq_reg & 0x7FF;
            c->sweep_timer  = (c->sweep_period ? c->sweep_period : 8) / 128.0;
        }
    }
    c->prev_freq_reg = (u16)freq_reg;
}

/* Render `frames` stereo samples and ADD them into out[] (interleaved s16).
 * dt is one sample's duration in seconds. */
void psg_pc_render(int16_t *out, unsigned frames, int sample_rate)
{
    unsigned i;
    double dt = 1.0 / (double)sample_rate;
    int snd_l = PSG_REG(R_SNDCNT_L);
    int snd_h = PSG_REG(R_SNDCNT_H);
    int snd_x = PSG_REG(R_SNDCNT_X);
    int master_on = (snd_x & 0x80) != 0;
    int ratio = snd_h & 0x3;                 /* 0=25% 1=50% 2=100% */
    double master = (ratio == 0) ? 0.25 : (ratio == 1) ? 0.5 : 1.0;
    double vol_l = ((snd_l >> 4) & 0x7) / 7.0;
    double vol_r = ((snd_l >> 0) & 0x7) / 7.0;
    int sq_freq[2], wave_freq;

    if (s_enabled < 0) {
        const char *e = getenv("RTPC_NO_PSG");
        const char *v = getenv("RTPC_PSG_VOLUME");
        s_enabled = (e && atoi(e)) ? 0 : 1;
        s_psg_gain = v ? (atof(v) / 100.0) : 1.0;
        if (s_psg_gain < 0.0) s_psg_gain = 0.0;
        if (s_psg_gain > 4.0) s_psg_gain = 4.0;
        psg_pc_reset();
    }

    // RTPC_PSG_TEST=<freq reg>: drive channel 2 directly with a sustained
    // square, so the synthesiser can be checked without finding a scene that
    // uses a PSG instrument bank.  1748 -> 131072/(2048-1748) = 436.9 Hz.
    if (getenv("RTPC_PSG_TEST")) {
        int f = atoi(getenv("RTPC_PSG_TEST"));
        PSG_REG(R_SNDCNT_X) |= 0x80;
        PSG_REG(R_SNDCNT_H) = (u16)((PSG_REG(R_SNDCNT_H) & ~0x3) | 2);
        PSG_REG(R_SNDCNT_L) = 0x2277;          /* ch2 on both sides, full vol */
        PSG_REG(R_S2CNT_L)  = (u16)((15 << 12) | (2 << 6));  /* vol 15, 50% duty */
        if (!s_sq[1].on) PSG_REG(R_S2CNT_H) = (u16)(0x8000 | (f & 0x7FF));
        snd_l = PSG_REG(R_SNDCNT_L); snd_h = PSG_REG(R_SNDCNT_H);
        snd_x = PSG_REG(R_SNDCNT_X); master_on = 1;
        ratio  = snd_h & 0x3;
        master = (ratio == 0) ? 0.25 : (ratio == 1) ? 0.5 : 1.0;
        vol_l  = ((snd_l >> 4) & 0x7) / 7.0;
        vol_r  = ((snd_l >> 0) & 0x7) / 7.0;
    }
    if (getenv("RTPC_AUDIO_STATS")) {
        static unsigned m = 0;
        if ((m++ % 120) == 0)
            fprintf(stderr, "[PSG] regs cntL=%04x cntH=%04x cntX=%04x | "
                            "1:%04x/%04x 2:%04x/%04x 3:%04x/%04x 4:%04x/%04x\n",
                    snd_l, snd_h, snd_x,
                    PSG_REG(R_S1CNT_H), PSG_REG(R_S1CNT_X),
                    PSG_REG(R_S2CNT_L), PSG_REG(R_S2CNT_H),
                    PSG_REG(R_S3CNT_H), PSG_REG(R_S3CNT_X),
                    PSG_REG(R_S4CNT_L), PSG_REG(R_S4CNT_H));
    }
    if (!s_enabled || !master_on) return;

    square_update_regs(&s_sq[0], PSG_REG(R_S1CNT_H), PSG_REG(R_S1CNT_X), 1,
                       PSG_REG(R_S1CNT_L));
    square_update_regs(&s_sq[1], PSG_REG(R_S2CNT_L), PSG_REG(R_S2CNT_H), 0, 0);

    /* Wave channel */
    {
        int freq_reg = PSG_REG(R_S3CNT_X);
        int ctl_h    = PSG_REG(R_S3CNT_H);
        int on_reg   = PSG_REG(R_S3CNT_L);
        if ((freq_reg & 0x8000) && freq_reg != s_wave.prev_freq_reg) {
            s_wave.phase       = 0.0;
            s_wave.on          = (on_reg & 0x80) != 0;
            s_wave.length_on   = (freq_reg & 0x4000) != 0;
            s_wave.length_left = (256 - (ctl_h & 0xFF)) / 256.0;
        }
        if (!(on_reg & 0x80)) s_wave.on = 0;
        /* bits 13-14: 0 mute, 1 full, 2 half, 3 quarter; bit 15 forces 75% */
        {
            int lv = (ctl_h >> 13) & 0x3;
            s_wave.volume_shift = (ctl_h & 0x8000) ? 0 : (lv == 0 ? 4 : lv - 1);
        }
        s_wave.prev_freq_reg = (u16)freq_reg;
        wave_freq = freq_reg & 0x7FF;
    }

    /* Noise channel */
    {
        int env_reg  = PSG_REG(R_S4CNT_L);
        int freq_reg = PSG_REG(R_S4CNT_H);
        int r = freq_reg & 0x7, sft = (freq_reg >> 4) & 0xF;
        double div = (r == 0) ? 0.5 : (double)r;
        s_noise.period = (div * (double)(1 << (sft + 1))) / 524288.0;
        s_noise.width7 = (freq_reg & 0x8) != 0;
        if (env_reg != s_noise.prev_env_reg) {
            load_envelope(env_reg, &s_noise.volume, &s_noise.env_dir, &s_noise.env_period);
            s_noise.env_timer = (s_noise.env_period ? s_noise.env_period : 8) / 64.0;
            if (((env_reg >> 11) & 0x1F) == 0) s_noise.on = 0;
            s_noise.prev_env_reg = (u16)env_reg;
        }
        if ((freq_reg & 0x8000) && freq_reg != s_noise.prev_freq_reg) {
            load_envelope(env_reg, &s_noise.volume, &s_noise.env_dir, &s_noise.env_period);
            s_noise.env_timer   = (s_noise.env_period ? s_noise.env_period : 8) / 64.0;
            s_noise.length_on   = (freq_reg & 0x4000) != 0;
            s_noise.length_left = (64 - (env_reg & 0x3F)) / 256.0;
            s_noise.lfsr        = s_noise.width7 ? 0x7F : 0x7FFF;
            s_noise.on          = 1;
        }
        s_noise.prev_freq_reg = (u16)freq_reg;
    }

    sq_freq[0] = s_sq[0].on && s_sq[0].sweep_shift ? s_sq[0].sweep_freq
                                                   : (PSG_REG(R_S1CNT_X) & 0x7FF);
    sq_freq[1] = PSG_REG(R_S2CNT_H) & 0x7FF;

    if (getenv("RTPC_AUDIO_STATS")) {
        static unsigned n = 0;
        if ((n++ % 60) == 0)
            fprintf(stderr, "[PSG] on: sq1=%d sq2=%d wave=%d noise=%d  vol=%d/%d\n",
                    s_sq[0].on, s_sq[1].on, s_wave.on, s_noise.on,
                    s_sq[0].volume, s_sq[1].volume);
    }

    for (i = 0; i < frames; i++) {
        double l = 0.0, r = 0.0;
        int ch;

        for (ch = 0; ch < 2; ch++) {
            struct PsgSquare *c = &s_sq[ch];
            int duty, step;
            double f, amp;
            if (!c->on) continue;

            if (c->length_on) {
                c->length_left -= dt;
                if (c->length_left <= 0.0) { c->on = 0; continue; }
            }
            if (c->env_period) {
                c->env_timer -= dt;
                if (c->env_timer <= 0.0) {
                    c->env_timer += c->env_period / 64.0;
                    c->volume += c->env_dir;
                    if (c->volume < 0)  { c->volume = 0;  c->on = 0; }
                    if (c->volume > 15) c->volume = 15;
                }
            }
            if (ch == 0 && c->sweep_period && c->sweep_shift) {
                c->sweep_timer -= dt;
                if (c->sweep_timer <= 0.0) {
                    c->sweep_timer += c->sweep_period / 128.0;
                    c->sweep_freq += c->sweep_dir * (c->sweep_freq >> c->sweep_shift);
                    if (c->sweep_freq > 2047) { c->sweep_freq = 2047; c->on = 0; }
                    if (c->sweep_freq < 0)      c->sweep_freq = 0;
                    sq_freq[0] = c->sweep_freq;
                }
            }

            if (sq_freq[ch] >= 2048) continue;
            f = PSG_SQUARE_HZ(sq_freq[ch]);
            duty = (ch == 0 ? (PSG_REG(R_S1CNT_H) >> 6) : (PSG_REG(R_S2CNT_L) >> 6)) & 0x3;
            c->phase += f * dt;
            c->phase -= (double)(long)c->phase;
            step = (int)(c->phase * 8.0) & 7;
            amp  = (kDuty[duty][step] ? 1.0 : -1.0) * (c->volume / 15.0);

            if (snd_l & (1 << (8 + ch)))  l += amp;   /* left enable  bits 8-11 */
            if (snd_l & (1 << (12 + ch))) r += amp;   /* right enable bits 12-15 */
        }

        if (s_wave.on && wave_freq < 2048) {
            const u8 *wram = (const u8 *)(gba_io + R_WAVE_RAM);
            int idx, nib;
            double amp;
            if (s_wave.length_on) {
                s_wave.length_left -= dt;
                if (s_wave.length_left <= 0.0) s_wave.on = 0;
            }
            s_wave.phase += PSG_WAVE_HZ(wave_freq) / 32.0 * dt;
            s_wave.phase -= (double)(long)s_wave.phase;
            idx = (int)(s_wave.phase * 32.0) & 31;
            nib = (idx & 1) ? (wram[idx >> 1] & 0xF) : (wram[idx >> 1] >> 4);
            amp = ((nib - 8) / 8.0) / (double)(1 << s_wave.volume_shift);
            if (snd_l & (1 << 10)) l += amp;
            if (snd_l & (1 << 14)) r += amp;
        }

        if (s_noise.on) {
            double amp;
            if (s_noise.length_on) {
                s_noise.length_left -= dt;
                if (s_noise.length_left <= 0.0) s_noise.on = 0;
            }
            if (s_noise.env_period) {
                s_noise.env_timer -= dt;
                if (s_noise.env_timer <= 0.0) {
                    s_noise.env_timer += s_noise.env_period / 64.0;
                    s_noise.volume += s_noise.env_dir;
                    if (s_noise.volume < 0)  { s_noise.volume = 0; s_noise.on = 0; }
                    if (s_noise.volume > 15) s_noise.volume = 15;
                }
            }
            s_noise.phase_acc += dt;
            while (s_noise.period > 0.0 && s_noise.phase_acc >= s_noise.period) {
                u16 x;
                s_noise.phase_acc -= s_noise.period;
                x = (u16)((s_noise.lfsr ^ (s_noise.lfsr >> 1)) & 1);
                s_noise.lfsr >>= 1;
                s_noise.lfsr |= (u16)(x << (s_noise.width7 ? 6 : 14));
                s_noise.out = !(s_noise.lfsr & 1);
            }
            amp = (s_noise.out ? 1.0 : -1.0) * (s_noise.volume / 15.0);
            if (snd_l & (1 << 11)) l += amp;
            if (snd_l & (1 << 15)) r += amp;
        }

        // Four channels each in [-1,1], summed, so a quarter each puts all
        // four at full tilt exactly at full scale — the DMG's own scaling.
        // (This was 0.18, an unjustified safety margin picked before the
        // master limiter existed; combined with the uninitialised 25 % output
        // ratio it left PSG notes about 15 dB below where they belong.)
        l *= master * vol_l * 0.25 * s_psg_gain;
        r *= master * vol_r * 0.25 * s_psg_gain;
        {
            int sl = out[i * 2 + 0] + (int)(l * 32767.0);
            int sr = out[i * 2 + 1] + (int)(r * 32767.0);
            if (sl >  32767) sl =  32767;
            if (sl < -32768) sl = -32768;
            if (sr >  32767) sr =  32767;
            if (sr < -32768) sr = -32768;
            out[i * 2 + 0] = (int16_t)sl;
            out[i * 2 + 1] = (int16_t)sr;
        }
    }
}

#endif // PLATFORM_PC
