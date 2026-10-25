"""Films the running port's window at the fastest rate it can (PrintWindow, in this process), to see on-screen button
prompts that last less than a second: the frames go to filme/ with their times, and a contact sheet to filme.png.
Or plays them: a button icon on screen (a disc of the button's colour, its white letter inside, a crisp edge) is
pressed as soon as two frames running show it.

    py -3 scripts/qte.py --filmar 6 [--folha 32]
    py -3 scripts/qte.py --jogar 600           presses the prompts for 10 minutes, the ones seen kept in qte/
    py -3 scripts/qte.py --jogar 600 --ciclo   and goes round rodar.py's QTE cycle meanwhile (tecla.py --ciclo)"""
import argparse
import ctypes
import ctypes.wintypes as wt
import functools
import shutil
import sys
import threading
import time
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import EXE, ROOT, _windows  # noqa: E402
from pilhas import find_pid  # noqa: E402

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32
user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))  # per monitor v2: physical pixels on any monitor


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long), ('biPlanes', wt.WORD),
                ('biBitCount', wt.WORD), ('biCompression', wt.DWORD), ('biSizeImage', wt.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', wt.DWORD),
                ('biClrImportant', wt.DWORD)]


class Camera:
    """the window's client area, grabbed with PrintWindow (PW_RENDERFULLCONTENT) into one reused bitmap"""

    def __init__(self, hwnd):
        self.hwnd = hwnd
        r = wt.RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(r))
        self.w, self.h = r.right - r.left, r.bottom - r.top
        self.screen = user32.GetDC(None)
        self.dc = gdi32.CreateCompatibleDC(self.screen)
        self.bmp = gdi32.CreateCompatibleBitmap(self.screen, self.w, self.h)
        gdi32.SelectObject(self.dc, self.bmp)
        self.info = BITMAPINFOHEADER(biSize=ctypes.sizeof(BITMAPINFOHEADER), biWidth=self.w, biHeight=-self.h,
                                     biPlanes=1, biBitCount=32)
        self.buffer = ctypes.create_string_buffer(self.w * self.h * 4)

    def grab(self):
        user32.PrintWindow(self.hwnd, self.dc, 2)
        gdi32.GetDIBits(self.dc, self.bmp, 0, self.h, self.buffer, ctypes.byref(self.info), 0)
        return Image.frombuffer('RGB', (self.w, self.h), self.buffer, 'raw', 'BGRX', 0, 1)

    def close(self):
        gdi32.DeleteObject(self.bmp)
        gdi32.DeleteDC(self.dc)
        user32.ReleaseDC(None, self.screen)


def camera():
    pid = find_pid(EXE.name)
    if not pid:
        sys.exit('o jogo não está rodando')
    found = _windows(pid)
    if not found:
        sys.exit('janela do jogo não encontrada')
    return Camera(found[0])


# Icon radii looked for, in half-resolution pixels: Asura's Wrath's prompts are 27 to 30 across at half size
RADII = (13, 18)
GRID = 20  # a letter's shape is compared at GRID x GRID
MATCH = 0.45  # the least overlap with a letter's template that counts as reading it
MARGIN = 0.08  # and by how much it must beat the next letter's


def _box_fraction(mask, r):
    """the count of mask set in the (2r+1)-wide box around every pixel (an integral image; outside counts as unset),
    and the box's area"""
    h, w = mask.shape
    side = 2 * r + 1
    ii = np.zeros((h + side, w + side), np.int32)
    ii[r + 1:r + 1 + h, r + 1:r + 1 + w] = mask
    ii = ii.cumsum(0).cumsum(1)
    total = ii[side:, side:] - ii[:-side, side:] - ii[side:, :-side] + ii[:-side, :-side]
    return total, side * side


