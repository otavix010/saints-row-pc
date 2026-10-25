"""The stacks of every thread of a running port, to see where a game that stopped advancing waits: each thread is
suspended for a moment, walked with dbghelp (the binaries' unwind data), and its frames named from the linker map
(the game's recompiled functions, sub_XXXXXXXX) and the DLLs' exports. Threads in the same place are grouped.

    py -3 scripts/pilhas.py [--quadros 12] [--todas]"""
import argparse
import ctypes
import ctypes.wintypes as wt
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pilha import export_symbols, map_symbols, nearest  # noqa: E402
from port import BUILD, NAME  # noqa: E402

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
dbghelp = ctypes.WinDLL('dbghelp', use_last_error=True)

PROCESS_ALL_ACCESS = 0x1F0FFF
THREAD_ALL_ACCESS = 0x1F03FF
TH32CS_SNAPPROCESS, TH32CS_SNAPTHREAD = 0x2, 0x4
CONTEXT_FULL = 0x10000B
CONTEXT_SIZE = 1232  # x64 CONTEXT, 16-byte aligned
IMAGE_FILE_MACHINE_AMD64 = 0x8664


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', wt.DWORD), ('cntUsage', wt.DWORD), ('th32ProcessID', wt.DWORD),
                ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', wt.DWORD), ('cntThreads', wt.DWORD),
                ('th32ParentProcessID', wt.DWORD), ('pcPriClassBase', ctypes.c_long), ('dwFlags', wt.DWORD),
                ('szExeFile', ctypes.c_wchar * 260)]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', wt.DWORD), ('cntUsage', wt.DWORD), ('th32ThreadID', wt.DWORD),
                ('th32OwnerProcessID', wt.DWORD), ('tpBasePri', ctypes.c_long), ('tpDeltaPri', ctypes.c_long),
                ('dwFlags', wt.DWORD)]


class ADDRESS64(ctypes.Structure):
    _fields_ = [('Offset', ctypes.c_uint64), ('Segment', wt.WORD), ('Mode', ctypes.c_int)]


class KDHELP64(ctypes.Structure):
    _fields_ = [('Thread', ctypes.c_uint64), ('ThCallbackStack', wt.DWORD), ('ThCallbackBStore', wt.DWORD),
                ('NextCallback', wt.DWORD), ('FramePointer', wt.DWORD), ('KiCallUserMode', ctypes.c_uint64),
                ('KeUserCallbackDispatcher', ctypes.c_uint64), ('SystemRangeStart', ctypes.c_uint64),
                ('KiUserExceptionDispatcher', ctypes.c_uint64), ('StackBase', ctypes.c_uint64),
                ('StackLimit', ctypes.c_uint64), ('BuildVersion', wt.DWORD),
                ('RetpolineStubFunctionTableSize', wt.DWORD), ('RetpolineStubFunctionTable', ctypes.c_uint64),
                ('RetpolineStubOffset', wt.DWORD), ('RetpolineStubSize', wt.DWORD),
                ('Reserved0', ctypes.c_uint64 * 2)]


class STACKFRAME64(ctypes.Structure):
    _fields_ = [('AddrPC', ADDRESS64), ('AddrReturn', ADDRESS64), ('AddrFrame', ADDRESS64),
                ('AddrStack', ADDRESS64), ('AddrBStore', ADDRESS64), ('FuncTableEntry', ctypes.c_void_p),
                ('Params', ctypes.c_uint64 * 4), ('Far', wt.BOOL), ('Virtual', wt.BOOL),
                ('Reserved', ctypes.c_uint64 * 3), ('KdHelp', KDHELP64)]


class MODULEINFO(ctypes.Structure):
    _fields_ = [('lpBaseOfDll', ctypes.c_void_p), ('SizeOfImage', wt.DWORD), ('EntryPoint', ctypes.c_void_p)]


k32.OpenProcess.restype = wt.HANDLE
k32.OpenThread.restype = wt.HANDLE
k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
dbghelp.SymFunctionTableAccess64.restype = ctypes.c_void_p
dbghelp.SymGetModuleBase64.restype = ctypes.c_uint64
dbghelp.StackWalk64.argtypes = [wt.DWORD, wt.HANDLE, wt.HANDLE, ctypes.POINTER(STACKFRAME64), ctypes.c_void_p,
                                ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]


def find_pid(exe):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    e = PROCESSENTRY32W(dwSize=ctypes.sizeof(PROCESSENTRY32W))
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        if e.szExeFile.lower() == exe.lower():
            k32.CloseHandle(snap)
            return e.th32ProcessID
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return None


