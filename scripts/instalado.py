"""Runs a port as the generator left it for the player (C:\\OtavixPorts\\<id>\\jogar.cmd), but in a window on the left
monitor, never fullscreen, with the frame counter: the player's path checked without taking over the main screen.
Prints the options in force from its log and the frame rate after some seconds, and leaves a print.

    py -3 scripts/instalado.py C:\\OtavixPorts\\dirt-2 dirt2 [--segundos 60] [--y 1000] [--pad "12:START,20:START"]"""
import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pilhas import find_pid  # noqa: E402
from port import move_left  # noqa: E402

CAPTURE = r'C:\Users\otavi\fifast2-port\scripts\captura.ps1'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pasta')
    ap.add_argument('exe')
    ap.add_argument('--segundos', type=float, default=60)
    ap.add_argument('--y', type=int, default=1000)
    ap.add_argument('--pad', default='')
    a = ap.parse_args()
    folder = Path(a.pasta)
    build = folder / 'out' / 'build' / 'win-amd64-release'
    env = dict(os.environ, REX_FULLSCREEN='false', REX_MOSTRAR_FPS='true', REX_HEADLESS='true',
               PORT_FPS_LOG=str(build / 'fps.log'))
    if a.pad:
        env['PORT_PAD'] = a.pad
    subprocess.run(['taskkill', '/F', '/IM', f'{a.exe}.exe'], capture_output=True)
    time.sleep(2)
    subprocess.run(['cmd', '/c', str(folder / 'jogar.cmd')], env=env, cwd=folder)
    t0 = time.time()
    pid = None
    while time.time() - t0 < 20 and not pid:
        pid = find_pid(f'{a.exe}.exe')
        time.sleep(0.2)
    if not pid:
        sys.exit('o jogo não abriu')
    while time.time() - t0 < 20 and not move_left(pid, a.y):
        time.sleep(0.1)
    time.sleep(max(a.segundos - (time.time() - t0), 0))
    log = (build / 'run.log') if (build / 'run.log').exists() else None
    logs = sorted(folder.rglob('*.log'), key=lambda p: p.stat().st_mtime)
    text = ''
    for p in reversed(logs):
        t = p.read_text(errors='replace')
        if 'port options' in t:
            text = t
            print(f'log: {p}')
            break
    m = re.findall(r'port options: (.*)', text)
    print('opções:', m[-1] if m else '(não achei no log)')
    fps_log = build / 'fps.log'
    if fps_log.exists():
        values = [int(x.split()[1]) for x in fps_log.read_text().split('\n')[-11:] if len(x.split()) == 2]
        print('fps nos últimos segundos:', values)
    out = folder / 'tela_instalado.png'
    subprocess.run(['powershell', '-NoProfile', '-File', CAPTURE, '-Id', str(pid), '-Saida', str(out)],
                   capture_output=True)
    print(f'print: {out}')
    _ = log


if __name__ == '__main__':
    main()
