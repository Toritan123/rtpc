// mgba_ref: reference screenshots from the real ROM, for comparing the PC
// port's rendering against mGBA.
//
// Boots the ROM in libmgba, waits for the warning screen, redirects the
// warning -> title scene transition to the requested scene (by rewriting the
// target of the matching entry in the scene transition table in IWRAM),
// presses A to leave the warning, and then saves a PPM every N frames.
//
//   cc -arch x86_64 -I/usr/local/include tools/mgba_ref.c -L/usr/local/lib -lmgba -o mgba_ref
//   ./mgba_ref ROM SCENE_ADDR FRAMES EVERY OUTDIR [INPUTS]
//
// SCENE_ADDR is the ROM address of a struct Scene, e.g. 0x089d2c04 for
// scene_rat_race (taken from the game select level table). INPUTS, optional,
// is a file of "frame keys" lines (keys = GBA KEYINPUT bit mask, held from
// that frame on), with frames counted from the scene switch -- or an
// RTPC_KEYLOG file as it is ("frame keys epoch clock since"), which is
// replayed on the ROM's script clock instead of on frame numbers.
//
// To compare a PC run: record it with RTPC_KEYLOG=keys.txt and pass that
// file. The frame offset between PC shot n and mGBA frame n + k (rat_race:
// k = 28) is not constant: the ROM spends extra frames on each load, so
// compare each PC shot with the best-matching ref frame nearby. Anything driven by the RNG
// will differ: the GBA spins get_agb_random_var() while it waits for
// VBlank, so its RNG advances by an amount that depends on spare CPU time.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/internal/gba/gba.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_SCENE_PTR 0x08935fac // D_08935fac: pointer to the title scene
#define SCRIPT_CLOCK    0x030053d8 // D_030053c0.runningTime

// blip_buf, exported by libmgba but its header is not installed.
struct blip_t;
int  blip_samples_avail(const struct blip_t *);
int  blip_read_samples(struct blip_t *, short out[], int count, int stereo);
void blip_set_rates(struct blip_t *, double clock_rate, double sample_rate);

#define AUDIO_RATE 44100 // same as the PC build's output (PC_SAMPLE_RATE)

static void write_wav_header(FILE *f, uint32_t dataBytes)
{
    uint32_t riff = 36 + dataBytes, sz16 = 16, rate = AUDIO_RATE, byteRate = AUDIO_RATE * 4;
    uint16_t fmt = 1, ch = 2, bits = 16, align = 4;
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); fwrite(&sz16, 4, 1, f);
    fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
}

static void save_ppm(const char *path, const color_t *buf, unsigned w, unsigned h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned i = 0; i < w * h; i++) {
        uint32_t c = buf[i]; // mGBA XBGR8: 0x00BBGGRR
        unsigned char px[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };
        fwrite(px, 1, 3, f);
    }
    fclose(f);
}

// MGBA_REF_CLOCK logs the script clock every 20 frames; set
// MGBA_REF_CLOCK_EVERY=N to log every N frames (1 = every frame).
static int clock_every(void) {
    const char *e = getenv("MGBA_REF_CLOCK_EVERY");
    return e && atoi(e) > 0 ? atoi(e) : 20;
}

