#!/usr/bin/env python3
# Compare a whole game on PC against the real ROM in mGBA.
#
#   tools/romcmp/cmpgame.py WORKDIR GAME [GAME...]
#
# GAME is a scene name without scene_ (games resolve through the level
# table), or NAME=0xADDR for a scene that is not a level (scene_studio is
# 0x089d85b4), or either with :FRAMES appended to stop after that many PC
# frames instead of at the results screen.
#
# For each game: run the PC build under RTPC_AUTO=4 to its results screen,
# saving a shot every 10 frames and the key log; replay that log in mGBA
# (tools/mgba_ref.c) on the script clock, saving every frame; then compare
# each PC shot with the ref frame nearest the running offset (the PC boots
# straight into the scene, so PC frame n is about ROM frame n + 28).
# Only the differing pairs are kept (WORKDIR/GAME/diff_NNNNN.png, PC left,
# ROM right); result.json holds every differing frame as [n, pixels, offset].
#
# Expected differences are listed in CLAUDE.md (agb_random, load timing, the
# first ~40 frames). Needs Pillow and numpy, and libmgba for mgba_ref.
import json, os, re, shutil, subprocess, sys, time, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from scene_addrs import scene_addrs, ROM_PATH

import numpy as np
from PIL import Image

BIN = os.path.join(ROOT, 'build/macos/rhythmtengoku_pc')
START_OFFSET = 28


def build_mgba_ref(work):
    exe = os.path.join(work, 'mgba_ref')
    src = os.path.join(ROOT, 'tools/mgba_ref.c')
    if not os.path.exists(exe) or os.path.getmtime(exe) < os.path.getmtime(src):
        subprocess.check_call(['cc', '-arch', 'x86_64', '-I/usr/local/include', src,
                               '-L/usr/local/lib', '-lmgba', '-o', exe])
    return exe


def pc_run(game, d, keylog, frames=None, limit=3000):
    os.makedirs(d)
    env = dict(os.environ, RTPC_SAVE='/dev/null', RTPC_SCENE=game, RTPC_SHOTS='10',
               RTPC_SHOT_DIR=d, RTPC_TRACE='1', RTPC_CUETRACE='1', RTPC_AUTO='4', RTPC_HEADLESS='1',
               RTPC_KEYLOG=keylog)
    logp = d + '.log'
    with open(logp, 'w') as log:
        p = subprocess.Popen([BIN], stdout=log, stderr=subprocess.STDOUT, env=env, cwd=ROOT)
        t0 = time.time()
        while p.poll() is None and time.time() - t0 < limit:
            time.sleep(3)
            txt = open(logp, errors='replace').read()
            if 'scene_results' in txt and 'scene_title' in txt.split('scene_results')[-1]:
                break
            if frames and os.path.exists(os.path.join(d, 'shot_%05d.bmp' % frames)):
                break
            # RTPC_TEST_LEVEL runs: game select -> game -> results -> epilogue
            # -> game select; stop once back at game select.
            if 'scene_epilogue' in txt and 'scene_game_select' in txt.split('scene_epilogue')[-1]:
                time.sleep(2)
                break
        p.kill(); p.wait()
    return open(logp, errors='replace').read()


def load(p):
    return np.asarray(Image.open(p).convert('RGB'), dtype=np.int16)


