"""Checks the GPU command trace in run.log (PORT_GPU_TRACE, written when a command list's Close fails) for what the
Direct3D 12 runtime rejects without its validation layer: texture copies out of the destination mip or not aligned to
the format's blocks, footprints with a misaligned offset or row pitch, viewports and scissors out of bounds.

    py -3 scripts/verificar_trace.py [run.log]"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD  # noqa: E402

# DXGI formats with 4x4 blocks: BC1-BC7
BLOCK = set(range(70, 85)) | set(range(94, 100))

LOC = re.compile(r'(?:subresource (\d+)|footprint off (\d+) (\d+)x(\d+)x(\d+) pitch (\d+) fmt (\d+)) '
                 r'(?:of|in) res (\d+)x(\d+)x(\d+) fmt (\d+) mips (\d+) dim (\d+)')


def parse_loc(text):
    m = LOC.search(text)
    if not m:
        return None
    g = m.groups()
    return {'sub': None if g[0] is None else int(g[0]),
            'off': None if g[1] is None else int(g[1]),
            'fw': int(g[2]) if g[2] else None, 'fh': int(g[3]) if g[3] else None, 'fd': int(g[4]) if g[4] else None,
            'pitch': int(g[5]) if g[5] else None, 'ffmt': int(g[6]) if g[6] else None,
            'w': int(g[7]), 'h': int(g[8]), 'd': int(g[9]), 'fmt': int(g[10]), 'mips': int(g[11]), 'dim': int(g[12])}


def main():
    log = Path(sys.argv[1]) if len(sys.argv) > 1 else BUILD / 'run.log'
    lines = [l.split('trace: ', 1)[1] for l in log.read_text(errors='replace').splitlines() if 'trace: ' in l]
    print(f'{len(lines)} comandos no registro')
    problems = 0
    for i, line in enumerate(lines):
        issues = []
        if 'CopyTextureRegion' in line or 'CopyTexture ' in line:
            m = re.search(r'at (\d+),(\d+),(\d+) dst (.*?) src (.*?)( box (\d+),(\d+),(\d+)-(\d+),(\d+),(\d+))?$', line)
            if not m:
                m2 = re.search(r'CopyTexture dst (.*?) src (.*)$', line)
                dst, src, x, y, box = parse_loc(m2.group(1)), parse_loc(m2.group(2)), 0, 0, None
            else:
                x, y = int(m.group(1)), int(m.group(2))
                dst, src = parse_loc(m.group(4)), parse_loc(m.group(5))
                box = tuple(int(v) for v in m.groups()[6:12]) if m.group(6) else None
            if dst and src:
                block = dst['fmt'] in BLOCK
                if dst['sub'] is not None:
                    mip = dst['sub'] % max(dst['mips'], 1)
                    mw, mh = max(dst['w'] >> mip, 1), max(dst['h'] >> mip, 1)
                    if block:  # a block-compressed mip is stored in whole blocks
                        mw, mh = (mw + 3) // 4 * 4, (mh + 3) // 4 * 4
                else:
                    mw, mh = dst['fw'], dst['fh']
                if box:
                    cw, ch = box[3] - box[0], box[4] - box[1]
                elif src['sub'] is None:
                    cw, ch = src['fw'], src['fh']
                else:
                    smip = src['sub'] % max(src['mips'], 1)
                    cw, ch = max(src['w'] >> smip, 1), max(src['h'] >> smip, 1)
                if x + cw > mw or y + ch > mh:
                    issues.append(f'cópia {cw}x{ch} em {x},{y} passa do mip de destino {mw}x{mh}')
                if block and (x % 4 or y % 4 or cw % 4 or ch % 4):
                    issues.append(f'formato em blocos {dst["fmt"]} com cópia não alinhada a 4: {x},{y} {cw}x{ch}')
                for loc, name in ((src, 'origem'), (dst, 'destino')):
                    if loc['off'] is not None:
                        if loc['off'] % 512:
                            issues.append(f'{name}: offset {loc["off"]} não alinhado a 512')
                        if loc['pitch'] % 256:
                            issues.append(f'{name}: pitch {loc["pitch"]} não alinhado a 256')
                        if loc['ffmt'] in BLOCK and (loc['fw'] % 4 or loc['fh'] % 4):
                            issues.append(f'{name}: footprint {loc["fw"]}x{loc["fh"]} em blocos não alinhado a 4')
                        if box and (box[3] > loc['fw'] or box[4] > loc['fh']):
                            issues.append(f'{name}: caixa {box} passa do footprint {loc["fw"]}x{loc["fh"]}')
        m = re.search(r'Viewport (\S+) (\S+) (\S+)x(\S+) depth (\S+)-(\S+)', line)
        if m:
            x, y, w, h, d0, d1 = (float(v) for v in m.groups())
            if w < 0 or h < 0 or x < -32768 or y < -32768 or x + w > 32767 or y + h > 32767 or not 0 <= d0 <= d1 <= 1:
                issues.append('viewport fora dos limites')
        m = re.search(r'Scissor (-?\d+),(-?\d+)-(-?\d+),(-?\d+)', line)
        if m:
            l, t, r, b = (int(v) for v in m.groups())
            if l > r or t > b or l < -32768 or t < -32768 or r > 32767 or b > 32767:
                issues.append('scissor inválido')
        if issues:
            problems += 1
            print(f'[{i}] {line}')
            for s in issues:
                print('      ->', s)
    print(f'{problems} comando(s) suspeito(s)')


if __name__ == '__main__':
    main()
