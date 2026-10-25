"""Measures the sound each port is playing: the peak of its Windows audio session, sampled for some seconds, to tell
without hearing it whether a game has sound (pycaw: py -3 -m pip install --user pycaw).

    py -3 scripts/som.py [--segundos 10]"""
import argparse
import time

from pycaw.pycaw import AudioUtilities, IAudioMeterInformation

GAMES = ('asuraswrath.exe', 'dantesinferno.exe', 'dirt2.exe', 'saintsrow.exe')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--segundos', type=float, default=10)
    a = ap.parse_args()
    peaks = {}
    end = time.time() + a.segundos
    while time.time() < end:
        for session in AudioUtilities.GetAllSessions():
            name = session.Process.name().lower() if session.Process else ''
            if name not in GAMES:
                continue
            meter = session._ctl.QueryInterface(IAudioMeterInformation)
            peak = meter.GetPeakValue()
            values = peaks.setdefault(name, [])
            values.append(peak)
        time.sleep(0.1)
    if not peaks:
        print('nenhum port com sessão de áudio')
    for name, values in peaks.items():
        loud = sum(1 for v in values if v > 0.01)
        print(f'{name}: pico máximo {max(values):.3f}, com som em {100 * loud // len(values)}% das amostras')


if __name__ == '__main__':
    main()
