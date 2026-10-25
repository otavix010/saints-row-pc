"""Presses buttons in the running game (PORT_PAD_LIVE, which scripts/rodar.py sets): holds them for some seconds from
now, or, with --ciclo, goes round rodar.py's QTE cycle live for that many seconds (a game already running, from any
point).

    py -3 scripts/tecla.py LY-+RY- 1.5     both sticks down for 1.5 s
    py -3 scripts/tecla.py LROT- 3         the left stick turning anticlockwise for 3 s
    py -3 scripts/tecla.py A               A for 0.2 s
    py -3 scripts/tecla.py --ciclo 600     the QTE cycle for 10 minutes"""
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD  # noqa: E402

LIVE = BUILD / 'pad_live.txt'


def press(buttons, seconds):
    with open(LIVE, 'a', encoding='ascii') as f:
        f.write(f'{seconds} {buttons}\n')


def live_cycle(total):
    from rodar import cycle  # the same steps: '<from>-<to>@<period>/<hold>:<buttons>'
    steps = []
    for item in cycle(0, 1):
        when, buttons = item.split(':', 1)
        start = float(when.split('-')[0])
        period, hold = when.split('@')[1].split('/')
        steps.append((start, float(hold), buttons, float(period)))
    period = steps[0][3]
    t0 = time.time()
    while time.time() - t0 < total:
        base = time.time()
        for start, hold, buttons, _ in sorted(steps):
            wait = base + start - time.time()
            if wait > 0:
                time.sleep(wait)
            press(buttons, hold)
        rest = base + period - time.time()
        if rest > 0:
            time.sleep(rest)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if sys.argv[1] == '--ciclo':
        live_cycle(float(sys.argv[2]) if len(sys.argv) > 2 else 600)
        return
    press(sys.argv[1], float(sys.argv[2]) if len(sys.argv) > 2 else 0.2)


if __name__ == '__main__':
    main()
