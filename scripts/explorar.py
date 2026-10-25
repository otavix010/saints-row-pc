"""Walks a running port's character about to find the trigger that ends a walking stretch: the left stick held in one
direction after another, the screen checked every half second for a cutscene's black bars (letterbox) or an on-screen
button prompt (qte.py), and stops there.

    py -3 scripts/explorar.py [--segundos 180] [--passo 5]"""
import argparse
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from qte import camera, detect  # noqa: E402
from tecla import press  # noqa: E402

# long runs one way, then sideways, so the walk sweeps the area rather than going round in place
ROUTE = ['LY+', 'LY+', 'LX+', 'LY+', 'LX-', 'LX-', 'LY+', 'LY-', 'LX+', 'LX+', 'LY-', 'LY-', 'LX-', 'LY+',
         'LX++LY+', 'LX-+LY+', 'LX++LY-', 'LX-+LY-']


def letterbox(a):
    """black bars across the top and the bottom of the game's picture (below the title bar)"""
    h = a.shape[0]
    top = a[int(h * 0.07):int(h * 0.11)]
    bottom = a[int(h * 0.93):int(h * 0.97)]
    middle = a[int(h * 0.4):int(h * 0.6)]
    return top.max() < 24 and bottom.max() < 24 and middle.mean() > 20


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--segundos', type=float, default=180)
    ap.add_argument('--passo', type=float, default=5)
    a = ap.parse_args()
    cam = camera()
    t0 = time.time()
    step = 0
    while time.time() - t0 < a.segundos:
        direction = ROUTE[step % len(ROUTE)]
        press(direction, a.passo)
        end = time.time() + a.passo
        while time.time() < end:
            frame = np.asarray(cam.grab())
            if letterbox(frame):
                print(f'{time.time() - t0:6.1f} s: cinemática (faixas pretas) andando {direction}')
                return
            found = detect(frame)
            if found:
                print(f'{time.time() - t0:6.1f} s: aviso {found[0][0]} andando {direction}')
                return
            time.sleep(0.5)
        step += 1
    print('nada encontrado')


if __name__ == '__main__':
    main()