def compare(pcd, refd, last):
    """Compare each PC shot with the ROM frames near n + offset.

    The offset is anchored: the replay runs on the script clock, so it only
    moves when a load pauses the ROM's script for a few frames. A shot is
    first matched within +-3 frames of the current offset; only if a wider
    search finds an exact match at another offset for 3 shots in a row is
    the offset moved there. (Moving it on any exact match let a static
    screen drag it away, which then made later, identical frames differ.)
    """
    cache = {}

    def ref(k):
        if k not in cache:
            p = os.path.join(refd, 'ref_%05d.ppm' % k)
            try:
                cache[k] = load(p) if os.path.exists(p) else None
            except OSError:
                cache[k] = None
        return cache[k]

    def best(A, n, lo, hi, cur):
        b = None
        for k in range(n + lo, n + hi + 1):
            B = ref(k)
            if B is None:
                continue
            c = int((np.abs(A - B).max(axis=2) > 24).sum())
            key = (c, abs(k - n - cur))
            if b is None or key < b[0]:
                b = (key, k - n)
        return (b[0][0], b[1]) if b else None

    bad = []; cur = START_OFFSET; pending = []
    for n in range(10, last + 1, 10):
        a = os.path.join(pcd, 'shot_%05d.bmp' % n)
        if not os.path.exists(a):
            continue
        try:
            A = load(a)
        except OSError:     # the last shot, cut short when the run was stopped
            continue
        near = best(A, n, cur - 3, cur + 3, cur)
        if near is None:
            continue
        c, o = near
        if c:
            wide = best(A, n, cur - 30, cur + 60, cur)
            if wide and wide[0] == 0:
                pending.append(wide[1])
                if len(pending) >= 3 and len(set(pending[-3:])) == 1:
                    cur = pending[-1]; pending = []
            else:
                pending = []
            bad.append((n, c, o))
        else:
            pending = []
        for k in list(cache):
            if k < n + cur - 40:
                del cache[k]
    return bad


def main():
    work = os.path.abspath(sys.argv[1]); games = sys.argv[2:]
    os.makedirs(work, exist_ok=True)
    mgba = build_mgba_ref(work)
    addrs = scene_addrs()
    for spec in games:
        frames = None
        if ':' in spec:
            spec, frames = spec.split(':'); frames = int(frames)
        game, _, addr = spec.partition('=')
        addr = int(addr, 0) if addr else addrs[game]
        g = os.path.join(work, game)
        shutil.rmtree(g, ignore_errors=True); os.makedirs(g)
        keylog = os.path.join(g, 'keys.txt')
        pcd = os.path.join(g, 'pc'); refd = os.path.join(g, 'ref')
        log = pc_run(game, pcd, keylog, frames)
        shots = [int(x[5:10]) for x in os.listdir(pcd) if x.startswith('shot_')]
        last = max(shots) if shots else 0
        os.makedirs(refd)
        m = re.search(r'\[TEST\] MGBA_REF_SAVE=(\S+)', log)
        menv = dict(os.environ, MGBA_REF_SAVE=m.group(1)) if m else os.environ
        subprocess.run([mgba, ROM_PATH, hex(addr), str(last + START_OFFSET + 120), '1', refd, keylog],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=menv)
        bad = compare(pcd, refd, last)
        for n, c, o in bad:
            a = Image.open(os.path.join(pcd, 'shot_%05d.bmp' % n)).convert('RGB')
            b = Image.open(os.path.join(refd, 'ref_%05d.ppm' % (n + o))).convert('RGB')
            im = Image.new('RGB', (484, 160), (255, 0, 0)); im.paste(a, (0, 0)); im.paste(b, (244, 0))
            im.save(os.path.join(g, 'diff_%05d.png' % n))
        shutil.rmtree(pcd); shutil.rmtree(refd)
        cues = dict(collections.Counter(re.findall(r'\[CUE\]\s+\d+ (\w+)', log)))
        runs = []
        for n, c, o in bad:
            if runs and n - runs[-1][1] <= 10:
                runs[-1][1] = n; runs[-1][2] = max(runs[-1][2], c)
            else:
                runs.append([n, n, c])
        json.dump({'last': last, 'cues': cues, 'bad': bad, 'runs': runs},
                  open(os.path.join(g, 'result.json'), 'w'))
        print('%-24s frames %5d cues %s' % (game, last, cues))
        print('    differing runs [first, last, max px]:', runs[:20], flush=True)


if __name__ == '__main__':
    main()