def _normalize(mask):
    """a letter's ink cropped to its box and scaled to GRID x GRID, or None when there is too little of it"""
    ys, xs = np.nonzero(mask)
    if len(ys) < 12:
        return None
    crop = mask[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    return np.asarray(Image.fromarray(crop.astype(np.uint8) * 255).resize((GRID, GRID), Image.BILINEAR)) > 127


@functools.cache
def _templates():
    """A, B, X and Y in a bold sans font, as _normalize leaves a letter"""
    from PIL import ImageDraw, ImageFont
    font = ImageFont.truetype('arialbd.ttf', 96)
    out = {}
    for letter in 'ABXY':
        im = Image.new('L', (128, 128))
        ImageDraw.Draw(im).text((16, 4), letter, fill=255, font=font)
        out[letter] = _normalize(np.asarray(im) > 127)
    return out


def _largest_part(mask):
    """the largest 4-connected part of a small mask (the letter, without the icon's glossy highlight above it)"""
    seen = np.zeros_like(mask)
    best = []
    h, w = mask.shape
    for sy, sx in zip(*np.nonzero(mask)):
        if seen[sy, sx]:
            continue
        part, todo = [], [(sy, sx)]
        seen[sy, sx] = True
        while todo:
            y, x = todo.pop()
            part.append((y, x))
            for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    todo.append((ny, nx))
        if len(part) > len(best):
            best = part
    out = np.zeros_like(mask)
    if best:
        ys, xs = zip(*best)
        out[list(ys), list(xs)] = True
    return out


def read_letter(rgb, x, y, radius):
    """the white letter inside the icon centred at x, y (full size, radius in full-size pixels): (letter, match 0-1),
    the match being the overlap of its shape with the template's (intersection over union)"""
    r = int(radius * 0.9)
    patch = rgb[max(y - r, 0):y + r + 1, max(x - r, 0):x + r + 1].astype(np.int16)
    mn = patch.min(-1)
    white = (mn > 185) & (patch.max(-1) - mn < 75)
    yy, xx = np.ogrid[:white.shape[0], :white.shape[1]]
    white &= (yy - white.shape[0] / 2) ** 2 + (xx - white.shape[1] / 2) ** 2 <= r * r
    letter = _normalize(_largest_part(white))
    if letter is None:
        return None, 0.0
    matches = sorted(((float((letter & t).sum() / max((letter | t).sum(), 1)), name)
                      for name, t in _templates().items()), reverse=True)
    if matches[0][0] - matches[1][0] < MARGIN:
        return None, matches[0][0]
    return matches[0][1], matches[0][0]


def detect(rgb):
    """the button icons in a frame (H x W x 3, uint8): [(button, match, x, y)] in full-size pixels, best first. An icon
    is a disc of colour, its white letter inside and darker all round; the letter is read from its shape, since a
    game may colour every prompt alike (Asura's Wrath: orange while it waits, green once pressed)"""
    a = rgb[::2, ::2].astype(np.float32) / 255.0
    mx = a.max(-1)
    mn = a.min(-1)
    sat = (mx - mn) / (mx + 1e-6)
    white = (mn > 0.8) & (sat < 0.25)
    vivid = (sat > 0.3) & (mx > 0.6)  # A's green disc is darker and paler than Y's yellow one
    found = []
    for radius in RADII:
        w_in, a_in = _box_fraction(white, radius // 2)
        w_disc, a_disc = _box_fraction(white, radius)
        f_disc, _ = _box_fraction(vivid, radius)
        bright_out, a_out = _box_fraction(white | vivid, radius + radius // 2)
        ring = (bright_out - w_disc - f_disc) / np.maximum(a_out - a_disc, 1)
        w_in = w_in / a_in
        f_disc = f_disc / a_disc
        ok = (w_in > 0.1) & (w_in < 0.75) & (ring < 0.3) & (f_disc > 0.25) & (f_disc + w_disc / a_disc > 0.55)
        score = np.where(ok, f_disc - ring, -1)
        i = int(score.argmax())
        if score.flat[i] > 0:
            y, x = divmod(i, score.shape[1])
            letter, match = read_letter(rgb, x * 2, y * 2, radius * 2)
            if letter and match >= MATCH:
                found.append((letter, float(match), x * 2, y * 2))
    found.sort(key=lambda f: -f[1])
    return found


def play(seconds, with_cycle=False):
    """presses each button icon seen on two frames running; each press and its frame go to qte/"""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from tecla import live_cycle, press
    if with_cycle:
        threading.Thread(target=live_cycle, args=(seconds,), daemon=True).start()
    out = ROOT / 'qte'
    out.mkdir(exist_ok=True)
    cam = camera()
    last, cooldown, kept, t0 = None, {}, 0, time.time()
    while time.time() - t0 < seconds:
        im = cam.grab()
        found = detect(np.asarray(im))
        best = found[0][0] if found else None
        now = time.time()
        if best and best == last and now >= cooldown.get(best, 0):
            press(best, 0.1)
            cooldown[best] = now + 0.5
            button, score, x, y = found[0]
            print(f'{now - t0:7.2f} s  {button}  {score:.2f} em {x},{y}', flush=True)
            if kept < 200:
                im.copy().save(out / f'{now - t0:07.2f}_{button}.png')
                kept += 1
        last = best
    cam.close()


def film(seconds, sheet):
    out = ROOT / 'filme'
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir()
    cam = camera()
    frames, t0 = [], time.time()
    while time.time() - t0 < seconds:
        frames.append((time.time() - t0, cam.grab().copy()))
    cam.close()
    for i, (t, im) in enumerate(frames):
        im.save(out / f'{i:04d}_{t:05.2f}.png')
    print(f'{len(frames)} quadros em {seconds} s ({len(frames) / seconds:.0f} por segundo) em {out}')
    step = max(1, len(frames) // sheet)
    chosen = frames[::step][:sheet]
    w = 270
    h = int(cam.h * w / cam.w)
    cols = 8
    rows = (len(chosen) + cols - 1) // cols
    board = Image.new('RGB', (w * cols, h * rows))
    for i, (t, im) in enumerate(chosen):
        board.paste(im.resize((w, h)), (w * (i % cols), h * (i // cols)))
    board.save(ROOT / 'filme.png')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--filmar', type=float)
    ap.add_argument('--folha', type=int, default=32)
    ap.add_argument('--jogar', type=float)
    ap.add_argument('--ciclo', action='store_true')
    a = ap.parse_args()
    if a.jogar:
        play(a.jogar, a.ciclo)
    else:
        film(a.filmar or 6, a.folha)


if __name__ == '__main__':
    main()
