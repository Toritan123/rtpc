#!/usr/bin/env python3
"""Run a command and kill it before it can freeze the machine.

    tools/samplematch/memguard.py [--max-rss MB] [--max-swap-growth MB] -- CMD...

Written after AudioSR, run directly, hung this 8 GB Mac until the kernel's
watchdog panicked it. Every 0.3 s it checks, from outside the job:
  - macOS's own memory pressure level (kern.memorystatus_vm_pressure_level;
    4 = critical): kill at critical;
  - swap in use (vm.swapusage): kill if it grows by more than
    --max-swap-growth (default 1500 MB) since the start;
  - the job's resident memory, children included: kill above --max-rss
    (default 5000 MB).
The job is killed with SIGKILL (its whole process group), the reason is
printed, and the peaks are reported at the end either way. Exit status is
the job's, or 137 when the guard killed it.
"""
import os, re, signal, subprocess, sys, time


def sysctl(name):
    try:
        return subprocess.run(['sysctl', '-n', name], capture_output=True, text=True).stdout.strip()
    except Exception:
        return ''


def swap_used_mb():
    m = re.search(r'used = ([\d.]+)M', sysctl('vm.swapusage'))
    return float(m.group(1)) if m else 0.0


def pressure():
    try:
        return int(sysctl('kern.memorystatus_vm_pressure_level') or 1)
    except ValueError:
        return 1


def tree_rss_mb(pgid):
    out = subprocess.run(['ps', '-axo', 'pgid=,rss='], capture_output=True, text=True).stdout
    total = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == str(pgid):
            total += int(parts[1])
    return total / 1024.0


def main():
    args = sys.argv[1:]
    max_rss, max_swap = 5000.0, 1500.0
    while args and args[0] != '--':
        if args[0] == '--max-rss':
            max_rss = float(args[1]); args = args[2:]
        elif args[0] == '--max-swap-growth':
            max_swap = float(args[1]); args = args[2:]
        else:
            sys.exit('usage: memguard.py [--max-rss MB] [--max-swap-growth MB] -- CMD...')
    cmd = args[1:]
    swap0 = swap_used_mb()
    p = subprocess.Popen(cmd, start_new_session=True)
    peak_rss = peak_swap = 0.0
    peak_pressure = 1
    reason = None
    t0 = time.time()
    while p.poll() is None:
        rss = tree_rss_mb(p.pid)
        swap = swap_used_mb() - swap0
        lvl = pressure()
        peak_rss, peak_swap, peak_pressure = max(peak_rss, rss), max(peak_swap, swap), max(peak_pressure, lvl)
        if lvl >= 4:
            reason = 'memory pressure critical'
        elif swap > max_swap:
            reason = 'swap grew by %.0f MB' % swap
        elif rss > max_rss:
            reason = 'job uses %.0f MB' % rss
        if reason:
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            p.wait()
            break
        time.sleep(0.3)
    print('[memguard] %s after %.0f s; peak job RSS %.0f MB, swap growth %.0f MB, pressure level %d'
          % ('KILLED: ' + reason if reason else 'finished', time.time() - t0, peak_rss, peak_swap, peak_pressure),
          file=sys.stderr, flush=True)
    sys.exit(137 if reason else p.returncode)


if __name__ == '__main__':
    main()