int main(int argc, char **argv)
{
    if (argc < 6) {
        fprintf(stderr, "usage: %s ROM SCENE_ADDR FRAMES EVERY OUTDIR [INPUTS]\n", argv[0]);
        return 1;
    }
    const char *rom = argv[1];
    uint32_t sceneAddr = strtoul(argv[2], NULL, 0);
    int frames = atoi(argv[3]), every = atoi(argv[4]);
    const char *outdir = argv[5];

    // Input lines are "frame keys" or, from RTPC_KEYLOG, "frame keys epoch
    // clock since". With the clock columns the keys are replayed when the
    // ROM's script clock reaches the logged point instead of on the logged
    // frame: the ROM spends extra frames on every load, so a frame offset
    // that fits the start of a game drifts by its end.
    static int inFrame[8192], inEpoch[8192], inClock[8192], inSince[8192];
    static uint32_t inKeys[8192];
    int nIn = 0, clocked = 0;
    if (argc > 6) {
        FILE *f = fopen(argv[6], "r");
        char line[128];
        while (f && nIn < 8192 && fgets(line, sizeof(line), f)) {
            int n = sscanf(line, "%d %i %d %d %d", &inFrame[nIn], (int *)&inKeys[nIn],
                           &inEpoch[nIn], &inClock[nIn], &inSince[nIn]);
            if (n == 5) clocked = 1;
            if (n >= 2) nIn++;
        }
        if (f) fclose(f);
    }

    struct mCore *core = mCoreFind(rom);
    if (!core || !core->init(core)) { fprintf(stderr, "no core\n"); return 1; }
    mCoreInitConfig(core, NULL);
    unsigned w, h;
    core->desiredVideoDimensions(core, &w, &h);
    color_t *buf = calloc(w * h, sizeof(color_t));
    core->setVideoBuffer(core, buf, w);
    if (!mCoreLoadFile(core, rom)) { fprintf(stderr, "cannot load %s\n", rom); return 1; }
    core->reset(core);

    uint32_t title = core->busRead32(core, TITLE_SCENE_PTR);
    int switched = 0, f0 = -1;
    for (int f = 0; f < 3000 && !switched; f++) {
        core->setKeys(core, 0);
        core->runFrame(core);
        // Find {initial, target = title} in IWRAM and retarget it.
        for (uint32_t a = 0x03000000; a < 0x03007ff8; a += 4) {
            if (core->busRead32(core, a + 4) == title) {
                uint32_t initial = core->busRead32(core, a);
                if (initial >= 0x08000000 && initial < 0x0a000000) {
                    core->busWrite32(core, a + 4, sceneAddr);
                    fprintf(stderr, "frame %d: transition %08x -> title at %08x retargeted\n", f, initial, a);
                    switched = 1;
                    break;
                }
            }
        }
    }
    if (!switched) { fprintf(stderr, "transition entry not found\n"); return 1; }

    // Leave the warning screen with A, then run the scene.
    // MGBA_REF_AUDIO=<wav>: the ROM's sound from the scene start on, 44.1 kHz
    // stereo s16, for comparison with the PC build's RTPC_AUDIO_DUMP.
    FILE *wav = NULL;
    uint32_t wavBytes = 0;
    struct blip_t *bl = core->getAudioChannel(core, 0), *br = core->getAudioChannel(core, 1);
    if (getenv("MGBA_REF_AUDIO")) {
        wav = fopen(getenv("MGBA_REF_AUDIO"), "wb");
        if (wav) write_wav_header(wav, 0);
        core->setAudioBufferSize(core, 4096);
        blip_set_rates(bl, core->frequency(core), AUDIO_RATE);
        blip_set_rates(br, core->frequency(core), AUDIO_RATE);
    }

    // The clock is tracked from here on, while the warning scene still runs:
    // epoch -1 is the warning scene, and the requested scene restarting the
    // clock (which can come after f0) makes epoch 0, as on the PC.
    int k = -1, epoch = -1, since = 0;
    int32_t lastClock = (int32_t)core->busRead32(core, SCRIPT_CLOCK);
    for (int f = 0; f < frames; f++) {
        uint32_t keys = 0;
        // The same bookkeeping as platform/input.c, read at the same point:
        // before the game polls the keys for this frame.
        int32_t clock = (int32_t)core->busRead32(core, SCRIPT_CLOCK);
        if (clock < lastClock) epoch++;
        since = (clock == lastClock) ? since + 1 : 0;
        lastClock = clock;
        if (f0 < 0 && (!clocked || epoch < 0)) {
            keys = ((f / 10) % 2) ? 0 : 1; // tap A until the scene changes
        } else if (clocked) {
            while (k + 1 < nIn &&
                   (inEpoch[k + 1] < epoch ||
                    (inEpoch[k + 1] == epoch && (inClock[k + 1] < clock ||
                     (inClock[k + 1] == clock && inSince[k + 1] <= since))))) k++;
            if (k >= 0) keys = inKeys[k];
        } else {
            while (k + 1 < nIn && inFrame[k + 1] <= f - f0) k++;
            if (k >= 0) keys = inKeys[k];
        }
        if (getenv("MGBA_REF_KEYS")) {
            static uint32_t lastKeys;
            if (keys != lastKeys) {
                fprintf(stderr, "rel %d keys %03x epoch %d clock %d since %d\n", f - f0, keys, epoch, lastClock, since);
                lastKeys = keys;
            }
        }
        if (getenv("MGBA_REF_SAVE") && epoch < 0) {
            // "off:val,..." -- bytes of the save data (TengokuSaveData, at
            // *0x030046a8 + 0x10), written while the warning screen runs,
            // i.e. before the requested scene reads the save. RTPC_TEST_LEVEL
            // prints the matching string.
            uint32_t save = core->busRead32(core, 0x030046a8);
            const char *p = getenv("MGBA_REF_SAVE");
            unsigned off, val;
            int n;
            while (save >= 0x02000000 && save < 0x03008000 && sscanf(p, "%u:%u%n", &off, &val, &n) == 2) {
                core->busWrite8(core, save + 0x10 + off, (uint8_t)val);
                p += n;
                if (*p == ',') p++;
            }
        }
        if (getenv("MGBA_REF_MEDALS")) {
            // Same as the PC's RTPC_TEST_MEDALS: D_030046a8 points at the save
            // buffer; data starts at +0x10, totalMedals at data + 0x28E.
            uint32_t save = core->busRead32(core, 0x030046a8);
            if (save >= 0x02000000 && save < 0x03008000)
                core->busWrite8(core, save + 0x10 + 0x28E, (uint8_t)atoi(getenv("MGBA_REF_MEDALS")));
        }
        core->setKeys(core, keys);
        core->runFrame(core);
        if (wav) {
            short l[2048], r[2048], lr[4096];
            int n;
            while ((n = blip_samples_avail(bl)) > 0) {
                if (n > 2048) n = 2048;
                blip_read_samples(bl, l, n, 0);
                blip_read_samples(br, r, n, 0);
                if (f0 < 0) continue; // only from the scene start
                for (int i = 0; i < n; i++) { lr[2 * i] = l[i]; lr[2 * i + 1] = r[i]; }
                fwrite(lr, 4, n, wav);
                wavBytes += 4 * n;
            }
        }
        // The warning scene has a white screen; the first frame after the
        // switch whose VRAM changed is hard to detect generically, so key the
        // frame counter on the first non-white frame after leaving it.
        if (f0 < 0 && f > 30) {
            int nonwhite = 0;
            for (unsigned i = 0; i < w * h; i += 97) if ((buf[i] & 0xFFFFFF) != 0xF8F8F8 && (buf[i] & 0xFFFFFF) != 0xFFFFFF) nonwhite++;
            if (nonwhite > (int)(w * h / 97) * 9 / 10) {
                f0 = f;
                fprintf(stderr, "scene frames start at %d, epoch %d\n", f0, epoch);
            }
        }
        if (getenv("MGBA_REF_CLOCK") && f0 >= 0 && ((f - f0) % clock_every()) == 0) {
            fprintf(stderr, "[CLOCK] rel %d clock %d delta %d bpm %d speed %d\n", f - f0,
                    (int32_t)core->busRead32(core, SCRIPT_CLOCK), (int32_t)core->busRead32(core, 0x030053d4),
                    core->busRead16(core, 0x030053cc), core->busRead16(core, 0x030053ce));
        }
        if (getenv("MGBA_REF_OAM") && f0 >= 0 && f - f0 == atoi(getenv("MGBA_REF_OAM"))) {
            // Same format as RTPC_OAMDUMP in platform/ppu.c, which also
            // writes VRAM to vram_NNNNN.bin.
            {
                char vp[512];
                snprintf(vp, sizeof(vp), "%s/vram_%05d.bin", outdir, f - f0);
                FILE *vf = fopen(vp, "wb");
                for (uint32_t a = 0; vf && a < 0x18000; a += 2) {
                    uint16_t v = core->busRead16(core, 0x06000000 + a);
                    fwrite(&v, 2, 1, vf);
                }
                if (vf) fclose(vf);
            }
            fprintf(stderr, "[IO]");
            // From mGBA's register file: the scroll registers are
            // write-only and read back as open bus through the bus.
            struct GBA *gba = core->board;
            for (int r = 0; r < 0x56; r += 2) fprintf(stderr, " %04x", gba->memory.io[r >> 1]);
            fprintf(stderr, "\n");
            for (int b = 0; b < 32; b++) { // 16 BG banks, then 16 OBJ banks
                fprintf(stderr, "[PAL] %s%2d", b < 16 ? "bg " : "obj", b & 15);
                for (int c = 0; c < 16; c++) fprintf(stderr, " %04x", core->busRead16(core, 0x05000000 + (b * 16 + c) * 2));
                fprintf(stderr, "\n");
            }
            for (int i = 0; i < 128; i++) {
                uint16_t a0 = core->busRead16(core, 0x07000000 + i * 8);
                uint16_t a1 = core->busRead16(core, 0x07000002 + i * 8);
                uint16_t a2 = core->busRead16(core, 0x07000004 + i * 8);
                if (((a0 >> 8) & 3) == 2) continue;
                fprintf(stderr, "[OAM] %3d %04x %04x %04x", i, a0, a1, a2);
                if (a0 & 0x100) {
                    uint32_t g = 0x07000000 + ((a1 >> 9) & 0x1F) * 32;
                    fprintf(stderr, "  pa %d pb %d pc %d pd %d", (int16_t)core->busRead16(core, g + 6),
                            (int16_t)core->busRead16(core, g + 14), (int16_t)core->busRead16(core, g + 22),
                            (int16_t)core->busRead16(core, g + 30));
                }
                fprintf(stderr, "\n");
            }
        }
        if (getenv("MGBA_REF_PC") && f0 >= 0 && f - f0 >= atoi(getenv("MGBA_REF_PC")) && ((f - f0) % 30) == 0) {
            // Where the CPU is when the frame ends, to tell a game that is
            // waiting from one that is stuck.
            uint32_t pc = 0, lr = 0;
            core->readRegister(core, "pc", &pc);
            core->readRegister(core, "lr", &lr);
            fprintf(stderr, "[PC] rel %d pc %08x lr %08x\n", f - f0, pc, lr);
        }
        if (getenv("MGBA_REF_RNG") && f0 >= 0 && f - f0 < 40) {
            fprintf(stderr, "rel %d rng %04x\n", f - f0, core->busRead16(core, 0x030000b4));
        }
        if (f0 >= 0 && ((f - f0) % every) == 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/ref_%05d.ppm", outdir, f - f0);
            save_ppm(path, buf, w, h);
        }
    }
    if (wav) { write_wav_header(wav, wavBytes); fclose(wav); }
    core->deinit(core);
    return 0;
}
