"""Runs the port, and while it stops on a call to a function the analysis missed, declares that function in
config/funcoes.toml, regenerates and rebuilds, then runs again.

    py -3 scripts/iterar.py [--vezes 25] [--segundos 40] [--pad ROTEIRO]

Each run: the window on the left monitor, the log in out/build/win-amd64-release/run.log; --pad drives the controller
(PORT_PAD syntax, pad_script.h) to reach the menus or the game."""
import argparse
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port import BUILD, EXE, FUNCS, NAME, REXGLUE, ROOT, move_left, test_env  # noqa: E402

MISSING = re.compile(r'Call to invalid or unregistered function at guest address 0x([0-9A-Fa-f]{8})')


def run(seconds, pad=''):
    log = BUILD / 'run.log'
    log.unlink(missing_ok=True)
    p = subprocess.Popen([str(EXE)], cwd=BUILD, env=test_env(log, PORT_PAD=pad) if pad else test_env(log))
    t0, moved = time.time(), False
    while time.time() - t0 < seconds and p.poll() is None:
        time.sleep(0.3)
        if not moved:
            moved = move_left(p.pid)
    alive = p.poll() is None
    if alive:
        p.kill()
        p.wait()
    text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
    return alive, p.returncode, text


def declare(addr):
    text = FUNCS.read_text(encoding='utf-8')
    key = f'0x{addr.upper()} = {{}}'
    if key in text:
        return False
    FUNCS.write_text(text.rstrip('\n') + f'\n{key}  # called through a pointer\n', encoding='utf-8')
    return True


def rebuild():
    r = subprocess.run([str(REXGLUE), 'codegen', f'{NAME}_manifest.toml'], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-2000:], r.stderr[-2000:])
        return False
    r = subprocess.run(['cmd', '/c', str(ROOT / 'build.cmd'), 'release'], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-3000:])
        return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--vezes', type=int, default=25)
    ap.add_argument('--segundos', type=float, default=40)
    ap.add_argument('--pad', default='')
    a = ap.parse_args()
    for n in range(1, a.vezes + 1):
        t0 = time.time()
        alive, code, text = run(a.segundos, a.pad)
        m = MISSING.search(text)
        if alive:
            print(f'[{n}] rodou {a.segundos:.0f} s sem parar')
            break
        if not m:
            print(f'[{n}] saiu com {code} sem função faltando; fim do log:')
            print('\n'.join(text.splitlines()[-15:]))
            break
        addr = m.group(1)
        if not declare(addr):
            print(f'[{n}] 0x{addr} já declarada e ainda faltando; parando')
            break
        print(f'[{n}] faltava 0x{addr} (rodou {time.time() - t0:.0f} s); gerando e compilando...', flush=True)
        if not rebuild():
            print('falhou a geração ou a compilação')
            break


if __name__ == '__main__':
    main()
