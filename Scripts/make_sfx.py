"""Generate the cartoony SFX pack as 16-bit 44.1 kHz mono WAVs in Audio/SFX/.

Pure synthesis (numpy + wave), seeded so every run writes identical files.
Usage: python3 Scripts/make_sfx.py [out_dir]
"""
import sys
import wave
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[1]
OUT_DIR = REPO / "Audio" / "SFX"
RATE = 44100
PEAK_DBFS = -1.0
FADE_S = 0.010
RNG_SEED = 2029


def _t(seconds):
    return np.arange(int(RATE * seconds)) / RATE


def _env(t, decay):
    return np.exp(-t / decay)


def _lowpass(x, width):
    """Moving-average low-pass; width in samples."""
    return np.convolve(x, np.ones(width) / width, mode="same")


def _place(dst, src, start_s):
    i = int(start_s * RATE)
    n = min(len(src), len(dst) - i)
    dst[i:i + n] += src[:n]


def trap_place(rng):
    t = _t(0.25)
    pitch = 180 * np.exp(-t * 8)
    body = np.sin(2 * np.pi * np.cumsum(pitch) / RATE) * _env(t, 0.05)
    knock = _lowpass(rng.standard_normal(len(t)), 8) * _env(t, 0.008)
    return body + 0.6 * knock


def countdown_beep(rng):
    t = _t(0.2)
    return np.sign(np.sin(2 * np.pi * 880 * t)) * 0.5 + np.sin(2 * np.pi * 1760 * t) * 0.3


def clock_tick(rng):
    t = _t(0.08)
    return (np.sin(2 * np.pi * 3200 * t) + 0.5 * rng.standard_normal(len(t))) * _env(t, 0.006)


def bonk(rng):
    t = _t(0.35)
    pitch = 600 * np.exp(-t * 12) + 150
    return np.sin(2 * np.pi * np.cumsum(pitch) / RATE) * _env(t, 0.09)


def thunder(rng):
    t = _t(2.0)
    rumble = _lowpass(_lowpass(rng.standard_normal(len(t)), 120), 120)
    crack = _lowpass(rng.standard_normal(len(t)), 6) * _env(t, 0.05) * 0.2
    swell = np.minimum(t / 0.15, 1) * _env(t, 0.7) * (1 + 0.4 * np.sin(2 * np.pi * 3 * t))
    return rumble * swell + crack


def _bell(freq, seconds):
    t = _t(seconds)
    return (np.sin(2 * np.pi * freq * t) + 0.4 * np.sin(2 * np.pi * freq * 2.76 * t)
            + 0.2 * np.sin(2 * np.pi * freq * 5.4 * t)) * _env(t, 0.35)


def gate_chime(rng):
    out = np.zeros(int(RATE * 1.2))
    for i, f in enumerate((1046.5, 1318.5, 1568.0, 2093.0)):
        _place(out, _bell(f, 1.2), i * 0.12)
    return out


def projector_start(rng):
    t = _t(1.5)
    speed = 8 + 22 * np.minimum(t / 1.0, 1)  # clicks per second, spinning up
    phase = np.cumsum(speed) / RATE
    clicks = np.zeros(len(t))
    for k in np.searchsorted(phase, np.arange(1, int(phase[-1]) + 1)):
        _place(clicks, clock_tick(rng)[:300] * 0.8, k / RATE)
    whir = np.sin(2 * np.pi * np.cumsum(60 + 80 * np.minimum(t, 1)) / RATE) * 0.3
    return clicks + whir * np.minimum(t / 0.3, 1)


def rating_ding(rng):
    return _bell(1760.0, 0.8)


def applause(rng):
    out = np.zeros(int(RATE * 3.0))
    for _ in range(600):
        n = int(RATE * 0.02)
        clap = _lowpass(rng.standard_normal(n), 3) * _env(_t(0.02), 0.004) * rng.uniform(0.3, 1)
        _place(out, clap, rng.uniform(0, 2.98))
    t = _t(3.0)
    return out * np.minimum(t / 0.3, 1) * np.minimum((3.0 - t) / 0.8, 1)


def _brass(freq, seconds):
    t = _t(seconds)
    tone = sum(np.sin(2 * np.pi * freq * h * t) / h for h in range(1, 9))
    return tone * np.minimum(t / 0.03, 1) * np.minimum((seconds - t) / 0.05, 1)


def fanfare(rng):
    out = np.zeros(int(RATE * 2.0))
    for start, f, dur in ((0.0, 392.0, 0.18), (0.2, 523.25, 0.18), (0.4, 659.25, 0.18), (0.6, 783.99, 1.35)):
        _place(out, _brass(f, dur), start)
    return out


SOUNDS = {
    "trap_place": (trap_place, 0.25),
    "countdown_beep": (countdown_beep, 0.2),
    "clock_tick": (clock_tick, 0.08),
    "bonk": (bonk, 0.35),
    "thunder": (thunder, 2.0),
    "gate_chime": (gate_chime, 1.2),
    "projector_start": (projector_start, 1.5),
    "rating_ding": (rating_ding, 0.8),
    "applause": (applause, 3.0),
    "fanfare": (fanfare, 2.0),
}


def finish(x):
    """10 ms fades, then scale to a PEAK_DBFS peak; returns int16 samples."""
    x = np.asarray(x, dtype=np.float64).copy()
    n = int(RATE * FADE_S)
    ramp = np.linspace(0, 1, n)
    x[:n] *= ramp
    x[-n:] *= ramp[::-1]
    peak = np.max(np.abs(x))
    if peak == 0:
        raise ValueError("silent sound")
    x *= 10 ** (PEAK_DBFS / 20) / peak
    return np.round(x * 32767).astype("<i2")


def write_wav(path, samples):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(samples.tobytes())


def main(out_dir=OUT_DIR):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(RNG_SEED)
    written = []
    for name, (make, _) in SOUNDS.items():
        path = out_dir / f"{name}.wav"
        write_wav(path, finish(make(rng)))
        written.append(path)
    return written


if __name__ == "__main__":
    for p in main(*sys.argv[1:]):
        print(p)
