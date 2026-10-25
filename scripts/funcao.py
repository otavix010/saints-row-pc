"""A recompiled function's PowerPC instructions (the comments of the generated code), with its labels, the functions it
calls and the strings and globals it makes (lis + addi/ori) named.

    py -3 scripts/funcao.py sub_824719B0"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import NAME, ROOT  # noqa: E402
from textos import strings  # noqa: E402


def body(name):
    for f in sorted((ROOT / 'generated' / 'default').glob(f'{NAME}_recomp.*.cpp')):
        text = f.read_text(encoding='utf-8', errors='replace')
        i = text.find(f'DEFINE_REX_FUNC({name})')
        if i >= 0:
            j = text.find('\nDEFINE_REX_FUNC(', i + 10)
            return text[i:j if j > 0 else len(text)]
    return None


def main():
    name = sys.argv[1]
    text = body(name)
    if text is None:
        sys.exit(f'{name} não está no código gerado')
    table = strings()
    known = {}
    for line in text.splitlines():
        if line.startswith('loc_'):
            print(line)
            continue
        if not line.startswith('\t// '):
            continue
        insn = line[4:]
        note = ''
        m = re.match(r'lis (r\d+),(-?\d+)$', insn)
        if m:
            known[m.group(1)] = (int(m.group(2)) & 0xFFFF) << 16
        m = re.match(r'(?:addi|ori) (r\d+),(r\d+),(-?\d+)$', insn)
        if m and m.group(2) in known:
            value = (known[m.group(2)] + int(m.group(3))) & 0xFFFFFFFF
            known[m.group(1)] = value
            note = f'0x{value:08X}' + (f' "{table[value][:50]}"' if value in table else '')
        m = re.match(r'\w+ (r\d+|f\d+),(-?\d+)\((r\d+)\)$', insn)
        if m and m.group(3) in known:
            note = f'[0x{(known[m.group(3)] + int(m.group(2))) & 0xFFFFFFFF:08X}]'
        print(f'    {insn:34} {note}')


if __name__ == '__main__':
    main()
