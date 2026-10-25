"""Finds functions the analysis did not see, three ways, and puts them in config/funcoes.toml:
  - pointers in data: every aligned word of the image outside the code that points into the code;
  - addresses the code builds: lis rD,hi then addi/ori with rD a few instructions on (a callback handed over), when
    the address lies outside the function that builds it (inside, it is that function's own label);
  - gaps: code no known function covers (each function's size counted from the generated code, one comment line a
    PowerPC instruction), where a run of instructions ends in a return or a branch: thunks met only at run time.
Each must be a plausible start: the word before is a return (blr, bctr), an unconditional branch or padding, or the
word itself opens a frame (mflr r12, stwu r1).

    py -3 scripts/varrer_ponteiros.py imagem.bin [--gravar]

imagem.bin: the loaded image (PORT_DUMP_IMAGE=file on a run)."""
import bisect
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import FUNCS, GENERATED, NAME  # noqa: E402

PCH = GENERATED / f'{NAME}_pch.h'
REGISTER = GENERATED / f'{NAME}_register.cpp'
BLR, BCTR = 0x4E800020, 0x4E800420


def define(name):
    return int(re.search(rf'#define {name} (0x[0-9A-Fa-f]+)', PCH.read_text()).group(1), 16)


def function_sizes():
    """{start: size in bytes} of the functions in the generated code: walking each in order, a label (loc_X:) puts the
    address at X and a PowerPC instruction (its comment line) adds 4; the end is the furthest reached, so a jump table
    kept in the code after a bctr, which has no instruction lines, still counts with the cases after it"""
    sizes = {}
    # the functions with a name of their own (xstart, __savegprlr_14...) by the registration's address
    named = {n: int(a, 16) for a, n in re.findall(r'SetFunction\(0x([0-9A-Fa-f]{8}), (\w+)\)', REGISTER.read_text())}
    head = re.compile(r'^DEFINE_REX_FUNC\((\w+)\)')
    label = re.compile(r'^loc_([0-9A-F]{8}):')
    for f in sorted(GENERATED.glob(f'{NAME}_recomp.*.cpp')):
        start = cur = furthest = None
        with open(f, encoding='utf-8', errors='replace') as src:
            for line in src:
                if line.startswith('DEFINE_REX_FUNC('):
                    if start is not None:
                        sizes[start] = furthest - start
                    m = head.match(line)
                    name = m.group(1) if m else ''
                    addr = named.get(name)
                    if addr is None and re.fullmatch(r'sub_[0-9A-F]{8}', name):
                        addr = int(name[4:], 16)
                    start = cur = furthest = addr
                elif start is None:
                    continue
                elif line.startswith('loc_'):
                    m = label.match(line)
                    if m:
                        cur = int(m.group(1), 16)
                elif line.startswith('\t// ') and not line.startswith('\t// ERROR'):
                    cur += 4
                    furthest = max(furthest, cur)
        if start is not None:
            sizes[start] = furthest - start
    return sizes


def ends_run(w):
    """blr, bctr or an unconditional branch: where a function (or a thunk) gives control away for good"""
    return w in (BLR, BCTR) or (w >> 26 == 18 and w & 1 == 0)


