"""Check a smoke run for sound without a listener.

python audiocheck.py [RAW] [--wav OUT] [--every SECONDS]

The game writes its final mix (raw s16 stereo 48 kHz, the host mixer's output in
src/sdk/iop_voice.cpp) to the file named by ROSEPAN_AUDIO_DUMP. Run it with
    python build.py --run 120 --env ROSEPAN_AUDIO_DUMP=<repo>/tools/reverse/work/audio/mix.raw
then this prints the loudness per interval (RMS and peak in dBFS), the stretches of sound and
silence, and writes a WAV next to the dump (or --wav OUT) for the user to listen to.
RAW defaults to work/audio/mix.raw. Audio time follows the device clock, so it lines up with
wall time in the run, not with presented frames (the game runs ~31 fps in a smoke run).
"""
import math
import struct
import sys
import wave
from pathlib import Path

import rosepan

RATE = 48000
SILENT_DB = -60.0


def db(x):
    return 20 * math.log10(x / 32768.0) if x > 0 else -120.0


def main(argv):
    raw = rosepan.WORK / 'audio' / 'mix.raw'
    wav_out = None
    every = 1.0
    args = list(argv)
    while args:
        a = args.pop(0)
        if a == '--wav':
            wav_out = Path(args.pop(0))
        elif a == '--every':
            every = float(args.pop(0))
        else:
            raw = Path(a)
    if not raw.exists():
        print(f'no dump at {raw}: run the game with ROSEPAN_AUDIO_DUMP={raw}')
        return 1
    data = raw.read_bytes()
    data = data[:len(data) - len(data) % 4]
    frames = len(data) // 4
    print(f'{raw}: {frames} frames, {frames / RATE:.1f} s')
    if frames == 0:
        return 1

    step = max(1, int(RATE * every))
    rows = []
    for start in range(0, frames, step):
        n = min(step, frames - start)
        chunk = struct.unpack_from(f'<{n * 2}h', data, start * 4)
        sq = sum(s * s for s in chunk)
        rms = math.sqrt(sq / len(chunk))
        peak = max(abs(s) for s in chunk)
        rows.append((start / RATE, db(rms), db(peak)))

    print(' time    rms dB  peak dB')
    for t, r, p in rows:
        bar = '#' * max(0, int((r + 60) / 3)) if r > SILENT_DB else ''
        print(f'{t:6.1f} {r:8.1f} {p:8.1f}  {bar}')

    # Stretches of sound / silence.
    print('stretches:')
    cur = None
    since = 0.0
    for t, r, _ in rows + [(frames / RATE, None, None)]:
        state = None if r is None else (r > SILENT_DB)
        if state != cur:
            if cur is not None:
                print(f'  {since:6.1f} .. {t:6.1f}  {"sound" if cur else "silence"}')
            cur, since = state, t
    loud = sum(1 for _, r, _ in rows if r > SILENT_DB)
    print(f'sound in {loud} of {len(rows)} intervals')

    wav_out = wav_out or raw.with_suffix('.wav')
    with wave.open(str(wav_out), 'wb') as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data)
    print(f'wrote {wav_out}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
