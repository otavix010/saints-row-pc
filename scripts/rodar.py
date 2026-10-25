"""Runs the port in a window on the left monitor (never fullscreen) for some seconds, with screenshots and the frame
rate of every second.

    py -3 scripts/rodar.py [--segundos 60] [--prints 15,30,55 | --cada 30] [--pad ROTEIRO] [--ciclo 80]
                           [--var NOME=valor ...]

--pad adds a controller script (PORT_PAD syntax, pad_script.h); --ciclo S from second S goes round every button, the
triggers, both sticks' four directions and the left stick turning, for QTEs met unseen; scripts/tecla.py presses
more while it runs. Screenshots go to tela_<s>.png, the log to out/build/win-amd64-release/run.log, the frame rate
to fps.log there (summed up per window at the end)."""
import argparse
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD, EXE, ROOT, move_left, test_env, window_state  # noqa: E402

CAPTURE = r'C:\Users\otavi\fifast2-port\scripts\captura.ps1'


def cycle(start, end):
    """every button, the triggers, both sticks together in the four directions and pulled apart or pushed together,
    and the left stick turning round both ways, round and round (16 s), for QTEs met unseen"""
    period, out = 16.0, []
    steps = [(0, 'A', 0.15), (0.5, 'B', 0.15), (1, 'X', 0.15), (1.5, 'Y', 0.15), (2, 'RT', 0.4),
             (2.5, 'LY-+RY-', 0.8), (3.5, 'LY++RY+', 0.8), (4.5, 'LX-+RX-', 0.8), (5.5, 'LX++RX+', 0.8),
             (6.5, 'LT+RT', 0.4), (8, 'LROT', 1.8), (10, 'LROT-', 1.8),
             (12, 'LX-+RX+', 1.0), (13.2, 'LX++RX-', 1.0), (14.4, 'LY++RY-', 0.7), (15.2, 'LY-+RY+', 0.7)]
    steps += [(7 + i * 0.2, 'B', 0.1) for i in range(5)]  # a burst, for the mashing ones
    for at, button, hold in steps:
        out.append(f'{start + at:g}-{end:g}@{period:g}/{hold:g}:{button}')
    return out


def cpu_seconds(pid):
    """the process's CPU time so far (user + kernel, all threads), from GetProcessTimes"""
    import ctypes
    handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not handle:
        return None
    times = [ctypes.c_ulonglong() for _ in range(4)]
    ok = ctypes.windll.kernel32.GetProcessTimes(handle, *[ctypes.byref(t) for t in times])
    ctypes.windll.kernel32.CloseHandle(handle)
    return (times[2].value + times[3].value) / 1e7 if ok else None


def start_gpu_sampling(path):
    """nvidia-smi writing the GPU's use once a second, when there is one"""
    try:
        return subprocess.Popen(['nvidia-smi', '--query-gpu=utilization.gpu', '--format=csv,noheader,nounits', '-l', '1'],
                                stdout=open(path, 'w'), stderr=subprocess.DEVNULL)
    except OSError:
        return None


def summarize_fps(path, window):
    """the frame rate per window of seconds: lowest and mean of the per-second counts"""
    if not path.exists():
        return
    rows = [line.split() for line in path.read_text().splitlines() if line.strip()]
    per_window = {}
    for t, n in rows:
        per_window.setdefault(int(float(t) // window), []).append(int(n))
    print('fps por trecho (mínimo / média):')
    for w, counts in sorted(per_window.items()):
        print(f'  {w * window:>5.0f}-{(w + 1) * window:<5.0f} s  {min(counts):>3} / {sum(counts) / len(counts):5.1f}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--segundos', type=float, default=60)
    ap.add_argument('--prints', default='15,30,55')
    ap.add_argument('--cada', type=float, default=0)
    ap.add_argument('--pad', default='')
    ap.add_argument('--ciclo', type=float, default=0)
    ap.add_argument('--var', action='append', default=[])
    ap.add_argument('--y', type=int, default=300, help='the height of the window on the left monitor (1000: the lower half)')
    a = ap.parse_args()
    if a.cada:
        a.prints = ','.join(str(int(x * a.cada)) for x in range(1, int(a.segundos / a.cada) + 1))
    pad = [x for x in a.pad.split(',') if x]
    if a.ciclo:
        pad += cycle(a.ciclo, a.segundos)
    live = BUILD / 'pad_live.txt'
    live.write_text('', encoding='ascii')
    log = BUILD / 'run.log'
    log.unlink(missing_ok=True)
    fps_log = BUILD / 'fps.log'
    fps_log.unlink(missing_ok=True)
    env = test_env(log, PORT_PAD=','.join(pad), PORT_PAD_LIVE=live, PORT_FPS_LOG=fps_log)
    for v in a.var:
        k, _, val = v.partition('=')
        env[k] = val
    p = subprocess.Popen([str(EXE)], cwd=BUILD, env=env)
    gpu_log = BUILD / 'gpu.log'
    gpu = start_gpu_sampling(gpu_log)
    marks = sorted(float(x) for x in a.prints.split(',') if x)
    t0, moved, last_state, cpu = time.time(), False, None, None
    while time.time() - t0 < a.segundos and p.poll() is None:
        cpu = cpu_seconds(p.pid) or cpu
        time.sleep(0.3)
        if not moved:
            moved = move_left(p.pid, a.y)
        state = window_state(p.pid)
        if moved and state != last_state:
            print(f'janela aos {time.time() - t0:.1f} s: {state}', flush=True)
            last_state = state
            # back to the left monitor whenever it leaves it (it is never to sit on the main one)
            if any(x > -500 for x, y, w, h, zoomed in state):
                move_left(p.pid, a.y)
        while marks and time.time() - t0 >= marks[0]:
            s = int(marks.pop(0))
            subprocess.run(['powershell', '-NoProfile', '-File', CAPTURE, '-Id', str(p.pid),
                            '-Saida', str(ROOT / f'tela_{s}.png')], capture_output=True)
            print(f'print aos {s} s', flush=True)
    elapsed = time.time() - t0
    if p.poll() is None:
        cpu = cpu_seconds(p.pid) or cpu
        p.kill()
        p.wait()
        print(f'fechado aos {a.segundos:.0f} s')
    else:
        print(f'saiu com {p.returncode} aos {elapsed:.0f} s')
    if gpu:
        gpu.kill()
    if cpu:
        print(f'CPU: {cpu / elapsed:.1f} núcleos em média')
    if gpu_log.exists():
        use = [int(x) for x in gpu_log.read_text().split() if x.isdigit()]
        if use:
            print(f'GPU: {sum(use) / len(use):.0f}% em média, {max(use)}% no máximo')
    summarize_fps(fps_log, a.cada or 30)


if __name__ == '__main__':
    main()
