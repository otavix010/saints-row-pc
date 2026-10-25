"""Shows the PowerPC instructions around an address of the loaded image (PORT_DUMP_IMAGE=file on a run), with the
common opcodes named and the functions the analysis knows marked, to look at code the analysis left out.

    py -3 scripts/desmontar.py imagem.bin 0x822153E0 [--antes 16] [--depois 16]"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import FUNCS, GENERATED, NAME  # noqa: E402

BASE = 0x82000000


def sx(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def decode(addr, w):
    op = w >> 26
    rd, ra, rb = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    imm = sx(w & 0xFFFF, 16)
    if w == 0x4E800020:
        return 'blr'
    if w == 0x4E800420:
        return 'bctr'
    if w == 0x4E800421:
        return 'bctrl'
    if w & 0xFC1FFFFF == 0x7C0802A6:
        return f'mflr r{rd}'
    if w & 0xFC1FFFFF == 0x7C0803A6:
        return f'mtlr r{rd}'
    if w & 0xFC1FFFFF == 0x7C0903A6:
        return f'mtctr r{rd}'
    if op == 18:
        t = sx(w & 0x3FFFFFC, 26)
        return f"b{'l' if w & 1 else ''}{'a' if w & 2 else ''} 0x{(addr + t) & 0xFFFFFFFF if not w & 2 else t & 0xFFFFFFFF:08X}"
    if op == 16:
        t = sx(w & 0xFFFC, 16)
        return f"bc{'l' if w & 1 else ''} {rd},{ra},0x{(addr + t) & 0xFFFFFFFF:08X}"
    if op == 19 and (w >> 1) & 1023 == 16:
        return f"bclr{'l' if w & 1 else ''} {rd},{ra}"
    named = {14: 'addi', 15: 'addis', 32: 'lwz', 33: 'lwzu', 34: 'lbz', 36: 'stw', 37: 'stwu', 38: 'stb', 40: 'lhz',
             44: 'sth', 48: 'lfs', 50: 'lfd', 52: 'stfs', 54: 'stfd', 24: 'ori', 25: 'oris', 28: 'andi.'}
    if op in (14, 15) and ra == 0:
        return f"{'li' if op == 14 else 'lis'} r{rd},{imm}"
    if op in (24, 25, 28):
        return f'{named[op]} r{ra},r{rd},0x{w & 0xFFFF:X}'
    if op in named:
        return f'{named[op]} r{rd},{imm}(r{ra})' if op >= 32 else f'{named[op]} r{rd},r{ra},{imm}'
    if op == 11:
        return f'cmpwi cr{rd >> 2},r{ra},{imm}'
    if op == 10:
        return f'cmplwi cr{rd >> 2},r{ra},{w & 0xFFFF}'
    if op == 58:
        return f"{['ld', 'ldu', 'lwa', '?'][w & 3]} r{rd},{sx(w & 0xFFFC, 16)}(r{ra})"
    if op == 62:
        return f"{['std', 'stdu', '?', '?'][w & 3]} r{rd},{sx(w & 0xFFFC, 16)}(r{ra})"
    if op == 31 and (w >> 1) & 1023 == 444 and rd == rb:
        return f'mr r{ra},r{rd}'
    if op == 31 and (w >> 1) & 1023 in (0, 32):
        return f"{'cmpw' if (w >> 1) & 1023 == 0 else 'cmplw'} cr{rd >> 2},r{ra},r{rb}"
    if op == 21:
        return f'rlwinm r{ra},r{rd},{rb},{(w >> 6) & 31},{(w >> 1) & 31}'
    if w == 0:
        return '(zero)'
    return f'op{op}'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('imagem')
    ap.add_argument('endereco')
    ap.add_argument('--antes', type=int, default=16)
    ap.add_argument('--depois', type=int, default=16)
    a = ap.parse_args()
    image = Path(a.imagem).read_bytes()
    center = int(a.endereco, 16)
    register = (GENERATED / f'{NAME}_register.cpp').read_text()
    known = {int(x, 16) for x in re.findall(r'SetFunction\(0x([0-9A-Fa-f]{8})', register)}
    declared = {int(x, 16) for x in re.findall(r'^0x([0-9A-Fa-f]{8})\s*=', FUNCS.read_text(), re.M)}
    for addr in range(center - 4 * a.antes, center + 4 * (a.depois + 1), 4):
        o = addr - BASE
        if not 0 <= o <= len(image) - 4:
            continue
        w = struct.unpack_from('>I', image, o)[0]
        mark = 'FUNÇÃO' if addr in known else ('declarada' if addr in declared else '')
        arrow = '>' if addr == center else ' '
        print(f'{arrow} {addr:08X}  {w:08X}  {decode(addr, w):<28} {mark}')


if __name__ == '__main__':
    main()
