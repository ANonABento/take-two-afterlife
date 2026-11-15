"""Fill Content/Audio/{Music,SFX} with the real music loops and SFX as 44.1 kHz 16-bit mono WAVs.

Music: CC0 loops downloaded from OpenGameArt (credited in CREDITS.md), cut to MUSIC_MAX_S so each
file stays under 3 MB, loudness-normalised to MUSIC_LUFS. SFX: the in-house CC0 synth pack from
make_sfx.py, renamed to the cue ids in Core/src/AudioCues.cpp and normalised to SFX_LUFS.
Needs ffmpeg on PATH and network access. Usage: python Scripts/build_audio.py
"""
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
MUSIC_DIR = REPO / "Content" / "Audio" / "Music"
SFX_DIR = REPO / "Content" / "Audio" / "SFX"
RATE = 44100
MUSIC_LUFS = -16
SFX_LUFS = -12
MUSIC_MAX_S = 32

# cue id -> (title, author, OpenGameArt page, file url); all CC0 1.0.
MUSIC = {
    "MUS_Lobby": ("In the Lobby", "Umplix", "https://opengameart.org/content/in-the-lobby",
                  "https://opengameart.org/sites/default/files/in_the_lobby.wav"),
    "MUS_Build": ("Happy Adventure (Loop)", "TinyWorlds", "https://opengameart.org/content/happy-adventure-loop",
                  "https://opengameart.org/sites/default/files/happy_adveture.mp3"),
    "MUS_Run": ("Lasso Lady (seamless loop)", "congusbongus", "https://opengameart.org/content/lasso-lady-seamless-loop",
                "https://opengameart.org/sites/default/files/lassolady_4.ogg"),
    "MUS_Edit": ("Menu Music", "mrpoly", "https://opengameart.org/content/menu-music",
                 "https://opengameart.org/sites/default/files/awesomeness.wav"),
    "MUS_Premiere": ("Heavenly Loop", "isaiah658", "https://opengameart.org/content/heavenly-loop",
                     "https://opengameart.org/sites/default/files/Heavenly%20Loop.flac"),
    "MUS_Podium": ("Funky Victory Loop", "Archonic", "https://opengameart.org/content/funky-victory-loop",
                   "https://opengameart.org/sites/default/files/funky-victory-loop_0.mp3"),
}

# make_sfx.py file stem -> cue id
SFX = {
    "trap_place": "SFX_TrapPlace", "countdown_beep": "SFX_CountdownBeep", "clock_tick": "SFX_ClockTick",
    "bonk": "SFX_ImpactBonk", "thunder": "SFX_Thunder", "gate_chime": "SFX_GateChime",
    "projector_start": "SFX_ProjectorStart", "rating_ding": "SFX_RatingDing", "applause": "SFX_Applause",
    "fanfare": "SFX_Fanfare",
}


def measured_lufs(path, max_s=None):
    """Integrated loudness via ebur128; short clips are looped to 4 s so the 3 s window can measure them."""
    cmd = ["ffmpeg", "-nostats", "-stream_loop", "-1" if not max_s else "0", "-i", str(path),
           "-t", str(max_s or 4), "-af", "ebur128", "-f", "null", "-"]
    out = subprocess.run(cmd, check=True, capture_output=True, text=True).stderr
    return float(out.rsplit("I:", 1)[1].split("LUFS")[0])


def to_wav(src, dst, lufs, max_s=None):
    gain = lufs - measured_lufs(src, max_s)
    cmd = ["ffmpeg", "-y", "-loglevel", "error", "-i", str(src)]
    if max_s:
        cmd += ["-t", str(max_s)]
    cmd += ["-af", f"volume={gain:.2f}dB,alimiter=limit=0.84:level=false", "-ac", "1", "-ar", str(RATE),
            "-c:a", "pcm_s16le", str(dst)]
    subprocess.run(cmd, check=True)


def main():
    if not shutil.which("ffmpeg"):
        sys.exit("build_audio: ffmpeg not found on PATH")
    MUSIC_DIR.mkdir(parents=True, exist_ok=True)
    SFX_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        for cue, (_, _, _, url) in MUSIC.items():
            src = tmp / (cue + Path(url).suffix)
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            src.write_bytes(urllib.request.urlopen(req, timeout=60).read())
            to_wav(src, MUSIC_DIR / f"{cue}.wav", MUSIC_LUFS, MUSIC_MAX_S)
            print(f"music {cue}")
        subprocess.run([sys.executable, str(Path(__file__).with_name("make_sfx.py")), str(tmp)], check=True)
        for stem, cue in SFX.items():
            to_wav(tmp / f"{stem}.wav", SFX_DIR / f"{cue}.wav", SFX_LUFS)
            print(f"sfx {cue}")


if __name__ == "__main__":
    main()
