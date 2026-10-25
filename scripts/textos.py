"""Which of the game's recompiled functions use which of its strings: every lis/addi (or lis/ori) pair in the generated
code that makes the address of a string from imagem.bin (PORT_DUMP_IMAGE), by the function it is in. To find a
subsystem's code from its messages (EA's APT, a save system, a menu).

    py -3 scripts/textos.py "(?i)apt|flash"     the functions using strings that match, with the strings
    py -3 scripts/textos.py --funcao sub_82629450   the strings that function uses
    py -3 scripts/textos.py --endereco 0x82AE9CC0   the functions making that address (or loading/storing there)"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import NAME, ROOT  # noqa: E402

BASE = 0x82000000
HEADER = re.compile(r'DEFINE_REX_FUNC\((sub_[0-9A-F]{8})\)')
LIS = re.compile(r'\t// lis (r\d+),(-?\d+)$')
LOW = re.compile(r'\t// (?:addi|ori) (r\d+),(r\d+),(-?\d+)$')
MOVE = re.compile(r'\t// mr (r\d+),(r\d+)$')


def strings():
    data = (ROOT / 'imagem.bin').read_bytes()
    return {BASE + m.start(): m.group()[:-1].decode('latin-1') for m in re.finditer(rb'[ -~]{4,200}\x00', data)}


def uses(table):
    """{function: {string address}}"""
    out = {}
    for f in sorted((ROOT / 'generated' / 'default').glob(f'{NAME}_recomp.*.cpp')):
        func, high = None, {}
        for line in open(f, encoding='utf-8', errors='replace'):
            m = HEADER.match(line)
            if m:
                func, high = m.group(1), {}
                continue
            m = LIS.match(line)
            if m:
                high[m.group(1)] = (int(m.group(2)) & 0xFFFF) << 16
                continue
            m = LOW.match(line)
            if m and m.group(2) in high and func:
                address = (high[m.group(2)] + int(m.group(3))) & 0xFFFFFFFF
                if address in table:
                    out.setdefault(func, set()).add(address)
                high[m.group(1)] = address  # a base kept in a register, offset again later
                continue
            m = MOVE.match(line)
            if m:
                if m.group(2) in high:
                    high[m.group(1)] = high[m.group(2)]
                else:
                    high.pop(m.group(1), None)
    return out


MEMORY = re.compile(r'\t// (l[bhwf][a-z]*|st[bhwf][a-z]*) (r\d+|f\d+),(-?\d+)\((r\d+)\)$')


def who_uses(targets):
    """{address: [(function, instruction)]}: where each address is made (lis + addi/ori) or read and written (lis +
    a load/store displacement)"""
    out = {t: [] for t in targets}
    for f in sorted((ROOT / 'generated' / 'default').glob(f'{NAME}_recomp.*.cpp')):
        func, known = None, {}
        for line in open(f, encoding='utf-8', errors='replace'):
            m = HEADER.match(line)
            if m:
                func, known = m.group(1), {}
                continue
            m = LIS.match(line)
            if m:
                known[m.group(1)] = (int(m.group(2)) & 0xFFFF) << 16
                continue
            m = LOW.match(line)
            if m:
                if m.group(2) in known:
                    value = (known[m.group(2)] + int(m.group(3))) & 0xFFFFFFFF
                    if value in out:
                        out[value].append((func, line.strip()[3:]))
                    known[m.group(1)] = value
                else:
                    known.pop(m.group(1), None)
                continue
            m = MEMORY.match(line)
            if m and m.group(4) in known:
                value = (known[m.group(4)] + int(m.group(3))) & 0xFFFFFFFF
                if value in out:
                    out[value].append((func, line.strip()[3:]))
                if m.group(1).startswith('l') and m.group(2).startswith('r'):
                    known.pop(m.group(2), None)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('padrao', nargs='?', default='.')
    ap.add_argument('--funcao')
    ap.add_argument('--endereco', nargs='+')
    a = ap.parse_args()
    if a.endereco:
        for address, places in who_uses([int(x, 16) for x in a.endereco]).items():
            print(f'{address:08X}: {len(places)} usos')
            for func, insn in places:
                print(f'    {func}  {insn}')
        return
    table = strings()
    found = uses(table)
    if a.funcao:
        for address in sorted(found.get(a.funcao, ())):
            print(f'{address:08X}  {table[address][:120]}')
        return
    pattern = re.compile(a.padrao)
    for func, addresses in sorted(found.items()):
        hits = [table[x] for x in sorted(addresses) if pattern.search(table[x])]
        if hits:
            print(f'{func}  ' + ' | '.join(h[:60] for h in hits)[:220])


if __name__ == '__main__':
    main()
