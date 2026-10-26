#!/usr/bin/env python3
"""Find higher-quality originals of the game's 962 samples in other sample
libraries (e.g. ones you extracted from later games in the series), and write
them out as replacement files for platform/sample_replace.c.

    tools/samplematch/match.py OUTDIR LIBDIR [LIBDIR...]

For every sample in audio/samples (sample_table.json gives its base key and
loop) it searches every .wav under the LIBDIRs:

  1. Everything is brought to the GBA's quality first -- resampled to
     13379 Hz, so band-limited to its 6.7 kHz -- and leading silence is
     trimmed, so a 32 kHz recording of the same sound compares equal to the
     8-bit GBA copy instead of losing on its extra treble.
  2. A spectrogram fingerprint (40 log-spaced bands, first 0.6 s) narrows
     the library to the 20 most similar candidates per sample, also trying
     the candidate an octave up and down.
  3. Those are compared by waveform: the peak normalised cross-correlation
     over +-250 ms. The same recording, however it was re-encoded, scores
     close to 1; a re-recorded or different sound does not.
  4. A match is accepted when the waveform correlation is >= --accept
     (0.90) and the candidate really carries more than the GBA had: a rate
     of at least 20 kHz and energy above 6.8 kHz no more than 50 dB below
     its total. Looping samples also need the candidate's own loop to land
     within 5 % of where the GBA loop does (otherwise it is listed for
     review, not written).

Accepted candidates are written as OUTDIR/samples/sample_NNN.wav: aligned in
time to the original (the lag found in step 3, so a rhythm cue does not move),
scaled to the original's level, at a rate that keeps its pitch, with its loop
in a smpl chunk. OUTDIR/matches.csv lists the best candidate of every sample
with the scores, and OUTDIR/listen/sample_NNN.wav holds original then
replacement, for listening.
"""
import csv, glob, hashlib, json, os, struct, sys, argparse
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AR = 13379                     # analysis rate: the GBA's
NFFT, HOP, NBANDS, NFRAMES = 512, 128, 40, 64
NOTE = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}


def note_to_midi(s):
    n = NOTE[s[0]]
    i = 1
    if s[i:i + 1] in ('#', 's'):
        n += 1; i += 1
    elif s[i:i + 1] == 'b':
        n -= 1; i += 1
    return n + 12 * (int(s[i:]) + 1)


