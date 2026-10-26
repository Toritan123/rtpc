#!/usr/bin/env python3
"""Pack the samples for the restoration models (run them on Colab with
colab_restore.ipynb).

    tools/samplematch/ai_prepare.py OUT.zip [MATCHDIR...]

Samples that MATCHDIR/matches.csv already replaces with a real higher-quality
recording (accept / accept-loopfit) are left out. In the zip:
  voice/sample_NNN.wav  every one-shot of 0.15-3 s (the voice models run on
                        all of them; ai_import.py keeps only results that
                        still match the original below 4 kHz, which is how
                        the non-voices drop out)
  inst/sample_NNN.wav   every sample not replaced, voices included (for
                        AudioSR)
16-bit, at the sample's own rate.
"""
import csv, json, os, sys, zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from match import ROOT, read_wav, write_wav   # noqa: E402


def main():
    out, mdirs = sys.argv[1], sys.argv[2:]
    done = set()
    for d in mdirs:
        for r in csv.DictReader(open(os.path.join(d, 'matches.csv'), encoding='utf-8')):
            if r['decision'] in ('accept', 'accept-loopfit'):
                done.add(r['sample'])
    table = json.load(open(os.path.join(ROOT, 'audio/sample_table.json')))['samples']
    nv = ni = 0
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
        for e in table:
            name = e['sample'][:-4]
            if name in done:
                continue
            x, rate, _, _ = read_wav(os.path.join(ROOT, 'audio/samples', e['sample']))
            path = os.path.join(os.path.dirname(out) or '.', '.tmp_ai.wav')
            write_wav(path, x, rate)
            data = open(path, 'rb').read()
            os.remove(path)
            dur = len(x) / rate
            if 'loop' not in e and 0.15 <= dur <= 3.0:
                z.writestr('voice/' + e['sample'], data)
                nv += 1
            z.writestr('inst/' + e['sample'], data)
            ni += 1
    print('%s: %d for the voice models, %d for AudioSR (%d already replaced)' % (out, nv, ni, len(done)))


if __name__ == '__main__':
    main()
