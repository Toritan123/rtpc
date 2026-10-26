#!/usr/bin/env python3
# Play every scene under RTPC_AUTO=4 and summarise each run: crash, whether
# it reached a results screen, and cue spawns / hits / barelies / misses /
# strays. The regression check after any change to shared code.
#
#   tools/romcmp/sweep.py OUTDIR [SCENE...]      (default: every scene)
#
# Runs 4 at a time, headless, with RTPC_SAVE=/dev/null; a run ends at the
# title screen after its results, or after LIMIT seconds (toys and endless
# games never end on their own). Drum lessons get RTPC_AUTO_IDLE=600.
# Scenes whose names end in _skipped_practice / _skipped_intro are left out.
import os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BIN = os.path.join(ROOT, 'build/macos/rhythmtengoku_pc')
PAR, LIMIT = 4, 600


def all_scenes():
    env = dict(os.environ, RTPC_SAVE='/dev/null', RTPC_HEADLESS='1', RTPC_SCENE='__list__')
    out = subprocess.run([BIN], env=env, capture_output=True, text=True).stderr
    names = re.findall(r'^  (\w+)$', out, re.M)
    return [n for n in names if not n.endswith(('skipped_practice', 'skipped_intro'))]


def summarize(log):
    pre = log.split('scene_results')[0]
    crash = '[CRASH]' in log
    res = re.search(r'enter \S+ mem=\d+ (scene_results\w*)', log)
    count = lambda w: len(re.findall(r'\[CUE\]\s+\d+ ' + w, pre))
    return ('crash=%s res=%s spawn=%d hit=%d barely=%d miss=%d stray=%d' %
            ('Y' if crash else 'n', res.group(1) if res else '-', count('spawn'), count('hit filter'),
             count('barely filter'), count('miss filter'), count('stray')))


def main():
    out = os.path.abspath(sys.argv[1]); os.makedirs(out, exist_ok=True)
    queue = sys.argv[2:] or all_scenes()
    running = {}
    summary = open(os.path.join(out, 'summary.txt'), 'w')
    while queue or running:
        while queue and len(running) < PAR:
            sc = queue.pop(0)
            f = open(os.path.join(out, sc + '.log'), 'w')
            env = dict(os.environ, RTPC_SAVE='/dev/null', RTPC_SCENE=sc, RTPC_AUTO='4',
                       RTPC_CUETRACE='1', RTPC_TRACE='1', RTPC_HEADLESS='1')
            if sc.startswith(('drum_lesson', 'drum_studio')):
                env['RTPC_AUTO_IDLE'] = '600'
            running[sc] = (subprocess.Popen([BIN], stdout=f, stderr=subprocess.STDOUT, env=env, cwd=ROOT), time.time(), f)
        time.sleep(3)
        for sc, (p, t0, f) in list(running.items()):
            f.flush()
            log = open(os.path.join(out, sc + '.log'), errors='replace').read()
            done = p.poll() is not None
            if not done and (re.search(r'enter \S+ mem=\d+ scene_title', log) or time.time() - t0 > LIMIT):
                p.kill(); done = True
            if done:
                p.wait(); f.close(); del running[sc]
                line = '%-36s %s' % (sc, summarize(open(os.path.join(out, sc + '.log'), errors='replace').read()))
                print(line, flush=True); summary.write(line + '\n'); summary.flush()


if __name__ == '__main__':
    main()
