"""Samples a running port's thread stacks for some seconds (the main thread by default) and counts the game's
functions: how often each is on the stack (inclusive) and on top of the game's part (exclusive), and the usual
chains. Samples spent only in the runtime or the OS (waiting for the frame) are left out of the percentages.

    py -3 scripts/amostrar.py [--segundos 10] [--thread "Main XThread"] [--profundidade 8] [--contem sub_X]"""
import argparse
import sys
import time
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pilhas as P  # noqa: E402
from port import NAME  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--segundos', type=float, default=10)
    ap.add_argument('--thread', default='Main XThread')
    ap.add_argument('--profundidade', type=int, default=8)
    ap.add_argument('--contem', help='only the samples with this function on the stack')
    ap.add_argument('--top', type=int, default=40)
    a = ap.parse_args()
    pid = P.find_pid(f'{NAME}.exe')
    process = P.k32.OpenProcess(P.PROCESS_ALL_ACCESS, False, pid)
    P.dbghelp.SymSetOptions(0x2)
    P.dbghelp.SymInitializeW(process, None, True)
    namer = P.Namer(P.modules(process))
    threads = []
    for tid in P.thread_ids(pid):
        h = P.k32.OpenThread(P.THREAD_ALL_ACCESS, False, tid)
        if P.description(h).startswith(a.thread):
            threads.append(h)
        else:
            P.k32.CloseHandle(h)
    inclusive, exclusive, chains = Counter(), Counter(), Counter()
    total = busy = 0
    end = time.time() + a.segundos
    prefix = f'{NAME}.exe!sub_'
    while time.time() < end:
        for h in threads:
            P.k32.SuspendThread(h)
            try:
                frames = P.walk(process, h, 64)
            finally:
                P.k32.ResumeThread(h)
            names = [namer.name(f) for f in frames]
            total += 1
            top_is_game = bool(names) and names[0].startswith(prefix)
            game = [n.split('!')[1].split('+')[0] for n in names if n.startswith(prefix)]
            if a.contem and a.contem not in game:
                continue
            if not top_is_game and not a.contem:
                continue
            busy += 1
            for g in set(game):
                inclusive[g] += 1
            if game:
                exclusive[game[0]] += 1
            chains[' < '.join(game[:a.profundidade])] += 1
        time.sleep(0.003)
    print(f'{total} amostras, {busy} no código do jogo ({100 * busy // max(total, 1)}%)')
    print('inclusivo (na pilha) / exclusivo (no topo), % das amostras no jogo:')
    for g, c in inclusive.most_common(a.top):
        print(f'  {100 * c / max(busy, 1):5.1f}%  {100 * exclusive[g] / max(busy, 1):5.1f}%  {g}')
    print('cadeias:')
    for k, c in chains.most_common(15):
        print(f'  {c:4}  {k}')


if __name__ == '__main__':
    main()
