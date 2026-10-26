#!/usr/bin/env python3
# Compare two game audio recordings (PC RTPC_AUDIO_DUMP vs mGBA MGBA_REF_AUDIO).
#
#   audiocmp.py PC.wav ROM.wav
#
# Aligns them by cross-correlating 10 ms RMS envelopes, then reports the lag,
# the overall gain ratio, how well the envelopes agree (correlation) and the
# per-octave level difference after removing the overall gain -- a tone or
# filter problem shows as a band that stands out; a constant gain does not.
import sys, wave
import numpy as np


def load(p):
    w = wave.open(p)
    d = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64)
    d = d.reshape(-1, w.getnchannels())
    return d, w.getframerate()


def envelope(x, rate, ms=10):
    n = int(rate * ms / 1000)
    m = len(x) // n
    return np.sqrt((x[:m * n].reshape(m, n) ** 2).mean(axis=1) + 1e-9)


def main():
    pc, r1 = load(sys.argv[1]); rom, r2 = load(sys.argv[2])
    assert r1 == r2, 'sample rates differ'
    rate = r1
    a = pc.mean(axis=1); b = rom.mean(axis=1)
    ea = np.log(envelope(a, rate)); eb = np.log(envelope(b, rate))
    ea -= ea.mean(); eb -= eb.mean()
    # lag in 10 ms steps: positive = PC is ahead (starts earlier)
    def score(L):
        x = ea[max(0, L):]; y = eb[max(0, -L):]
        m = min(len(x), len(y))
        return np.dot(x[:m], y[:m]) / max(1, m)
    best = max(range(-300, 301), key=score)
    lag = best * int(rate / 100)
    if lag >= 0:
        a2, b2 = pc[lag:], rom
    else:
        a2, b2 = pc, rom[-lag:]
    n = min(len(a2), len(b2))
    a2, b2 = a2[:n], b2[:n]
    am, bm = a2.mean(axis=1), b2.mean(axis=1)
    gain = np.sqrt((am ** 2).mean() / (bm ** 2).mean())
    e1, e2 = envelope(am, rate), envelope(bm, rate)
    corr = np.corrcoef(e1, e2)[0, 1]
    print('lag %+.3f s (PC ahead), compared %.1f s, gain PC/ROM %.3f, envelope correlation %.4f'
          % (lag / rate, n / rate, gain, corr))
    # stereo balance
    for ch, nm in ((0, 'L'), (1, 'R')):
        print('  %s rms PC %.0f ROM %.0f' % (nm, np.sqrt((a2[:, ch] ** 2).mean()), np.sqrt((b2[:, ch] ** 2).mean())))
    # per-octave spectrum, gain-normalised
    seg = 8192
    k = n // seg
    def spec(x):
        s = np.abs(np.fft.rfft(x[:k * seg].reshape(k, seg) * np.hanning(seg), axis=1)) ** 2
        return s.mean(axis=0)
    sa, sb = spec(am), spec(bm / 1.0)
    f = np.fft.rfftfreq(seg, 1 / rate)
    print('  octave band  PC/ROM dB (after removing the overall gain)')
    lo = 31.25
    while lo < rate / 2:
        hi = lo * 2
        m = (f >= lo) & (f < hi)
        if m.any():
            d = 10 * np.log10(sa[m].sum() / max(sb[m].sum(), 1e-9)) - 20 * np.log10(gain)
            print('  %6.0f-%-6.0f Hz %+6.1f' % (lo, hi, d))
        lo = hi
    # windows where the envelopes disagree most (gain-normalised, 100 ms)
    w1 = envelope(am, rate, 100) / gain; w2 = envelope(bm, rate, 100)
    # The floor (~-40 dBFS) keeps decay tails in near-silence from reading
    # as 10-15 dB differences: 21 vs 2 is a ratio, not anything audible.
    diff = 20 * np.log10((w1 + 300) / (w2 + 300))
    idx = np.argsort(-np.abs(diff))[:8]
    print('  largest 100 ms differences (t in ROM time, dB):',
          ', '.join('%.1fs %+.1f' % (i / 10 + max(0, -lag) / rate, diff[i]) for i in sorted(idx)))


main()
