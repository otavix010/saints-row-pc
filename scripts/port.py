"""What the scripts share: the project's paths (its name from <name>_manifest.toml), the environment of a test run
(never fullscreen; the disc image named in iso.txt, when there is one) and the test window on the left monitor."""
import ctypes
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NAME = next(ROOT.glob('*_manifest.toml')).name[:-len('_manifest.toml')]
BUILD = ROOT / 'out' / 'build' / 'win-amd64-release'
EXE = BUILD / f'{NAME}.exe'
GENERATED = ROOT / 'generated' / 'default'
FUNCS = ROOT / 'config' / 'funcoes.toml'
REXGLUE = ROOT.parent / 'rexglue-sdk' / 'win-amd64' / 'bin' / 'rexglue.exe'
ISO_FILE = ROOT / 'iso.txt'


def kill_from(folder, exe):
    """ends the copies of exe that run from inside folder, and only those: never by name alone, which would also end
    the player's own copy of the game (C:\\OtavixPorts) running at the same time"""
    inside = str(Path(folder).resolve()).lower().rstrip('\\') + '\\'
    out = subprocess.run(['powershell', '-NoProfile', '-Command',
                          f"Get-CimInstance Win32_Process -Filter \"Name='{exe}'\" | "
                          "ForEach-Object { \"$($_.ProcessId)|$($_.ExecutablePath)\" }"],
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        pid, _, path = line.partition('|')
        if pid.strip().isdigit() and path.strip().lower().startswith(inside):
            subprocess.run(['taskkill', '/F', '/PID', pid.strip()], capture_output=True)


def test_env(log, **extra):
    """a run's environment: the log, the game folder, a window (never fullscreen), the prompts answered"""
    env = dict(os.environ, REX_LOG_FILE=str(log), REX_GAME_DATA_ROOT=str(ROOT / 'assets'), REX_FULLSCREEN='false',
               REX_HEADLESS='true')
    if ISO_FILE.exists():
        env['REX_ISO'] = ISO_FILE.read_text(encoding='utf-8').strip()
    env.update({k: str(v) for k, v in extra.items()})
    return env


def _windows(pid):
    user32 = ctypes.windll.user32
    found = []

    @ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def each(hwnd, _):
        owner = ctypes.c_ulong()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True

    user32.EnumWindows(each, 0)
    return found


def move_left(pid, y=300):
    """the whole window on the left monitor (1080x1920 at x=-1080): 1080 wide, 16:9, away from the main one; two fit
    one above the other (y=300 and y=1000)"""
    found = _windows(pid)
    for hwnd in found:
        ctypes.windll.user32.SetWindowPos(hwnd, None, -1080, y, 1080, 640, 0x14)  # NOZORDER | NOACTIVATE
    return bool(found)


def window_state(pid):
    """(x, y, w, h, maximized) of the process's visible windows"""
    out = []
    for hwnd in _windows(pid):
        r = (ctypes.c_long * 4)()
        ctypes.windll.user32.GetWindowRect(hwnd, r)
        out.append((r[0], r[1], r[2] - r[0], r[3] - r[1], bool(ctypes.windll.user32.IsZoomed(hwnd))))
    return out
