"""Engine-free helpers for create_placeholder_audio.py (tested in Scripts/tests).

The cue list is read from Core/src/AudioCues.cpp, the single source of truth for every sound the
game loads; paths follow tta::assetPath: /Game/Audio/<VO|Music|SFX>/<id>.<id>.
"""
import re
import struct
import wave
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CUES_CPP = REPO / "Core" / "src" / "AudioCues.cpp"
FOLDERS = {"Narrator": "VO", "Music": "Music", "Sfx": "SFX"}
RATE = 22050
SECONDS = 0.25


def cues(src=CUES_CPP):
    """[(id, folder)] for every cue in the Core table, in table order."""
    text = Path(src).read_text(encoding="utf-8")
    return [(cid, FOLDERS[kind]) for cid, kind in re.findall(r'\{"(\w+)",\s*AudioKind::(\w+)', text)]


def asset_path(cid, folder):
    return f"/Game/Audio/{folder}/{cid}.{cid}"


def package_dir(folder):
    return f"/Game/Audio/{folder}"


def missing(cue_list, content_dir=REPO / "Content"):
    """Cues with no real asset under Content/Audio/<folder>/<id>.uasset yet."""
    root = Path(content_dir) / "Audio"
    return [(cid, folder) for cid, folder in cue_list if not (root / folder / f"{cid}.uasset").exists()]


def write_silent_wav(path, seconds=SECONDS):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack("<h", 0) * int(RATE * seconds))
    return path
