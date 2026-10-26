/*
 * Replacement samples (pc-enhanced branch, audio_quality=high only).
 *
 * Every sound the game makes -- instruments and sound effects alike -- is
 * one of the 962 samples in platform/sample_data.c (sample_001 ..
 * sample_962), started through midi_sampler_load. A file named
 * sample_NNN.wav in the samples folder (next to settings.ini, or
 * RTPC_SAMPLE_DIR) is played instead of sample NNN: mono or stereo (mixed
 * down), 16/24-bit PCM or 32-bit float, any sample rate. It keeps the
 * original's pitch: a file at a different rate is stepped through faster or
 * slower in proportion, so a 48 kHz upsample of a 13379 Hz original plays at
 * the same pitch. Loop points come from the file's `smpl` chunk when it has
 * one, otherwise from the original's, scaled by the length ratio. Files
 * up to 2^23 samples (about 3 minutes at 48 kHz) are accepted.
 *
 * RTPC_SAMPLE_EXPORT=<dir> writes every original out as sample_NNN.wav
 * (16-bit, at its own rate, with its loop in a `smpl` chunk) plus
 * samples.csv, and exits -- the starting point for editing or upscaling
 * them, since the shipped game has no audio/ folder.
 */
#ifdef PLATFORM_PC

#include "global.h"
#include "sample_replace.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct SampleData;
extern struct SampleData *const gPcSampleTable[];
extern const unsigned gPcSampleCount;
extern const char *pc_settings_get(const char *key);
extern int pc_audio_hq(void);

// The generated file's own struct (platform/sample_data.c); same layout as
// include/sound.h's.
struct PcSampleData {
    u32 length, sampleRate, pitch, loopStart, loopEnd;
    const u32 *waveform;
};

static struct PcReplacement *s_repl;   // indexed by sample number - 1
static struct { const void *ptr; unsigned idx; } *s_sorted;
static unsigned s_loaded;

static u32 rd32(const u8 *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
static u16 rd16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }

// Parse a RIFF WAVE file into mono s16. Returns 0 on success.
static int load_wav(const char *path, struct PcReplacement *r, int *has_loop)
{
    FILE *f = fopen(path, "rb");
    long size;
    u8 *buf, *p, *end, *fmt = NULL, *data = NULL;
    u32 data_len = 0;
    int ok = -1;

    *has_loop = 0;
    if (!f) return -1;
    fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
    if (size < 44 || !(buf = (u8 *)malloc(size))) { fclose(f); return -1; }
    if (fread(buf, 1, size, f) != (size_t)size || memcmp(buf, "RIFF", 4) || memcmp(buf + 8, "WAVE", 4)) goto out;
    for (p = buf + 12, end = buf + size; p + 8 <= end; ) {
        u32 len = rd32(p + 4);
        if (p + 8 + len > end) len = (u32)(end - p - 8);
        if (!memcmp(p, "fmt ", 4) && len >= 16) fmt = p + 8;
        else if (!memcmp(p, "data", 4)) { data = p + 8; data_len = len; }
        else if (!memcmp(p, "smpl", 4) && len >= 36 + 24 && rd32(p + 8 + 28) > 0) {
            r->loopStart = rd32(p + 8 + 36 + 8);
            r->loopEnd   = rd32(p + 8 + 36 + 12) + 1;   // smpl's end is inclusive
            *has_loop = 1;
        }
        p += 8 + len + (len & 1);
    }
    if (!fmt || !data) goto out;
    {
        u16 format = rd16(fmt), ch = rd16(fmt + 2), bits = rd16(fmt + 14);
        u32 rate = rd32(fmt + 4), frame = ch * (bits / 8), n, i;
        if (format == 0xFFFE) format = rd16(fmt + 24);   // WAVE_FORMAT_EXTENSIBLE
        if (!ch || !frame || !rate || !((format == 1 && (bits == 16 || bits == 24)) || (format == 3 && bits == 32)))
            goto out;
        n = data_len / frame;
        r->data = (s16 *)malloc((n + 4) * sizeof(s16));
        if (!r->data) goto out;
        for (i = 0; i < n; i++) {
            double acc = 0.0;
            u16 c;
            for (c = 0; c < ch; c++) {
                const u8 *s = data + i * frame + c * (bits / 8);
                if (format == 3) { float v; memcpy(&v, s, 4); acc += v * 32767.0; }
                else if (bits == 16) acc += (s16)rd16(s);
                else acc += (double)((s32)((s[0] << 8) | (s[1] << 16) | ((u32)s[2] << 24)) >> 8) / 256.0;
            }
            acc /= ch;
            r->data[i] = (s16)(acc > 32767 ? 32767 : acc < -32768 ? -32768 : acc);
        }
        memset(r->data + n, 0, 4 * sizeof(s16));
        r->length = n;
        r->rate = rate;
        ok = 0;
    }
out:
    free(buf);
    fclose(f);
    return ok;
}

