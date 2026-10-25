"""How long a menu's transition takes: presses a button while filming the window and measures from the first frame
that changes to the last, to compare a game's interface at two frame rates (a menu that runs per frame takes half
the time at 120).

    py -3 scripts/transicao.py A [--segundos 4]"""
import argparse
import sys
import threading
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from qte import camera  # noqa: E402
from tecla import press  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('botao')
    ap.add_argument('--segundos', type=float, default=4)
    a = ap.parse_args()
    cam = camera()
    frames, t0 = [], time.time()
    threading.Timer(0.5, press, args=(a.botao, 0.1)).start()
    while time.time() - t0 < a.segundos:
        im = np.asarray(cam.grab().convert('L').resize((270, 160)), dtype=np.float32)
        frames.append((time.time() - t0, im))
    cam.close()
    changes = [(t, float(np.abs(im - prev).mean())) for (t, im), (_, prev) in zip(frames[1:], frames[:-1])]
    moving = [t for t, d in changes if d > 0.5]
    if not moving:
        print('nada mudou')
        return
    print(f'{len(frames)} quadros; a tela mudou de {moving[0] - 0.5:.2f} s a {moving[-1] - 0.5:.2f} s depois do botão: '
          f'transição de {moving[-1] - moving[0]:.2f} s')


if __name__ == '__main__':
    main()