def read_wav(path):
    """-> (float mono array in [-1,1], rate, loop (start, end) or None, unity key or None)"""
    b = open(path, 'rb').read()
    if b[:4] != b'RIFF' or b[8:12] != b'WAVE':
        raise ValueError('not RIFF WAVE')
    p, fmt, data, loop, unity = 12, None, None, None, None
    while p + 8 <= len(b):
        cid, ln = b[p:p + 4], struct.unpack('<I', b[p + 4:p + 8])[0]
        body = b[p + 8:p + 8 + ln]
        if cid == b'fmt ':
            fmt = struct.unpack('<HHIIHH', body[:16])
            if fmt[0] == 0xFFFE and len(body) >= 26:
                fmt = (struct.unpack('<H', body[24:26])[0],) + fmt[1:]
        elif cid == b'data':
            data = body
        elif cid == b'smpl' and len(body) >= 36:
            unity = struct.unpack('<I', body[12:16])[0]
            if struct.unpack('<I', body[28:32])[0] > 0 and len(body) >= 60:
                s, e = struct.unpack('<II', body[44:52])
                loop = (s, e + 1)
        p += 8 + ln + (ln & 1)
    if fmt is None or data is None:
        raise ValueError('no fmt/data')
    tag, ch, rate, _, align, bits = fmt
    if tag == 1 and bits == 8:
        x = (np.frombuffer(data, np.uint8).astype(np.float32) - 128) / 128
    elif tag == 1 and bits == 16:
        x = np.frombuffer(data[:len(data) // 2 * 2], '<i2').astype(np.float32) / 32768
    elif tag == 1 and bits == 24:
        a = np.frombuffer(data[:len(data) // 3 * 3], np.uint8).reshape(-1, 3).astype(np.int32)
        x = ((a[:, 0] << 8 | a[:, 1] << 16 | a[:, 2] << 24) >> 8).astype(np.float32) / 8388608
    elif tag == 3 and bits == 32:
        x = np.frombuffer(data[:len(data) // 4 * 4], '<f4').astype(np.float32)
    else:
        raise ValueError('format %d/%d' % (tag, bits))
    if ch > 1:
        x = x[:len(x) // ch * ch].reshape(-1, ch).mean(1)
    return x, rate, loop, unity


def write_wav(path, x, rate, loop=None, unity=None):
    pcm = np.clip(np.round(x * 32767), -32768, 32767).astype('<i2').tobytes()
    smpl = b''
    if loop is not None:
        body = struct.pack('<IIIIIIIII', 0, 0, int(1e9 / rate), unity or 60, 0, 0, 0, 1, 0)
        body += struct.pack('<IIIIII', 0, 0, loop[0], loop[1] - 1, 0, 0)
        smpl = b'smpl' + struct.pack('<I', len(body)) + body
    fmt = struct.pack('<HHIIHH', 1, 1, rate, rate * 2, 2, 16)
    riff = b'WAVE' + b'fmt ' + struct.pack('<I', 16) + fmt + b'data' + struct.pack('<I', len(pcm)) + pcm + smpl
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(riff)) + riff)


def resample(x, r_from, r_to):
    """FFT resampling; going down, everything above the new Nyquist is dropped."""
    if r_from == r_to or len(x) == 0:
        return x.astype(np.float32)
    n = len(x)
    m = max(1, int(round(n * r_to / r_from)))
    X = np.fft.rfft(x)
    Y = np.zeros(m // 2 + 1, complex)
    k = min(len(X), len(Y))
    Y[:k] = X[:k]
    return (np.fft.irfft(Y, m) * (m / n)).astype(np.float32)


def trim_start(x, thresh_db=-40):
    if len(x) == 0:
        return 0
    a = np.abs(x)
    peak = a.max()
    if peak <= 0:
        return 0
    idx = np.nonzero(a > peak * 10 ** (thresh_db / 20))[0]
    return int(idx[0]) if len(idx) else 0


_win = np.hanning(NFFT).astype(np.float32)
_freqs = np.fft.rfftfreq(NFFT, 1 / AR)
_edges = np.geomspace(60, 6000, NBANDS + 1)
_band_of = np.digitize(_freqs, _edges) - 1
# bins -> bands as one matrix product (the per-band loop was most of the cost)
_band_mat = np.zeros((len(_freqs), NBANDS), np.float32)
for _b in range(NBANDS):
    _band_mat[_band_of == _b, _b] = 1.0


def fingerprint(y):
    """y at AR, start-trimmed -> unit vector."""
    need = NFFT + HOP * (NFRAMES - 1)
    if len(y) < need:
        y = np.concatenate([y, np.zeros(need - len(y), np.float32)])
    frames = np.lib.stride_tricks.sliding_window_view(y[:need], NFFT)[::HOP][:NFRAMES]
    S = np.abs(np.fft.rfft(frames * _win, axis=1)) ** 2
    B = (S.astype(np.float32) @ _band_mat)
    F = np.log1p(B / (B.max() + 1e-12) * 1e4).ravel()
    F -= F.mean()
    n = np.linalg.norm(F)
    return F / n if n > 0 else F


def xcorr_peak(a, b, maxlag):
    """Peak normalised cross-correlation of a against b (b shifted by lag), |lag| <= maxlag.
    -> (corr, lag): b[i + lag] ~ a[i]."""
    n = len(a) + len(b)
    nf = 1 << int(np.ceil(np.log2(n)))
    c = np.fft.irfft(np.fft.rfft(a, nf) * np.conj(np.fft.rfft(b, nf)), nf)
    lags = np.concatenate([np.arange(0, maxlag + 1), np.arange(-maxlag, 0)])
    vals = np.concatenate([c[:maxlag + 1], c[nf - maxlag:]])
    i = int(np.argmax(np.abs(vals)))
    lag = -int(lags[i])
    # normalise over the overlap actually compared
    if lag >= 0:
        aa, bb = a[:max(0, min(len(a), len(b) - lag))], b[lag:lag + len(a)]
    else:
        aa, bb = a[-lag:-lag + len(b)], b[:max(0, len(a) + lag)]
    m = min(len(aa), len(bb))
    if m < 64:
        return 0.0, lag
    aa, bb = aa[:m], bb[:m]
    d = np.sqrt((aa * aa).sum() * (bb * bb).sum())
    return (float((aa * bb).sum() / d) if d > 0 else 0.0), lag


HEAD_S = 12          # seconds of each library file kept for the search
POOL_K = 8           # best fingerprints kept per sample, per shift and chunk


def onsets(y, most=4):
    """Start of the first sound, then up to most-1 later attacks (a 10 dB
    jump in 20 ms energy over the 100 ms before), in AR samples."""
    out = [trim_start(y)]
    w = AR // 50
    if len(y) < 8 * w:
        return out
    e = np.convolve(y.astype(np.float64) ** 2, np.ones(w), 'valid')[::w] + 1e-9
    floor = e.max() * 1e-4
    for i in range(5, len(e)):
        if e[i] > floor and e[i] > 10 * e[i - 5:i].mean() and i * w - out[-1] > AR // 4:
            out.append(i * w)
            if len(out) == most:
                break
    return out


def collect(pool, S, metas, k):
    """Keep each sample's POOL_K best rows of this chunk of scores. Rows at
    the file's own pitch go to a pool of their own, so the many chance
    look-alikes at other shifts cannot crowd them out."""
    kk = min(POOL_K, S.shape[0])
    idx = np.argpartition(-S, kk - 1, axis=0)[:kk]
    for gi in range(S.shape[1]):
        bucket = pool[gi][0 if k == 0 else 1]
        for r in idx[:, gi]:
            ci, o = metas[r]
            bucket.append((float(S[r, gi]), ci, k, o))
        if len(bucket) > 200:
            bucket[:] = sorted(bucket, reverse=True)[:100]


def ncc_search(a, b):
    """Normalised cross-correlation of a against b at every lag where they
    overlap enough (b may be much longer: a can sit anywhere in it), each
    lag normalised by the energy of the part that actually overlaps.
    -> (corr, lag): b[i + lag] ~ a[i]."""
    la, lb = len(a), len(b)
    if la < 64 or lb < 64:
        return 0.0, 0
    nf = 1 << int(np.ceil(np.log2(la + lb)))
    c = np.fft.irfft(np.conj(np.fft.rfft(a, nf)) * np.fft.rfft(b, nf), nf)
    lags = np.arange(-(la - 1), lb)                      # b index of a[0]
    cc = np.concatenate([c[nf - (la - 1):], c[:lb]])
    ca = np.concatenate([[0.0], np.cumsum(a.astype(np.float64) ** 2)])
    cb = np.concatenate([[0.0], np.cumsum(b.astype(np.float64) ** 2)])
    i0 = np.maximum(0, -lags)                            # overlap in a: [i0, i1)
    i1 = np.minimum(la, lb - lags)
    ov = i1 - i0
    ea = ca[i1] - ca[i0]
    eb = cb[i1 + lags] - cb[i0 + lags]
    ok = (ov >= max(64, int(0.5 * la))) & (ov >= int(0.7 * min(la, lb)))
    d = np.sqrt(ea * eb)
    r = np.where(ok & (d > 0), cc / np.where(d > 0, d, 1), 0.0)
    i = int(np.argmax(np.abs(r)))
    return float(r[i]), int(lags[i])


def bandwidth(x, rate):
    """Highest frequency (Hz) whose 500 Hz band still holds energy within
    60 dB of the loudest band: how much treble the recording really has,
    whatever rate it is stored at (an upsampled 13 kHz file reports ~6.7k)."""
    if len(x) < 256:
        return 0.0
    X = np.abs(np.fft.rfft(x.astype(np.float64))) ** 2
    f = np.fft.rfftfreq(len(x), 1 / rate)
    edges = np.arange(0, rate / 2 + 500, 500)
    e = np.array([X[(f >= lo) & (f < lo + 500)].sum() for lo in edges[:-1]])
    if e.max() <= 0:
        return 0.0
    ok = np.nonzero(e > e.max() * 1e-6)[0]
    return float(edges[ok[-1] + 1]) if len(ok) else 0.0


def image_score(x, rate, gba_rate=13379):
    """Correlation of the log spectrum above the GBA's Nyquist with its mirror
    below it. A 13 kHz sample played back at a higher rate without proper
    filtering (a console mixer's render, a lazy upsample) has images there:
    "treble" that is the band below, reflected. Real treble scores near 0."""
    if rate <= gba_rate + 1000 or len(x) < 512:
        return 0.0
    X = np.log(np.abs(np.fft.rfft(x * np.hanning(len(x)))) + 1e-9)
    f = np.fft.rfftfreq(len(x), 1 / rate)
    ny = gba_rate / 2
    fs = np.linspace(ny + 300, min(gba_rate - 300, rate / 2 - 100), 200)
    up, dn = np.interp(fs, f, X), np.interp(gba_rate - fs, f, X)
    if up.std() == 0 or dn.std() == 0:
        return 0.0
    return float(np.corrcoef(up, dn)[0, 1])


def hf_db(x, rate, cut=6800):
    if rate <= 2 * cut or len(x) < 64:
        return -999.0
    X = np.abs(np.fft.rfft(x)) ** 2
    f = np.fft.rfftfreq(len(x), 1 / rate)
    tot, hi = X.sum(), X[f >= cut].sum()
    return float(10 * np.log10(hi / tot)) if tot > 0 and hi > 0 else -999.0


def search(paths, GF, ngba, cands=None, pool=None):
    """Fingerprint every library file and keep, per GBA sample, the best
    (score, file, shift, onset) rows: pool[gi] = (same pitch, other shifts).

    Library files are read one at a time and their fingerprints scored in
    batches, the audio dropped at once, so memory stays bounded however
    many files there are (this 8 GB machine froze once under an unbounded
    job). Every semitone from an octave down to an octave up is tried from
    the first sound in the file; later onsets (the sample may be one hit
    inside a longer file) at the file's own pitch."""
    if pool is None:
        pool = [([], []) for _ in range(ngba)]
    shifts = range(-12, 13)
    buf = {k: ([], []) for k in shifts}

    def flush():
        for k, (rows, metas) in buf.items():
            if rows:
                collect(pool, np.stack(rows) @ GF.T, metas, k)
                rows.clear(); metas.clear()

    cands = cands if cands is not None else []
    seen = {c['sha1']: i for i, c in enumerate(cands) if 'sha1' in c}
    known = {c['path'] for c in cands}
    for p in paths:
        if p in known:
            continue
        try:
            raw = open(p, 'rb').read()
            h = hashlib.sha1(raw).hexdigest()
            if h in seen:
                cands[seen[h]]['dups'] += 1
                continue
            x, rate, loop, unity = read_wav(p)
        except Exception as ex:
            print('skip', p, ex, file=sys.stderr)
            continue
        if len(x) < 32 or len(x) > 30 * rate:   # music streams are not samples
            continue
        ci = seen[h] = len(cands)
        cands.append(dict(path=p, rate=rate, loop=loop, dups=0, hf=hf_db(x, rate), sha1=h))
        yh = resample(x[:int(HEAD_S * rate)], rate, AR)
        ons = onsets(yh)
        for k in shifts:
            for o in (ons if k == 0 else ons[:1]):
                need = int((NFFT + HOP * NFRAMES) * 2 ** (k / 12)) + 8
                buf[k][0].append(fingerprint(resample(yh[o:o + need], AR * 2 ** (k / 12), AR)))
                buf[k][1].append((ci, o))
        if len(cands) % 500 == 0:
            flush()
            print('  %d distinct files searched' % len(cands), flush=True)
    flush()
    print('library files: %d, distinct: %d' % (len(paths), len(cands)), flush=True)
    return cands, pool


# Folders holding finished sound effects recorded from a console's output
# rather than raw waveforms. Their mixer's images can pass for treble (Gold's
# DS_ACCURATE renders measure "24 kHz") and they carry envelopes and effects,
# so a raw sample of the same sound is preferred; a render fills a gap only.
RENDER_DIRS = ('DS_ACCURATE', '01_効果音', '_ALL_SFX_WAV', 'PREVIEW')


def is_render(path):
    return any(('/%s' % d) in path for d in RENDER_DIRS)


RANK = {'accept': 5, 'accept-loopfit': 4, 'review': 3, 'same-quality': 2, 'similar': 1, 'none': 0}


def evaluate(g, c, best, accept):
    """-> (decision, note, loopfit, rate_eff) for candidate c (with its audio
    in c['x']) at best = {k, lag, corr, spec}."""
    # k: the candidate matched when raised k semitones, so it plays at
    # rate * 2^(k/12)
    rate_eff = c['rate'] * 2 ** (best['k'] / 12)
    # Higher quality means real treble: 9 kHz of actual bandwidth (a 44.1 kHz
    # file that stops at 7 kHz is no better), and not the GBA's band mirrored
    # (Best+ renders of Space Dance's voices scored 0.4-0.8 on that).
    hq = (c['rate'] >= 20000 and c['rate'] > g['rate'] and c['hf'] > -50
          and bandwidth(c['x'], c['rate']) >= 9000 and image_score(c['x'], c['rate']) < 0.35)
    corr = abs(best['corr'])
    # Trying every semitone and every position makes a chance match of two
    # steady tones easy (one Fever sound matched 15 different GBA samples at
    # odd shifts at 0.8 or more), so a transposed match needs a
    # near-identical waveform, and a mere "similar" one must be at the same
    # pitch or an octave, near the start.
    if corr >= accept and (best['k'] == 0 or corr >= 0.97):
        if not hq:
            return 'same-quality', '', None, rate_eff
        if not g['loop']:
            return 'accept', '', None, rate_eff
        # where the GBA loop falls in the candidate's samples
        gl0 = (g['loop'][0] / g['rate']) * AR
        ex0 = (gl0 + best['lag']) / AR * rate_eff
        if c['loop'] and abs(c['loop'][0] - ex0) <= 0.05 * max(ex0, 1):
            return 'accept', '', None, rate_eff
        # The candidate does not loop there: take the GBA's loop, moved onto
        # the candidate's timeline, and fit its end.
        loopfit = fit_loop(g, c, best, rate_eff)
        if not loopfit:
            return 'review', 'candidate too short for the GBA loop', None, rate_eff
        note = ('candidate has no loop' if not c['loop'] else
                'candidate loops at %d, GBA at %.0f' % (c['loop'][0], ex0))
        note += '; loop fitted, join error %.3f%s' % (
            loopfit[2], ' (crossfaded)' if loopfit[2] > 0.02 else '')
        return 'accept-loopfit', note, loopfit, rate_eff
    if (best['k'] in (0, 12, -12) and abs(best['lag']) <= 0.3 * AR
            and corr >= 0.8 and best['spec'] >= 0.85):
        return 'similar', '', None, rate_eff
    return 'none', '', None, rate_eff


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('outdir')
    ap.add_argument('libdirs', nargs='+')
    ap.add_argument('--accept', type=float, default=0.90)
    ap.add_argument('--top', type=int, default=20)
    ap.add_argument('--reuse', action='store_true',
                    help='reuse OUTDIR/search.pkl from an earlier run (only the comparison is redone)')
    ap.add_argument('--add', action='store_true',
                    help='with --reuse: also search the LIBDIRs\' files that the saved search has not seen')
    args = ap.parse_args()
    os.makedirs(os.path.join(args.outdir, 'samples'), exist_ok=True)
    os.makedirs(os.path.join(args.outdir, 'listen'), exist_ok=True)

    table = json.load(open(os.path.join(ROOT, 'audio/sample_table.json')))['samples']
    gba = []
    for i, e in enumerate(table):
        x, rate, _, _ = read_wav(os.path.join(ROOT, 'audio/samples', e['sample']))
        y = resample(x, rate, AR)
        t = trim_start(y)
        gba.append(dict(n=i + 1, x=x, rate=rate, key=note_to_midi(e['pitch']),
                        loop=tuple(e['loop']) if 'loop' in e else None, y=y, t=t))
    print('GBA samples:', len(gba), flush=True)

    paths = sorted(p for d in args.libdirs for p in glob.glob(os.path.join(d, '**', '*.wav'), recursive=True))
    # Library files are read in batches: each one's fingerprints are taken
    # and scored at once and its audio dropped, so memory stays bounded
    # however many files there are (this 8 GB machine froze once under an
    # unbounded job). Every semitone from an octave down to an octave up is
    # tried from the first sound in the file; later onsets (the sample may be
    # one hit inside a longer file) at the file's own pitch.
    import pickle
    cache_path = os.path.join(args.outdir, 'search.pkl')
    GF = np.stack([fingerprint(g['y'][g['t']:]) for g in gba])
    if args.reuse and os.path.exists(cache_path):
        cands, pool = pickle.load(open(cache_path, 'rb'))
        print('reusing the fingerprint search in', cache_path, flush=True)
        if args.add:
            # search only files not seen before, and keep the result
            n0 = len(cands)
            cands, pool = search(paths, GF, len(gba), cands, pool)
            print('added %d distinct files' % (len(cands) - n0), flush=True)
            pickle.dump((cands, pool), open(cache_path, 'wb'))
    else:
        cands, pool = search(paths, GF, len(gba))
        pickle.dump((cands, pool), open(cache_path, 'wb'))

    out_rows = []
    cache = {}

    def audio(c):
        x = cache.get(c['path'])
        if x is None:
            x = read_wav(c['path'])[0]
            if len(cache) > 300:
                cache.clear()
            cache[c['path']] = x
        return x

    for gi, g in enumerate(gba):
        same, other = pool[gi]
        # the best at the file's own pitch (more of them: copies of the same
        # sound at GBA quality can fill the first places), then the best of
        # all shifts
        pairs = sorted(same, reverse=True)[:3 * args.top] + sorted(other, reverse=True)[:args.top]
        tried, results = set(), []
        for spec, ci, k, o in pairs:
            if (ci, k) in tried:
                continue
            tried.add((ci, k))
            c = dict(cands[ci])
            c['x'] = audio(c)
            y = resample(c['x'], c['rate'] * 2 ** (k / 12), AR)
            corr, lag = ncc_search(g['y'], y)
            b = dict(ci=ci, k=k, spec=spec, corr=corr, lag=lag)
            dec, note, loopfit, rate_eff = evaluate(g, c, b, args.accept)
            # Most usable first: a high-quality match over the same recording
            # at GBA quality, a working loop over a broken one. Among equally
            # usable ones a raw waveform over a render, then the one with the
            # most treble (to the kHz), and only then the closest waveform --
            # every accepted one is already the same recording (>= 0.90).
            c['bw'] = bandwidth(c['x'], c['rate']) if dec in ('accept', 'accept-loopfit') else 0.0
            results.append(((RANK[dec], not is_render(c['path']), int(c['bw'] / 1000), abs(corr)),
                            b, c, dec, note, loopfit, rate_eff))
        results.sort(key=lambda r: r[0], reverse=True)
        _, best, c, decision, note, loopfit, rate_eff = results[0]
        alts = sum(1 for r in results if r[3] in ('accept', 'accept-loopfit'))
        out_rows.append(dict(sample='sample_%03d' % g['n'], decision=decision,
                         wave_corr='%.3f' % best['corr'], spec_sim='%.3f' % best['spec'],
                         shift_semitones=best['k'], lag_ms='%.1f' % (best['lag'] / AR * 1000),
                         gba_rate=g['rate'], cand_rate=c['rate'], cand_hf_db='%.1f' % c['hf'],
                         cand_bandwidth_hz='%.0f' % c.get('bw', 0), usable_candidates=alts,
                         gba_loop=g['loop'] or '', cand_loop=c['loop'] or '',
                         candidate=c['path'], same_file_also=c['dups'], note=note))
        if decision in ('accept', 'accept-loopfit'):
            write_replacement(args.outdir, g, c, best, rate_eff, loopfit)
        elif decision == 'similar' and c['rate'] >= 20000 and c['hf'] > -50:
            # a different take or re-recording, maybe: only for listening
            write_listen(os.path.join(args.outdir, 'listen_similar'), g, c['x'], int(round(rate_eff)))
        if gi % 50 == 0:
            print('%d/%d' % (gi, len(gba)), flush=True)

    with open(os.path.join(args.outdir, 'matches.csv'), 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=list(out_rows[0].keys()))
        w.writeheader()
        w.writerows(out_rows)
    from collections import Counter
    print(Counter(r['decision'] for r in out_rows))


def shift_for(best, rate_eff):
    return int(round(best['lag'] / AR * rate_eff))


def fit_loop(g, c, best, rate_eff):
    """GBA loop moved onto the candidate's (aligned) samples, end adjusted
    within +-8 GBA samples to where the
    waveform after the end best continues the one after the start.
    -> (start, end, relative join error) in aligned candidate samples, or None."""
    k = rate_eff / g['rate']
    x = aligned(c['x'], shift_for(best, rate_eff))
    s0 = int(round(g['loop'][0] * k))
    e0 = int(round(g['loop'][1] * k))
    w = max(16, int(k * 4))
    span = max(4, int(k * 8))
    if s0 - w < 0 or e0 + span + w >= len(x) or e0 - span <= s0 + w:
        return None
    ref = x[s0 - w:s0 + w]
    best_e, best_err = e0, None
    for e in range(e0 - span, e0 + span + 1):
        seg = x[e - w:e + w]
        err = float(((seg - ref) ** 2).sum())
        if best_err is None or err < best_err:
            best_e, best_err = e, err
    rel = best_err / (float((ref ** 2).sum()) + 1e-12)
    return (s0, best_e, rel)


def aligned(x, shift):
    if shift > 0:
        return x[shift:]
    if shift < 0:
        return np.concatenate([np.zeros(-shift, np.float32), x])
    return x


def write_listen(d, g, x, rate):
    os.makedirs(d, exist_ok=True)
    a = resample(g['x'], g['rate'], 48000)
    b = resample(x, rate, 48000)
    pk = max(np.abs(a).max(), np.abs(b).max(), 1e-6)
    write_wav(os.path.join(d, 'sample_%03d.wav' % g['n']),
              np.concatenate([a, np.zeros(19200, np.float32), b]) / pk * 0.9, 48000)


def write_replacement(outdir, g, c, best, rate_eff, loopfit=None):
    x, rc = c['x'], c['rate']
    # The original plays sample i of g at time i/g.rate; candidate sample j plays
    # at j/rate_eff. best.lag (AR samples) maps g's AR timeline onto c's:
    # c_AR[i + lag] ~ g_AR[i]. Shift the candidate so they coincide.
    shift = int(round(best['lag'] / AR * rate_eff))
    loop = c['loop']
    if shift > 0:
        x = x[shift:]
        if loop:
            loop = (loop[0] - shift, loop[1] - shift)
            if loop[0] < 0:
                loop = None
    elif shift < 0:
        x = np.concatenate([np.zeros(-shift, np.float32), x])
        if loop:
            loop = (loop[0] - shift, loop[1] - shift)
    # level: least squares of the band-limited, aligned candidate onto the original
    y = resample(x, rate_eff, AR)
    m = min(len(y), len(g['y']))
    d = float((y[:m] * y[:m]).sum())
    gain = float((y[:m] * g['y'][:m]).sum()) / d if d > 0 else 1.0
    x = x * gain
    if loopfit:
        loop = (loopfit[0], loopfit[1])
        x = x[:loopfit[1] + 64].copy()
        if loopfit[2] > 0.02:
            # The recording does not repeat cleanly there: crossfade the end of
            # the loop into the audio just before its start, so playback runs
            # on from the end into the start without a step.
            s0, e0 = loop
            L = int(min((e0 - s0) // 2, 0.02 * rate_eff, s0))
            if L >= 8:
                t = np.linspace(0.0, 1.0, L, dtype=np.float32)
                x[e0 - L:e0] = x[e0 - L:e0] * (1 - t) + x[s0 - L:s0] * t
    if not g['loop']:
        loop = None
    rate_out = int(round(rate_eff))
    write_wav(os.path.join(outdir, 'samples', 'sample_%03d.wav' % g['n']), x, rate_out, loop, g['key'])
    # listening file: original, 0.4 s silence, replacement, at 48 kHz
    write_listen(os.path.join(outdir, 'listen'), g, x, rate_out)


if __name__ == '__main__':
    main()
