#!/usr/bin/env python3
"""Upscale some of the game's samples with AudioSR, as replacement files.

    AUDIOSR_ROOT=... <audiosr venv>/bin/python tools/samplematch/audiosr_batch.py OUTDIR N [N...]

N are sample numbers (sample_NNN in audio/samples). AUDIOSR_ROOT is a
folder holding sr_lite.py (a low-memory loader for the audiosr_basic
checkpoint) and the Hugging Face cache; its env.sh sets the rest.

For each sample it writes
  OUTDIR/raw/sample_NNN.wav      AudioSR's 48 kHz output, cut to length
  OUTDIR/samples/sample_NNN.wav  the original up to 6 kHz + AudioSR above it:
                                 the timing, level and character of the
                                 original are kept exactly, only the band
                                 the GBA never had is added. The loop, if
                                 any, is moved to 48 kHz and crossfaded.
  OUTDIR/listen/sample_NNN.wav   original, AudioSR raw, merged (0.4 s apart)

Run it under tools/samplematch/memguard.py. On the 8 GB M1 this was
written on, its first run (2026-10-20) froze the whole machine until the
kernel's watchdog panicked it. The likely cause, found later on Colab, is
not the model: AudioSR resamples its input to 48 kHz with torchaudio, whose
polyphase filter has 48000 / gcd(rate, 48000) phases -- 48000 for the GBA's
13379 Hz (3 for 16 kHz, which is all the earlier successful trials used);
the first 13379 Hz file grew a Colab process to 11.6 GB before it was
killed. Inputs are now FFT-resampled to 48 kHz first.
RTPC_SR_DEVICE picks the device (cpu by default; mps once cpu is known
to fit).
"""
import json, os, sys, time
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from match import read_wav, write_wav, resample, note_to_midi   # noqa: E402

# macOS 26+ (Darwin 27) refuses to load SciPy's Fortran extension modules
# (dyld: "section '__DATA/__thread_bss' has a zero-fill section type, but
# offset field is not zero"). AudioSR only needs SciPy because librosa
# imports it; none of the affected modules (PROPACK, COBYLA, ODE solvers) is
# on its path. Stand in for each one the loader refuses, so the import
# completes; a real call into one would raise.
class _Any:
    def __init__(self, name):
        self._n = name

    def __getattr__(self, a):
        if a.startswith('__'):
            raise AttributeError(a)
        return _Any(self._n + '.' + a)

    def __call__(self, *a, **k):
        raise RuntimeError(self._n + ' is unavailable on this OS')

    def __getitem__(self, k):
        return _Any(self._n + '[]')

    def __iter__(self):
        return iter(())


def _import_scipy_around_broken_extensions():
    import re, types

    class Broken(types.ModuleType):
        def __getattr__(self, attr):
            if attr.startswith('__'):
                raise AttributeError(attr)
            return _Any(self.__name__ + '.' + attr)

    stubbed = []
    for _ in range(40):
        try:
            import librosa, scipy.signal, scipy.sparse   # noqa: F401
            break
        except ImportError as ex:
            m = re.search(r"site-packages/(scipy/[^']+?)\.cpython", str(ex))
            if not m:
                raise
            name = m.group(1).replace('/', '.')
            sys.modules[name] = Broken(name)
            stubbed.append(name)
            for k in list(sys.modules):
                if k.startswith(('scipy', 'librosa')) and k not in stubbed:
                    del sys.modules[k]
    if stubbed:
        print('stand-ins for', len(stubbed), 'SciPy extensions the OS will not load', flush=True)


OUT_RATE = 48000
SPLIT_HZ = 6000


def band_split(x, rate, hz):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / rate)
    # a short raised-cosine crossover (5.5-6.5 kHz) instead of a brick wall
    w = np.clip((f - (hz - 500)) / 1000.0, 0, 1)
    w = 0.5 - 0.5 * np.cos(np.pi * w)
    lo = np.fft.irfft(X * (1 - w), len(x))
    hi = np.fft.irfft(X * w, len(x))
    return lo.astype(np.float32), hi.astype(np.float32)


def main():
    outdir, nums = sys.argv[1], [int(a) for a in sys.argv[2:]]
    for d in ('raw', 'samples', 'listen'):
        os.makedirs(os.path.join(outdir, d), exist_ok=True)
    sys.path.insert(0, os.environ['AUDIOSR_ROOT'])
    _import_scipy_around_broken_extensions()
    import torch, sr_lite, audiosr
    dev = os.environ.get('RTPC_SR_DEVICE', 'cpu')
    t0 = time.time()
    m, *_ = sr_lite.build(dev)
    m = m.to(dev).eval()
    m.device = torch.device(dev)
    print('model ready on %s in %.0f s' % (dev, time.time() - t0), flush=True)
    table = json.load(open(os.path.join(ROOT, 'audio/sample_table.json')))['samples']

    for n in nums:
        e = table[n - 1]
        src = os.path.join(ROOT, 'audio/samples', e['sample'])
        x, rate, _, _ = read_wav(src)
        t = time.time()
        tmp = os.path.join(outdir, '_in48.wav')
        write_wav(tmp, resample(x, rate, OUT_RATE), OUT_RATE)    # see the docstring
        sr = np.asarray(audiosr.super_resolution(m, tmp, seed=42, ddim_steps=50, guidance_scale=3.5),
                        dtype=np.float32).squeeze()
        up = resample(x, rate, OUT_RATE)
        sr = sr[:len(up)]
        if len(sr) < len(up):
            sr = np.concatenate([sr, np.zeros(len(up) - len(sr), np.float32)])
        lo, _ = band_split(up, OUT_RATE, SPLIT_HZ)
        _, hi = band_split(sr, OUT_RATE, SPLIT_HZ)
        merged = lo + hi
        loop = None
        if 'loop' in e:
            k = OUT_RATE / rate
            s0, e0 = int(round(e['loop'][0] * k)), int(round(e['loop'][1] * k))
            e0 = min(e0, len(merged))
            L = int(min((e0 - s0) // 2, 0.01 * OUT_RATE, s0))
            if L >= 8:
                w = np.linspace(0, 1, L, dtype=np.float32)
                merged[e0 - L:e0] = merged[e0 - L:e0] * (1 - w) + merged[s0 - L:s0] * w
            loop = (s0, e0)
        key = note_to_midi(e['pitch'])
        write_wav(os.path.join(outdir, 'raw', e['sample']), sr, OUT_RATE)
        write_wav(os.path.join(outdir, 'samples', e['sample']), merged, OUT_RATE, loop, key)
        gap = np.zeros(int(0.4 * OUT_RATE), np.float32)
        lst = np.concatenate([up, gap, sr, gap, merged])
        write_wav(os.path.join(outdir, 'listen', e['sample']), lst / max(np.abs(lst).max(), 1e-6) * 0.9, OUT_RATE)
        F = np.abs(np.fft.rfft(merged)) ** 2
        f = np.fft.rfftfreq(len(merged), 1 / OUT_RATE)
        print('%s  %.0f s  energy above 7 kHz: %.1f dB of total' % (
            e['sample'], time.time() - t, 10 * np.log10(F[f > 7000].sum() / F.sum() + 1e-15)), flush=True)


if __name__ == '__main__':
    main()
