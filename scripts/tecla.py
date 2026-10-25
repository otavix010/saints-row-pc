"""Presses buttons in the running game (PORT_PAD_LIVE, which scripts/rodar.py sets): holds them for some seconds from
now.

    py -3 scripts/tecla.py LY-+RY- 1.5     both sticks down for 1.5 s
    py -3 scripts/tecla.py LROT- 3         the left stick turning anticlockwise for 3 s
    py -3 scripts/tecla.py A               A for 0.2 s"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD  # noqa: E402


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 0.2
    with open(BUILD / 'pad_live.txt', 'a', encoding='ascii') as f:
        f.write(f'{seconds} {sys.argv[1]}\n')


if __name__ == '__main__':
    main()
