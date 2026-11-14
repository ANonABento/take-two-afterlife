"""Grid and path size, read from Core's Placement.h so traps and maps share one source."""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PLACEMENT_H = REPO / "Core" / "include" / "tta" / "Placement.h"


def _constants():
    text = PLACEMENT_H.read_text(encoding="utf-8")
    return dict(re.findall(r"constexpr double (k\w+Meters) = ([\d.]+);", text))


def _read(name):
    try:
        return float(_constants()[name])
    except KeyError:
        raise ValueError(f"Placement.h has no {name}") from None


def corridor():
    """(length, width) of the path in metres."""
    return _read("kCorridorLengthMeters"), _read("kCorridorWidthMeters")


def budget():
    """Per-player build budget (kBudget)."""
    m = re.search(r"constexpr int kBudget = (\d+);", PLACEMENT_H.read_text(encoding="utf-8"))
    if not m:
        raise ValueError("Placement.h has no kBudget")
    return int(m.group(1))


def grid_meters():
    return _read("kGridMeters")


def in_corridor(x, y, length, half_width):
    return 0.0 <= x <= length and -half_width <= y <= half_width
