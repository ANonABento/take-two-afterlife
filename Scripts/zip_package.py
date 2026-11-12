"""Zip a packaged Win64 build as <TeamName>_TakeTwoAfterlife.zip and enforce the jam's 1 GB limit."""
import argparse
import re
import sys
import zipfile
from pathlib import Path

GAME = "TakeTwoAfterlife"
MAX_BYTES = 1024 ** 3  # jam rule: the zip must be under 1 GB


def zip_name(team: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9-]+", team):
        raise ValueError(f"team name must be letters, digits or '-', got {team!r}")
    return f"{team}_{GAME}"


def make_zip(build_dir: Path, team: str, out_dir: Path, max_bytes: int = MAX_BYTES) -> Path:
    if not (build_dir / f"{GAME}.exe").is_file():
        raise FileNotFoundError(f"{build_dir} has no {GAME}.exe; package the game first")
    name = zip_name(team)
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / f"{name}.zip"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for f in sorted(build_dir.rglob("*")):
            if f.is_file():
                zf.write(f, Path(name) / f.relative_to(build_dir))
    size = out.stat().st_size
    if size >= max_bytes:
        out.unlink()
        raise ValueError(f"{out.name} is {size} bytes, limit is under {max_bytes}")
    return out


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("build_dir", type=Path, help="packaged Windows folder (contains TakeTwoAfterlife.exe)")
    p.add_argument("team", help="team name, used as <team>_TakeTwoAfterlife.zip")
    p.add_argument("--out", type=Path, default=Path("Dist"))
    a = p.parse_args()
    out = make_zip(a.build_dir, a.team, a.out)
    print(f"{out} ({out.stat().st_size / 1024 ** 2:.1f} MiB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
