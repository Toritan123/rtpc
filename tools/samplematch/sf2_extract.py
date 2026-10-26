#!/usr/bin/env python3
"""Write every sample in SoundFont 2 files out as .wav, so match.py can
search instrument banks too.

    tools/samplematch/sf2_extract.py OUTDIR FILE.sf2 [FILE.sf2...]

OUTDIR/<bank>/<NNN>_<sample name>.wav: 16-bit mono at the sample's own rate,
with its loop and original key in a smpl chunk. Linked stereo halves are
written separately (the matcher compares mono anyway).
"""
import os, re, struct, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from match import write_wav   # noqa: E402


def chunks(b, start, end):
    p = start
    while p + 8 <= end:
        cid, ln = b[p:p + 4], struct.unpack('<I', b[p + 4:p + 8])[0]
        yield cid, p + 8, ln
        p += 8 + ln + (ln & 1)


def extract(path, outdir):
    b = open(path, 'rb').read()
    if b[:4] != b'RIFF' or b[8:12] != b'sfbk':
        raise ValueError('not a SoundFont')
    smpl = shdr = None
    for cid, p, ln in chunks(b, 12, len(b)):
        if cid == b'LIST':
            kind = b[p:p + 4]
            for sid, sp, sln in chunks(b, p + 4, p + ln):
                if kind == b'sdta' and sid == b'smpl':
                    smpl = np.frombuffer(b[sp:sp + sln // 2 * 2], '<i2')
                elif kind == b'pdta' and sid == b'shdr':
                    shdr = b[sp:sp + sln]
    if smpl is None or shdr is None:
        raise ValueError('no smpl/shdr')
    bank = os.path.splitext(os.path.basename(path))[0]
    d = os.path.join(outdir, bank)
    os.makedirs(d, exist_ok=True)
    n = 0
    for i in range(len(shdr) // 46 - 1):            # the last record is EOS
        rec = shdr[i * 46:(i + 1) * 46]
        name = rec[:20].split(b'\0')[0].decode('latin-1')
        start, end, ls, le, rate = struct.unpack('<IIIII', rec[20:40])
        key, _corr, _link, typ = struct.unpack('<BbHH', rec[40:46])
        if typ & 0x8000 or end <= start or rate == 0:  # ROM samples have no data here
            continue
        x = smpl[start:end].astype(np.float32) / 32768
        loop = (ls - start, le - start) if start <= ls < le <= end else None
        safe = re.sub(r'[^A-Za-z0-9_.-]+', '_', name) or 'sample'
        write_wav(os.path.join(d, '%03d_%s.wav' % (i, safe)), x, rate, loop, key)
        n += 1
    return n


if __name__ == '__main__':
    out = sys.argv[1]
    total = 0
    for f in sys.argv[2:]:
        try:
            total += extract(f, out)
        except Exception as ex:
            print('skip', f, ex, file=sys.stderr)
    print('%d samples from %d files' % (total, len(sys.argv) - 2))