def main():
    image = Path(sys.argv[1]).read_bytes()
    write = '--gravar' in sys.argv
    base, code, code_size = define('REX_IMAGE_BASE'), define('REX_CODE_BASE'), define('REX_CODE_SIZE')
    end = code + code_size
    known = {int(a, 16) for a in re.findall(r'SetFunction\(0x([0-9A-Fa-f]{8})', REGISTER.read_text())}
    declared = {int(a, 16) for a in re.findall(r'^0x([0-9A-Fa-f]{8})\s*=', FUNCS.read_text(), re.M)}

    def word(addr):
        o = addr - base
        return struct.unpack_from('>I', image, o)[0] if 0 <= o <= len(image) - 4 else None

    def plausible(t):
        before, first = word(t - 4), word(t)
        if first is None or first == 0:
            return False
        if first == 0x7D8802A6 or first >> 16 == 0x9421:  # mflr r12 / stwu r1,-n(r1)
            return True
        # the word before ends a function: blr, bctr, an unconditional branch, or padding
        return before in (BLR, BCTR, 0) or (before is not None and before >> 26 == 18 and before & 3 == 0)

    # every function's body as the generated code covers it: a candidate inside one is that function's own label (an
    # exception table's handler, a computed jump's way back), never a function the analysis missed
    sizes = function_sizes()
    starts = sorted(known | declared)
    body_of = {s: s + n for s, n in sizes.items()}

    def function_of(a):
        i = bisect.bisect_right(starts, a) - 1
        return starts[i] if i >= 0 else None

    def inside_known(t):
        s = function_of(t)
        return s is not None and s != t and t < body_of.get(s, s)

    def candidate(v):
        return (code <= v < end and v % 4 == 0 and v not in known and v not in declared and plausible(v)
                and not inside_known(v))

    # pointers in data
    found = {}
    for o in range(0, len(image) - 3, 4):
        addr = base + o
        if code <= addr < end:
            continue  # the code itself: its own constants are not pointers
        v = struct.unpack_from('>I', image, o)[0]
        if candidate(v):
            found.setdefault(v, addr)

    # addresses the code builds
    built = {}
    for o in range(max(0, code - base), min(len(image), end - base) - 3, 4):
        v = struct.unpack_from('>I', image, o)[0]
        if v >> 26 != 15 or (v >> 16) & 31 != 0:  # lis is addis rD,0,hi
            continue
        rd, hi = (v >> 21) & 31, v & 0xFFFF
        for k in range(1, 9):
            if o + 4 * k > len(image) - 4:
                break
            u = struct.unpack_from('>I', image, o + 4 * k)[0]
            if (u >> 16) & 31 != rd:
                continue
            if u >> 26 == 14:  # addi
                lo = u & 0xFFFF
                t = ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
            elif u >> 26 == 24:  # ori
                t = (hi << 16) | (u & 0xFFFF)
            else:
                continue
            if candidate(t) and t not in found and function_of(t) != function_of(base + o):
                built.setdefault(t, base + o)
            break

    # gaps in the code no function covers. Stricter than above: after a plain branch a gap is more often a switch's
    # case the analysis did not resolve (it jumps on into its function's shared code) than a function, so here a start
    # follows a return or padding, or opens a frame; and a run that ends jumping into the middle of a known function
    # is a piece of that function
    covered = sorted((s, s + n) for s, n in sizes.items() if n)

    def strong_start(t):
        before, first = word(t - 4), word(t)
        if first in (0, None) or base <= first < base + len(image):
            return False  # a word that is an address of the image is data kept among the code (a table, a vtable)
        return first == 0x7D8802A6 or first >> 16 == 0x9421 or before in (BLR, BCTR, 0)

    gaps = {}
    for (s0, e0), (s1, _) in zip(covered, covered[1:]):
        a = e0
        while a < s1:
            w = word(a)
            if w in (0, None):
                a += 4
                continue
            # a run of instructions from a, up to the next end of run, all inside the gap
            b, ok = a, False
            while b < s1 and b - a < 256:
                wb = word(b)
                if wb in (0, None):
                    break
                if ends_run(wb):
                    ok = True
                    break
                b += 4
            if ok:
                last = word(b)
                target = ((b + (last & 0x3FFFFFC) - (0x4000000 if last & 0x2000000 else 0)) & 0xFFFFFFFF
                          if last >> 26 == 18 else None)
                fragment = target is not None and inside_known(target)
                if not fragment and candidate(a) and strong_start(a) and a not in found and a not in built:
                    gaps.setdefault(a, e0)
            a = b + 4 if ok else s1

    print(f'{len(known)} funções conhecidas, {len(declared)} declaradas; candidatas novas: {len(found)} nos dados, '
          f'{len(built)} montadas no código, {len(gaps)} em lacunas')
    for v in sorted(found)[:8]:
        print(f'  0x{v:08X} (ponteiro em 0x{found[v]:08X})')
    for v in sorted(built)[:8]:
        print(f'  0x{v:08X} (montada em 0x{built[v]:08X})')
    for v in sorted(gaps)[:8]:
        print(f'  0x{v:08X} (lacuna depois de 0x{gaps[v]:08X})')
    if write and (found or built or gaps):
        text = FUNCS.read_text(encoding='utf-8').rstrip('\n')
        for title, group in (('pointers in data to plausible function starts', found),
                             ('addresses the code builds (lis + addi/ori) at plausible function starts', built),
                             ('code no function covered, a run ending in a return or a branch', gaps)):
            if group:
                text += f'\n\n# {title} (scripts/varrer_ponteiros.py)\n'
                text += ''.join(f'0x{v:08X} = {{}}\n' for v in sorted(group))
        FUNCS.write_text(text + '\n', encoding='utf-8')
        print('gravadas em', FUNCS)


if __name__ == '__main__':
    main()
