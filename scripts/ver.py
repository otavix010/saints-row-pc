"""A screenshot of the running port now (tela_agora.png), to drive it live with scripts/tecla.py.

    py -3 scripts/ver.py [--espera 1.5]     waits that long first (a button just pressed)"""
import argparse
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import EXE, ROOT  # noqa: E402

CAPTURE = r'C:\Users\otavi\fifast2-port\scripts\captura.ps1'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--espera', type=float, default=0)
    a = ap.parse_args()
    time.sleep(a.espera)
    out = subprocess.run(['tasklist', '/FI', f'IMAGENAME eq {EXE.name}', '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout.strip().splitlines()
    if not out or EXE.name not in out[0]:
        sys.exit('o jogo não está rodando')
    pid = out[0].split('","')[1]
    dst = ROOT / 'tela_agora.png'
    subprocess.run(['powershell', '-NoProfile', '-File', CAPTURE, '-Id', pid, '-Saida', str(dst)], capture_output=True)
    print(dst)


if __name__ == '__main__':
    main()