static int cmp_ptr(const void *a, const void *b)
{
    const void *x = *(const void *const *)a, *y = *(const void *const *)b;
    return x < y ? -1 : x > y;
}

static void sample_dir(char *out, size_t n)
{
    const char *e = getenv("RTPC_SAMPLE_DIR");
    out[0] = '\0';
    if (e) { snprintf(out, n, "%s/", e); return; }
    if (!getenv("RTPC_SAVE")) {
        char *pref = SDL_GetPrefPath(NULL, "Rhythm Tengoku");
        if (pref) { snprintf(out, n, "%ssamples/", pref); SDL_free(pref); }
    }
}

static void export_all(const char *dir)
{
    char path[1024];
    FILE *csv;
    unsigned i;
    snprintf(path, sizeof(path), "%s/samples.csv", dir);
    csv = fopen(path, "w");
    if (!csv) { fprintf(stderr, "[SAMPLES] cannot write %s\n", path); exit(1); }
    fprintf(csv, "file,rate,pitch,length,loop_start,loop_end\n");
    for (i = 0; i < gPcSampleCount; i++) {
        const struct PcSampleData *sd = (const struct PcSampleData *)gPcSampleTable[i];
        const s8 *w = (const s8 *)sd->waveform;
        u32 n = sd->length + 1, k, data = n * 2;
        int loops = (sd->loopStart | sd->loopEnd) != 0;
        u32 smpl = loops ? 36 + 24 : 0;
        u8 hdr[44];
        FILE *f;
        snprintf(path, sizeof(path), "%s/sample_%03u.wav", dir, i + 1);
        if (!(f = fopen(path, "wb"))) continue;
        memcpy(hdr, "RIFF", 4);
#define W32(p, v) do { u32 _v = (v); (p)[0] = _v; (p)[1] = _v >> 8; (p)[2] = _v >> 16; (p)[3] = _v >> 24; } while (0)
#define W16(p, v) do { u32 _v = (v); (p)[0] = _v; (p)[1] = _v >> 8; } while (0)
        W32(hdr + 4, 36 + data + (smpl ? 8 + smpl : 0));
        memcpy(hdr + 8, "WAVEfmt ", 8);
        W32(hdr + 16, 16); W16(hdr + 20, 1); W16(hdr + 22, 1);
        W32(hdr + 24, sd->sampleRate); W32(hdr + 28, sd->sampleRate * 2);
        W16(hdr + 32, 2); W16(hdr + 34, 16);
        memcpy(hdr + 36, "data", 4); W32(hdr + 40, data);
        fwrite(hdr, 1, 44, f);
        for (k = 0; k < n; k++) { u8 s[2]; W16(s, (u16)(s16)(w[k] * 256)); fwrite(s, 1, 2, f); }
        if (smpl) {
            u8 c[8 + 36 + 24] = {0};
            memcpy(c, "smpl", 4); W32(c + 4, smpl);
            W32(c + 8 + 8, (u32)(1e9 / sd->sampleRate));    // sample period, ns
            W32(c + 8 + 12, sd->pitch);                     // MIDI unity note
            W32(c + 8 + 28, 1);                             // one loop
            W32(c + 8 + 36 + 8, sd->loopStart);
            W32(c + 8 + 36 + 12, sd->loopEnd - 1);          // inclusive end
            fwrite(c, 1, sizeof(c), f);
        }
#undef W32
#undef W16
        fclose(f);
        fprintf(csv, "sample_%03u.wav,%u,%u,%u,%u,%u\n", i + 1, sd->sampleRate, sd->pitch, n,
                loops ? sd->loopStart : 0, loops ? sd->loopEnd : 0);
    }
    fclose(csv);
    fprintf(stderr, "[SAMPLES] exported %u samples to %s\n", gPcSampleCount, dir);
    exit(0);
}

