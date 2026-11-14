"""Asset packs and the CREDITS.md check."""
import re
from collections import namedtuple

# kit key -> (credits name, author, url, licence). `verified` = mesh names checked against the published pack.
Kit = namedtuple("Kit", "name author url licence verified")


def meshes_by_kit(props):
    used = {}
    for p in props:
        used.setdefault(p.kit, set()).add(p.mesh)
    return {kit: sorted(names) for kit, names in used.items()}


def credits_missing(props, kits, credits_text):
    """(kit, mesh) pairs and kit URLs that CREDITS.md does not list."""
    missing = []
    for kit, names in meshes_by_kit(props).items():
        rows = [line for line in credits_text.splitlines() if kits[kit].url in line]
        if not rows:
            missing.append((kit, kits[kit].url))
            continue
        for name in names:
            if not any(re.search(rf"`{re.escape(name)}`", row) for row in rows):
                missing.append((kit, name))
    return missing