def thread_ids(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    e = THREADENTRY32(dwSize=ctypes.sizeof(THREADENTRY32))
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def modules(process):
    handles = (ctypes.c_void_p * 1024)()
    needed = wt.DWORD()
    psapi.EnumProcessModulesEx(process, handles, ctypes.sizeof(handles), ctypes.byref(needed), 3)
    out = []
    for h in handles[:needed.value // ctypes.sizeof(ctypes.c_void_p)]:
        name = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(process, ctypes.c_void_p(h), name, 520)
        info = MODULEINFO()
        psapi.GetModuleInformation(process, ctypes.c_void_p(h), ctypes.byref(info), ctypes.sizeof(info))
        out.append((info.lpBaseOfDll, info.SizeOfImage, Path(name.value)))
    return out


class Namer:
    def __init__(self, mods):
        self.mods = mods
        self.tables = {}

    def name(self, address):
        for base, size, path in self.mods:
            if base <= address < base + size:
                rva = address - base
                key = path.name.lower()
                if key not in self.tables:
                    mapfile = path.with_suffix('.map')
                    try:
                        if key == f'{NAME}.exe' and mapfile.exists():
                            self.tables[key] = map_symbols(mapfile)
                        else:
                            self.tables[key] = export_symbols(path)
                    except (OSError, ValueError, IndexError):
                        self.tables[key] = []
                symbol = nearest(self.tables[key], rva) if self.tables[key] else None
                return f'{path.name}!{symbol}' if symbol else f'{path.name}+0x{rva:x}'
        return f'0x{address:x}'


def walk(process, thread, depth):
    raw = ctypes.create_string_buffer(CONTEXT_SIZE + 16)
    address = (ctypes.addressof(raw) + 15) & ~15
    ctypes.c_uint32.from_address(address + 0x30).value = CONTEXT_FULL
    if not k32.GetThreadContext(thread, ctypes.c_void_p(address)):
        return []
    rip = ctypes.c_uint64.from_address(address + 0xF8).value
    rsp = ctypes.c_uint64.from_address(address + 0x98).value
    rbp = ctypes.c_uint64.from_address(address + 0xA0).value
    frame = STACKFRAME64()
    frame.AddrPC.Offset, frame.AddrPC.Mode = rip, 3
    frame.AddrStack.Offset, frame.AddrStack.Mode = rsp, 3
    frame.AddrFrame.Offset, frame.AddrFrame.Mode = rbp, 3
    out = []
    table = ctypes.cast(dbghelp.SymFunctionTableAccess64, ctypes.c_void_p)
    base = ctypes.cast(dbghelp.SymGetModuleBase64, ctypes.c_void_p)
    while len(out) < depth and dbghelp.StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, ctypes.byref(frame),
                                                   ctypes.c_void_p(address), None, table, base, None):
        if not frame.AddrPC.Offset:
            break
        out.append(frame.AddrPC.Offset)
    return out


def description(thread):
    text = ctypes.c_wchar_p()
    try:
        if k32.GetThreadDescription(thread, ctypes.byref(text)) >= 0 and text.value:
            value = text.value
            k32.LocalFree(text)
            return value
    except AttributeError:
        pass
    return ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--quadros', type=int, default=12)
    ap.add_argument('--todas', action='store_true', help='also the threads only waiting in the OS, without game frames')
    a = ap.parse_args()
    pid = find_pid(f'{NAME}.exe')
    if not pid:
        sys.exit(f'{NAME}.exe não está rodando')
    process = k32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
    dbghelp.SymSetOptions(0x2)  # SYMOPT_UNDNAME; the names come from the map and the exports
    dbghelp.SymInitializeW(process, None, True)
    namer = Namer(modules(process))
    groups = defaultdict(list)
    for tid in thread_ids(pid):
        thread = k32.OpenThread(THREAD_ALL_ACCESS, False, tid)
        if not thread:
            continue
        k32.SuspendThread(thread)
        try:
            frames = walk(process, thread, a.quadros)
        finally:
            k32.ResumeThread(thread)
        label = description(thread) or '?'
        k32.CloseHandle(thread)
        names = tuple(namer.name(f) for f in frames)
        if not a.todas and not any(n.lower().startswith((f'{NAME}.exe', 'rex')) for n in names):
            continue
        groups[names].append(f'{tid} {label}')
    dbghelp.SymCleanup(process)
    for names, threads in sorted(groups.items(), key=lambda g: g[1][0]):
        print(f'== {len(threads)} thread(s): ' + ', '.join(threads))
        for n in names:
            print(f'     {n}')


if __name__ == '__main__':
    main()