void pc_samples_init(void)
{
    char dir[1024], path[1100];
    unsigned i;
    const char *e = getenv("RTPC_SAMPLE_EXPORT");

    if (e) export_all(e);
    if (!pc_audio_hq()) return;
    sample_dir(dir, sizeof(dir));
    if (!dir[0]) return;
    s_repl = (struct PcReplacement *)calloc(gPcSampleCount, sizeof(*s_repl));
    s_sorted = calloc(gPcSampleCount, sizeof(*s_sorted));
    if (!s_repl || !s_sorted) return;
    for (i = 0; i < gPcSampleCount; i++) {
        const struct PcSampleData *sd = (const struct PcSampleData *)gPcSampleTable[i];
        struct PcReplacement *r = &s_repl[i];
        int has_loop;
        s_sorted[i].ptr = sd;
        s_sorted[i].idx = i;
        snprintf(path, sizeof(path), "%ssample_%03u.wav", dir, i + 1);
        if (load_wav(path, r, &has_loop) != 0) { r->data = NULL; continue; }
        r->ratio = (double)r->rate / (double)sd->sampleRate;
        if (!has_loop) {
            if ((sd->loopStart | sd->loopEnd) != 0) {
                double k = (double)r->length / (double)(sd->length + 1);
                r->loopStart = (u32)lround(sd->loopStart * k);
                r->loopEnd   = (u32)lround(sd->loopEnd * k);
            } else {
                r->loopStart = r->loopEnd = r->length;
            }
        }
        // Stream positions are 32-bit fixed point, Q18.14 for the originals
        // (262143 samples). A longer file -- 5.4 s at 48 kHz -- gets fewer
        // fraction bits instead, keeping one bit of headroom for the step.
        r->fbits = 14;
        while (r->fbits > 8 && r->length >= (1u << (31 - r->fbits))) r->fbits--;
        if (r->length >= (1u << (31 - r->fbits))) {
            fprintf(stderr, "[SAMPLES] %s: %u samples is too long; skipped\n", path, r->length);
            free(r->data); r->data = NULL;
            continue;
        }
        if (r->loopEnd > r->length) r->loopEnd = r->length;
        if (r->loopStart > r->loopEnd) r->loopStart = r->loopEnd;
        s_loaded++;
    }
    qsort(s_sorted, gPcSampleCount, sizeof(*s_sorted), cmp_ptr);
    if (s_loaded) fprintf(stderr, "[SAMPLES] %u replacement samples from %s\n", s_loaded, dir);
}

const struct PcReplacement *pc_sample_replacement(const void *sample_data)
{
    unsigned lo = 0, hi;
    if (!s_loaded) return NULL;
    hi = gPcSampleCount;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (s_sorted[mid].ptr == sample_data) {
            const struct PcReplacement *r = &s_repl[s_sorted[mid].idx];
            return r->data ? r : NULL;
        }
        if ((const char *)s_sorted[mid].ptr < (const char *)sample_data) lo = mid + 1; else hi = mid;
    }
    return NULL;
}

#endif
