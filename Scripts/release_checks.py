"""Engine-free release checks on Config: Title is GameDefaultMap and every match map is in MapsToCook.

Run before cooking (package_win64.bat) and again before zipping (zip_package.py).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TITLE_MAP = "/Game/Maps/Title"
_GAME_MODE_SECTION = "[/Script/TakeTwoAfterlife.TTAGameMode]"


def _section(text, header):
    if header not in text:
        raise ValueError(f"config has no {header} section")
    return text.split(header, 1)[1].split("\n[", 1)[0]


def _package(path):
    """/Game/Maps/Title.Title -> /Game/Maps/Title"""
    return path.split(".", 1)[0]


def default_map(engine_ini_text):
    m = re.search(r"^GameDefaultMap=(\S+)\s*$", engine_ini_text, re.M)
    if not m:
        raise ValueError("DefaultEngine.ini has no GameDefaultMap")
    return _package(m.group(1))


def maps_to_cook(game_ini_text):
    return [_package(p) for p in re.findall(r'^\+MapsToCook=\(FilePath="([^"]+)"\)\s*$', game_ini_text, re.M)]


def match_maps(game_ini_text):
    """Title plus every *Map= the game mode travels to."""
    body = _section(game_ini_text, _GAME_MODE_SECTION)
    travel = [_package(p) for p in re.findall(r"^\w+Map=(\S+)\s*$", body, re.M)]
    if not travel:
        raise ValueError(f"{_GAME_MODE_SECTION} lists no maps")
    return [TITLE_MAP] + travel


def problems(engine_ini_text, game_ini_text):
    out = []
    if default_map(engine_ini_text) != TITLE_MAP:
        out.append(f"GameDefaultMap is {default_map(engine_ini_text)}, must be {TITLE_MAP}")
    cooked = set(maps_to_cook(game_ini_text))
    out += [f"{m} is not in MapsToCook" for m in match_maps(game_ini_text) if m not in cooked]
    return out


def check(root=ROOT):
    """Raise ValueError listing every problem in <root>/Config."""
    cfg = Path(root) / "Config"
    found = problems((cfg / "DefaultEngine.ini").read_text(encoding="utf-8"),
                     (cfg / "DefaultGame.ini").read_text(encoding="utf-8"))
    if found:
        raise ValueError("release check failed: " + "; ".join(found))


def main():
    check()
    print("release checks: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
