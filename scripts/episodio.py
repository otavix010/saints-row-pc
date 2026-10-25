"""Plays a running port on its own for a while: the button-prompt reader with the QTE cycle (fights, QTEs), then the
explorer (walking stretches) when no prompt has come for a while, and round again. The explorer hands back at the
first cutscene or prompt it meets.

    py -3 scripts/episodio.py --minutos 60 [--luta 90] [--andar 60]"""
import argparse
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--minutos', type=float, default=60)
    ap.add_argument('--luta', type=float, default=90, help='seconds of prompts and cycle before a walk is tried')
    ap.add_argument('--andar', type=float, default=60, help='the longest walk')
    a = ap.parse_args()
    end = time.time() + a.minutos * 60
    rounds = 0
    while time.time() < end:
        fight = subprocess.run([sys.executable, '-I', str(ROOT / 'scripts' / 'qte.py'), '--jogar', str(a.luta),
                                '--ciclo'], cwd=ROOT, capture_output=True, text=True)
        presses = len([x for x in fight.stdout.splitlines() if x.strip()])
        walk = subprocess.run([sys.executable, '-I', str(ROOT / 'scripts' / 'explorar.py'), '--segundos', str(a.andar),
                               '--passo', '6'], cwd=ROOT, capture_output=True, text=True)
        rounds += 1
        print(f'{time.strftime("%H:%M:%S")} rodada {rounds}: {presses} avisos apertados; andar: '
              f'{walk.stdout.strip() or walk.stderr.strip()[-80:]}', flush=True)
        if 'não está rodando' in (fight.stderr + walk.stderr):
            print('o jogo fechou')
            return


if __name__ == '__main__':
    main()
