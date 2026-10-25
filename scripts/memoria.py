"""Reads a running port's guest memory (the arena's virtual base is in run.log): finds big-endian floats or words in a
guest range, or shows the words at an address.

    py -3 scripts/memoria.py --floats 0.0083333 0.0166667 120 60 [--de 0x82000000 --ate 0x83000000]
    py -3 scripts/memoria.py --ler 0x82AC4138 [--n 8]"""
import argparse
import ctypes
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pilhas import find_pid, k32  # noqa: E402
from port import BUILD, NAME  # noqa: E402

k32.ReadProcessMemory.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]


def membase():
    text = (BUILD / 'run.log').read_text(errors='replace')
    found = re.findall(r'virtual base 0x([0-9A-Fa-f]+)', text)
    if not found:
        sys.exit('base da memória não encontrada no run.log')
    return int(found[-1], 16)


def read(process, address, size):
    buffer = ctypes.create_string_buffer(size)
    done = ctypes.c_size_t()
    ok = k32.ReadProcessMemory(process, ctypes.c_void_p(address), buffer, size, ctypes.byref(done))
    return buffer.raw[:done.value] if ok or done.value else b''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--floats', nargs='+', type=float)
    ap.add_argument('--de', default='0x82000000')
    ap.add_argument('--ate', default='0x83000000')
    ap.add_argument('--ler')
    ap.add_argument('--n', type=int, default=8)
    a = ap.parse_args()
    pid = find_pid(f'{NAME}.exe')
    if not pid:
        sys.exit(f'{NAME}.exe não está rodando')
    process = k32.OpenProcess(0x0410, False, pid)  # QUERY_INFORMATION | VM_READ
    base = membase()
    if a.ler:
        address = int(a.ler, 16)
        data = read(process, base + address, a.n * 4)
        for i in range(len(data) // 4):
            word = data[i * 4:i * 4 + 4]
            print(f'{address + i * 4:08X}  {struct.unpack(">I", word)[0]:08X}  {struct.unpack(">f", word)[0]:.6g}')
        return
    start, end = int(a.de, 16), int(a.ate, 16)
    chunk = 0x10000
    hits = {v: [] for v in a.floats}
    for address in range(start, end, chunk):
        data = read(process, base + address, chunk)
        if not data:
            continue
        for v in a.floats:
            needle = struct.pack('>f', v)
            for m in re.finditer(re.escape(needle), data):
                if m.start() % 4 == 0:
                    hits[v].append(address + m.start())
    for v, places in hits.items():
        print(f'{v:g}: {len(places)} lugares ' + ' '.join(f'{p:08X}' for p in places[:24]))


if __name__ == '__main__':
    main()
