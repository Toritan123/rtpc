#!/usr/bin/env python3
"""Turn the restoration models' output (rtpc_ai_output.zip from
colab_restore.ipynb) into replacement candidates for review.py / apply.py.

    tools/samplematch/ai_import.py rtpc_ai_output.zip MATCHDIR

For each model folder in the zip (voicefixer, resemble, audiosr) it writes
MATCHDIR/ai_<model>/samples/sample_NNN.wav and .../listen/sample_NNN.wav
(original, model output, result):

  audiosr        the original below 6 kHz + the model above it, so timing,
                 level and character stay the original's; loop moved to
                 48 kHz and crossfaded.
  voice models   the model's whole output (they rebuild the voice), aligned
                 to the original by cross-correlation and set to its level.
                 Kept only if its spectrogram below 4 kHz still matches the
                 original's (correlation of log band energies >= 0.80) and it
                 is not near-silent: a voice model turns a drum or a bell into
                 something else, and that is how they drop out. (Not the
                 waveform: these models resynthesise through a vocoder, so
                 the phase is new and a waveform correlation is ~0 even for a
                 perfect result -- the first import rejected all 508.)
A summary goes to MATCHDIR/ai_<model>/import.csv.
"""
import csv, json, os, sys, tempfile, zipfile
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import match   # noqa: E402

OUT_RATE = 48000
AR = match.AR


def band_split(x, rate, hz, width=1000.0):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / rate)
    w = np.clip((f - (hz - width / 2)) / width, 0, 1)
    w = 0.5 - 0.5 * np.cos(np.pi * w)
    return (np.fft.irfft(X * (1 - w), len(x)).astype(np.float32),
            np.fft.irfft(X * w, len(x)).astype(np.float32))


_EDGES = np.geomspace(80, 4000, 31)


def spectro(x, rate):
    """Log energy in 30 bands, 80 Hz-4 kHz, 10 ms hop."""
    n, hop = 1024, rate // 100
    if len(x) < n:
        x = np.concatenate([x, np.zeros(n - len(x), np.float32)])
    fr = np.lib.stride_tricks.sliding_window_view(x, n)[::hop] * np.hanning(n)
    S = np.abs(np.fft.rfft(fr, axis=1)) ** 2
    f = np.fft.rfftfreq(n, 1 / rate)
    B = np.stack([S[:, (f >= lo) & (f < hi)].sum(1) for lo, hi in zip(_EDGES[:-1], _EDGES[1:])], 1)
    return np.log10(B + 1e-10)


def spec_match(a, b, rate, maxlag=5):
    """Correlation of two log spectrograms over the frames where the
    original has sound, at the best offset within +-maxlag frames.
    -> (corr, offset in frames)."""
    A, B = spectro(a, rate), spectro(b, rate)
    loud = A.max(1) > A.max() - 4                # within 40 dB of the peak
    best = (-1.0, 0)
    for d in range(-maxlag, maxlag + 1):
        idx = np.nonzero(loud)[0]
        idx = idx[(idx + d >= 0) & (idx + d < len(B))]
        if len(idx) < 3:
            continue
        x, y = A[idx].ravel(), B[idx + d].ravel()
        if x.std() == 0 or y.std() == 0:
            continue
        c = float(np.corrcoef(x, y)[0, 1])
        if c > best[0]:
            best = (c, d)
    return best


def listen(path, parts):
    gap = np.zeros(int(0.4 * OUT_RATE), np.float32)
    seq = []
    for p in parts:
        seq += [p, gap]
    y = np.concatenate(seq[:-1])
    match.write_wav(path, y / max(np.abs(y).max(), 1e-6) * 0.9, OUT_RATE)


def main():
    zpath, mdir = sys.argv[1], sys.argv[2]
    table = json.load(open(os.path.join(match.ROOT, 'audio/sample_table.json')))['samples']
    tmp = tempfile.mkdtemp()
    zipfile.ZipFile(zpath).extractall(tmp)
    for model in sorted(os.listdir(tmp)):
        src = os.path.join(tmp, model)
        if not os.path.isdir(src):
            continue
        out = os.path.join(mdir, 'ai_' + model)
        for d in ('samples', 'listen'):
            os.makedirs(os.path.join(out, d), exist_ok=True)
        rows = []
        for fn in sorted(os.listdir(src)):
            if not fn.startswith('sample_') or not fn.endswith('.wav'):
                continue
            n = int(fn[7:10])
            e = table[n - 1]
            gx, grate, _, _ = match.read_wav(os.path.join(match.ROOT, 'audio/samples', e['sample']))
            mx, mrate, _, _ = match.read_wav(os.path.join(src, fn))
            up = match.resample(gx, grate, OUT_RATE)
            mo = match.resample(mx, mrate, OUT_RATE)
            loop, note = None, ''
            if model == 'audiosr':
                mo = mo[:len(up)]
                if len(mo) < len(up):
                    mo = np.concatenate([mo, np.zeros(len(up) - len(mo), np.float32)])
                lo, _ = band_split(up, OUT_RATE, 6000)
                _, hi = band_split(mo, OUT_RATE, 6000)
                res = lo + hi
                if 'loop' in e:
                    k = OUT_RATE / grate
                    s0, e0 = int(round(e['loop'][0] * k)), min(int(round(e['loop'][1] * k)), len(res))
                    L = int(min((e0 - s0) // 2, 0.01 * OUT_RATE, s0))
                    if L >= 8:
                        t = np.linspace(0, 1, L, dtype=np.float32)
                        res[e0 - L:e0] = res[e0 - L:e0] * (1 - t) + res[s0 - L:s0] * t
                    loop = (s0, e0)
                keep = True
            else:
                # judge and align by the spectrogram (see the docstring)
                c4, d = spec_match(up, mo, OUT_RATE)
                shift = d * (OUT_RATE // 100)
                res = match.aligned(mo, shift)[:len(up) + int(0.05 * OUT_RATE)]
                if len(res) < len(up):
                    res = np.concatenate([res, np.zeros(len(up) - len(res), np.float32)])
                lo_g, _ = band_split(up, OUT_RATE, 4000)
                lo_r, _ = band_split(res[:len(up)], OUT_RATE, 4000)
                rg, rr = np.sqrt((lo_g ** 2).mean()), np.sqrt((lo_r ** 2).mean())
                level = rr / rg if rg > 0 else 0.0
                res = res * (rg / rr if rr > 0 else 1.0)
                keep = c4 >= 0.80 and level >= 0.1
                note = 'spectrogram corr below 4 kHz %.2f, offset %d ms, level %.2f%s' % (
                    c4, d * 10, level, '' if level >= 0.1 else ' (near-silent output)')
            rows.append(dict(sample=fn[:-4], kept=keep, note=note))
            if keep:
                match.write_wav(os.path.join(out, 'samples', fn), res, OUT_RATE, loop,
                                match.note_to_midi(e['pitch']))
                listen(os.path.join(out, 'listen', fn), [up, mo, res])
        with open(os.path.join(out, 'import.csv'), 'w', newline='', encoding='utf-8') as f:
            w = csv.DictWriter(f, fieldnames=['sample', 'kept', 'note'])
            w.writeheader()
            w.writerows(rows)
        print('%s: %d of %d kept' % (model, sum(r['kept'] for r in rows), len(rows)))


if __name__ == '__main__':
    main()
