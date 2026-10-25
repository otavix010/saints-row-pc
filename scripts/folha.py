"""Puts the run's screenshots (tela_<s>.png) side by side in folha.png, 7 a row, each labelled with its second.

    py -3 scripts/folha.py [--largura 320]"""
import argparse
import glob
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--largura', type=int, default=320)
    a = ap.parse_args()
    files = sorted(glob.glob(str(ROOT / 'tela_*.png')), key=lambda f: int(Path(f).stem[5:]))
    if not files:
        print('sem prints')
        return
    ims = [Image.open(f).convert('RGB') for f in files]
    w = a.largura
    h = int(ims[0].height * w / ims[0].width)
    cols = min(7, len(ims))
    rows = (len(ims) + cols - 1) // cols
    out = Image.new('RGB', (w * cols, h * rows))
    for i, (f, im) in enumerate(zip(files, ims)):
        x, y = (i % cols) * w, (i // cols) * h
        out.paste(im.resize((w, h)), (x, y))
        ImageDraw.Draw(out).text((x + 6, y + 16), Path(f).stem[5:] + ' s', fill=(255, 255, 0))
    out.save(ROOT / 'folha.png')
    print(f'{len(files)} prints em folha.png')


if __name__ == '__main__':
    main()
