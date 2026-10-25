"""Names the frames of falha.log: the port's from the linker map (<name>.map), the DLLs' from their exports.

    py -3 scripts/pilha.py [falha.log] [--pasta out/build/win-amd64-release]"""
import argparse
import bisect
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD, NAME  # noqa: E402


def map_symbols(path):
    """(rva, name) sorted, from an lld-link /MAP file."""
    base, out = 0x140000000, []
    for line in path.read_text(errors='replace').splitlines():
        m = re.match(r'\s*Preferred load address is ([0-9a-fA-F]+)', line)
        if m:
            base = int(m.group(1), 16)
            continue
        m = re.match(r'\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{16})\s', line)
        if m and not m.group(1).startswith('__imp_'):
            out.append((int(m.group(2), 16) - base, m.group(1)))
    out.sort()
    return out


def export_symbols(path):
    """(rva, name) sorted, from a PE file's export directory."""
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    nsec = struct.unpack_from('<H', data, pe + 6)[0]
    opt = pe + 24
    opt_size = struct.unpack_from('<H', data, pe + 20)[0]
    exp_rva = struct.unpack_from('<I', data, opt + 112)[0]
    sections = []
    for i in range(nsec):
        s = opt + opt_size + i * 40
        vsize, va, rsize, raw = struct.unpack_from('<IIII', data, s + 8)
        sections.append((va, max(vsize, rsize), raw))

    def off(rva):
        for va, size, raw in sections:
            if va <= rva < va + size:
                return rva - va + raw
        raise ValueError(hex(rva))

    if not exp_rva:
        return []
    e = off(exp_rva)
    nfuncs, nnames, funcs, names, ords = struct.unpack_from('<IIIII', data, e + 20)
    out = []
    for i in range(nnames):
        name_rva = struct.unpack_from('<I', data, off(names) + i * 4)[0]
        ordinal = struct.unpack_from('<H', data, off(ords) + i * 2)[0]
        rva = struct.unpack_from('<I', data, off(funcs) + ordinal * 4)[0]
        n = data[off(name_rva):data.index(b'\0', off(name_rva))].decode(errors='replace')
        out.append((rva, n))
    out.sort()
    return out


def nearest(table, rva):
    i = bisect.bisect_right([r for r, _ in table], rva) - 1
    if i < 0:
        return None
    return f'{table[i][1]}+0x{rva - table[i][0]:x}'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log', nargs='?')
    ap.add_argument('--pasta', default=str(BUILD))
    a = ap.parse_args()
    folder = Path(a.pasta)
    log = Path(a.log) if a.log else folder / 'falha.log'
    tables = {}
    for line in log.read_text(errors='replace').splitlines():
        m = re.match(r'(\s*\d+ )(\S+)\+0x([0-9a-f]+)$', line)
        if not m:
            print(line)
            continue
        module, rva = m.group(2), int(m.group(3), 16)
        if module not in tables:
            f = folder / module
            if module.lower() == f'{NAME}.exe' and (folder / f'{NAME}.map').exists():
                tables[module] = map_symbols(folder / f'{NAME}.map')
            elif f.exists():
                tables[module] = export_symbols(f)
            else:
                tables[module] = []
        name = nearest(tables[module], rva) if tables[module] else None
        print(f'{m.group(1)}{module}+0x{rva:x}' + (f'  {name}' if name else ''))


if __name__ == '__main__':
    main()
