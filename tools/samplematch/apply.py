#!/usr/bin/env python3
"""Build the final replacement folder from the choices made in review.py.

    tools/samplematch/apply.py OUTDIR [DEST]

OUTDIR is match.py's output (matches.csv, samples/, review.json). Into DEST
(default OUTDIR/final) it puts:
  - every sample match.py accepted (OUTDIR/samples/), unless marked "no";
  - every "similar" candidate marked "use": aligned to the original by the
    cross-correlation peak, at the original's level (RMS of the band both
    share, since a different take cannot be fitted by least squares), at
    the same pitch, with the GBA loop fitted the way match.py does it;
  - every AI result (OUTDIR/ai_*/samples/) marked "use".
A sample chosen in several groups takes, in order: an accepted match, a
similar recording, VoiceFixer, Resemble Enhance, AudioSR.
"""
import csv, glob, json, os, shutil, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import match   # noqa: E402

ROOT = match.ROOT
AR = match.AR


def main():
    outdir = sys.argv[1]
    dest = sys.argv[2] if len(sys.argv) > 2 else os.path.join(outdir, 'final')
    os.makedirs(dest, exist_ok=True)
    review = {}
    if os.path.exists(os.path.join(outdir, 'review.json')):
        review = json.load(open(os.path.join(outdir, 'review.json'), encoding='utf-8'))
    rows = {r['sample']: r for r in csv.DictReader(open(os.path.join(outdir, 'matches.csv'), encoding='utf-8'))}
    table = json.load(open(os.path.join(ROOT, 'audio/sample_table.json')))['samples']
    done = {}

    for f in sorted(glob.glob(os.path.join(outdir, 'samples', 'sample_*.wav'))):
        name = os.path.basename(f)[:-4]
        if review.get('listen/' + name) != 'no':
            shutil.copy(f, os.path.join(dest, name + '.wav'))
            done[name] = 'match'

    # When one sample is marked "use" in several groups: a real recording
    # first (accepted matches above, then 'similar' ones), then the voice
    # models (picked on purpose, where they suited the voice), then AudioSR.
    order = {'listen_similar': 0, 'ai_voicefixer/listen': 1, 'ai_resemble/listen': 2, 'ai_audiosr/listen': 3}
    for key, v in sorted(review.items(), key=lambda kv: (order.get(kv[0].rsplit('/', 1)[0], 9), kv[0])):
        folder, name = key.rsplit('/', 1)
        if v != 'use' or name in done:
            continue
        if folder == 'listen_similar':
            if write_similar(dest, rows[name], table):
                done[name] = 'similar'
        elif folder.startswith('ai_') and folder.endswith('/listen'):
            src = os.path.join(outdir, folder[:-len('/listen')], 'samples', name + '.wav')
            if os.path.exists(src):
                shutil.copy(src, os.path.join(dest, name + '.wav'))
                done[name] = folder.split('/')[0]

    # anything previously copied but now rejected
    for f in glob.glob(os.path.join(dest, 'sample_*.wav')):
        if os.path.basename(f)[:-4] not in done:
            os.remove(f)
    from collections import Counter
    print('%d replacement samples in %s' % (len(done), dest), dict(Counter(done.values())))


def write_similar(dest, r, table):
    n = int(r['sample'][7:])
    e = table[n - 1]
    gx, grate, _, _ = match.read_wav(os.path.join(ROOT, 'audio/samples', e['sample']))
    cx, crate, cloop, _ = match.read_wav(r['candidate'])
    k = int(r['shift_semitones'])
    rate_eff = crate * 2 ** (k / 12)
    gy = match.resample(gx, grate, AR)
    cy = match.resample(cx, rate_eff, AR)
    _, lag = match.xcorr_peak(gy, cy, AR // 4)
    shift = int(round(lag / AR * rate_eff))
    x = match.aligned(cx, shift)
    y = match.resample(x, rate_eff, AR)
    m = min(len(y), len(gy))
    ry, rg = np.sqrt((y[:m] ** 2).mean()), np.sqrt((gy[:m] ** 2).mean())
    x = x * (rg / ry if ry > 0 else 1.0)
    loop = None
    if 'loop' in e:
        g = dict(x=gx, rate=grate, loop=tuple(e['loop']))
        best = dict(lag=lag)
        fit = match.fit_loop(g, dict(x=cx), best, rate_eff)
        if not fit:
            print('%s: no room for the loop in the candidate; skipped' % r['sample'])
            return False
        loop = (fit[0], fit[1])
        x = x[:fit[1] + 64].copy()
        s0, e0 = loop
        L = int(min((e0 - s0) // 2, 0.02 * rate_eff, s0))
        if fit[2] > 0.02 and L >= 8:
            t = np.linspace(0.0, 1.0, L, dtype=np.float32)
            x[e0 - L:e0] = x[e0 - L:e0] * (1 - t) + x[s0 - L:s0] * t
    match.write_wav(os.path.join(dest, r['sample'] + '.wav'), x, int(round(rate_eff)), loop,
                    match.note_to_midi(e['pitch']))
    return True


if __name__ == '__main__':
    main()
