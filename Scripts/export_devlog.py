"""Check DEVLOG.md has an entry for every task in plan.md and export it as HTML to paste into the itch devlog."""
import argparse
import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TASK_LINE = re.compile(r"^(\d+\w*)\. \[", re.M)
ENTRY = re.compile(r"^## Task (\d+\w*) ", re.M)


def plan_tasks(plan: str) -> list[str]:
    section = plan.split("## Tasks", 1)[1].split("\n## ", 1)[0]
    tasks = TASK_LINE.findall(section)
    if "task **11b" in plan:
        tasks.append("11b")
    return tasks


def missing_entries(plan: str, devlog: str) -> list[str]:
    have = set(ENTRY.findall(devlog))
    return [t for t in plan_tasks(plan) if t not in have]


def inline(text: str) -> str:
    text = html.escape(text, quote=False)
    text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
    return re.sub(r"`(.+?)`", r"<code>\1</code>", text)


def to_html(devlog: str) -> str:
    out, in_list = [], False
    for line in devlog.splitlines():
        if line.startswith("- "):
            if not in_list:
                out.append("<ul>")
                in_list = True
            out.append(f"<li>{inline(line[2:])}</li>")
            continue
        if in_list:
            out.append("</ul>")
            in_list = False
        if line.startswith("## "):
            out.append(f"<h2>{inline(line[3:])}</h2>")
        elif line.startswith("# "):
            out.append(f"<h1>{inline(line[2:])}</h1>")
        elif line.strip():
            out.append(f"<p>{inline(line)}</p>")
    if in_list:
        out.append("</ul>")
    return "\n".join(out) + "\n"


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--out", type=Path, default=ROOT / "Dist" / "devlog_itch.html")
    a = p.parse_args()
    devlog = (ROOT / "DEVLOG.md").read_text(encoding="utf-8")
    missing = missing_entries((ROOT / "plan.md").read_text(encoding="utf-8"), devlog)
    if missing:
        print(f"DEVLOG.md has no entry for task(s): {', '.join(missing)}", file=sys.stderr)
        return 1
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(to_html(devlog), encoding="utf-8")
    print(a.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
