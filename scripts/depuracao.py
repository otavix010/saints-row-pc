"""Collects what processes write with OutputDebugString (the Direct3D 12 validation layer's messages), the way
DebugView does: the DBWIN_BUFFER shared memory and its two events. Runs until stopped or for some seconds.

    py -3 scripts/depuracao.py [--segundos 300] [--saida depuracao.log] [--pid 1234]"""
import argparse
import ctypes
import ctypes.wintypes as wt
import time
from pathlib import Path

k32 = ctypes.windll.kernel32
k32.CreateEventW.restype = wt.HANDLE
k32.CreateFileMappingW.restype = wt.HANDLE
k32.MapViewOfFile.restype = ctypes.c_void_p
k32.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--segundos', type=float, default=300)
    ap.add_argument('--saida', default='depuracao.log')
    ap.add_argument('--pid', type=int, default=0)
    a = ap.parse_args()
    buffer_ready = k32.CreateEventW(None, False, False, 'DBWIN_BUFFER_READY')
    data_ready = k32.CreateEventW(None, False, False, 'DBWIN_DATA_READY')
    mapping = k32.CreateFileMappingW(wt.HANDLE(-1), None, 4, 0, 4096, 'DBWIN_BUFFER')  # PAGE_READWRITE
    view = k32.MapViewOfFile(mapping, 4, 0, 0, 4096)  # FILE_MAP_READ
    if not (buffer_ready and data_ready and mapping and view):
        raise SystemExit('não deu para abrir o DBWIN (outro DebugView aberto?)')
    out = open(Path(a.saida), 'a', encoding='utf-8')
    end = time.time() + a.segundos
    count = 0
    k32.SetEvent(buffer_ready)
    while time.time() < end:
        if k32.WaitForSingleObject(data_ready, 500) != 0:
            continue
        pid = ctypes.c_uint32.from_address(view).value
        text = ctypes.string_at(view + 4).decode('utf-8', errors='replace').rstrip()
        k32.SetEvent(buffer_ready)
        if a.pid and pid != a.pid:
            continue
        out.write(f'{time.strftime("%H:%M:%S")} [{pid}] {text}\n')
        out.flush()
        count += 1
    print(f'{count} mensagens em {a.saida}')


if __name__ == '__main__':
    main()
