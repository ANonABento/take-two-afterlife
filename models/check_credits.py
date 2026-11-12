"""Fail if CREDITS.md is incomplete: placeholders left, or any audio cue / kit / font without a row.
    python models/check_credits.py        (engine-free; add to `make test`)
Fill models/audio_credits.csv (one row per cue id) then run with --write to regenerate the per-sound table.
"""
import csv
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CREDITS = REPO / "CREDITS.md"
CSV = Path(__file__).with_name("audio_credits.csv")
BEGIN, END = "<!-- audio-rows:begin -->", "<!-- audio-rows:end -->"
PLACEHOLDER = re.compile(r"fill in|TODO|TBD|check terms", re.I)


def cue_ids():
    text = (REPO / "Core" / "src" / "AudioCues.cpp").read_text(encoding="utf-8")
    return re.findall(r'\{"((?:VO|MUS|SFX)_\w+)", AudioKind::\w+', text)


def rows():
    with CSV.open(encoding="utf-8") as f:
        return list(csv.DictReader(f))


def table(rs):
    out = [BEGIN, "| Cue | Title / sound | Author | License | Source |", "|---|---|---|---|---|"]
    out += [f"| `{r['id']}` | {r['title']} | {r['author']} | {r['license']} | {r['source']} |" for r in rs]
    return "\n".join(out + [END])


def write():
    text = CREDITS.read_text(encoding="utf-8")
    block = table(rows())
    text = re.sub(re.escape(BEGIN) + ".*?" + re.escape(END), block, text, flags=re.S) if BEGIN in text \
        else text.replace("\n## Tools", "\n### Per-sound credits\n" + block + "\n\n## Tools")
    CREDITS.write_text(text, encoding="utf-8")


def problems():
    text = CREDITS.read_text(encoding="utf-8")
    p = [f"placeholder: {line.strip()[:80]}" for line in text.splitlines() if PLACEHOLDER.search(line)]
    by_id = {r["id"]: r for r in rows()}
    for cid in cue_ids():
        r = by_id.get(cid)
        if not r or not all(r[k].strip() for k in ("title", "author", "license", "source")):
            p.append(f"audio row incomplete: {cid}")
        elif f"`{cid}`" not in text:
            p.append(f"not in CREDITS.md (run --write): {cid}")
    for need in ("KayKit Prototype Bits", "City Kit (Suburban)", "Nature Kit", "Nunito"):
        if need not in text:
            p.append(f"no credits row: {need}")
    return p


if __name__ == "__main__":
    if "--write" in sys.argv:
        write()
    bad = problems()
    print("\n".join(bad) + f"\n{len(bad)} credit problems")
    sys.exit(1 if bad else 0)
