"""Design docs (Req 7-9): maps.md has every field for every map, lobby.md the load flow, links parse."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_doc_links import links  # noqa: E402
from maps import house, pirate, sky  # noqa: E402

DOCS = Path(__file__).resolve().parents[2] / "docs" / "design"
MAPS = ((house, house.HOUSE), (sky, sky.SKY), (pirate, pirate.PIRATE))
FIELDS = ("**Route:**", "**Start / finish:**", "**Kill zones:**", "**Build zones:**", "**Variants:**",
          "**Asset packs (CC0):**")
PUBLISHERS = (("kenney.nl",), ("kaylousberg", "kaykit"), ("quaternius.com",))  # KayKit hosts on itch and GitHub


def _map_blocks(text):
    """Map name -> its part of maps.md, from its `## Title (`name`)` heading to the next heading."""
    out = {}
    for block in text.split("\n## ")[1:]:
        head = block.splitlines()[0]
        if head.endswith("`)"):
            out[head.rsplit("(`", 1)[1][:-2]] = block
    return out


class MapsDocTest(unittest.TestCase):
    def setUp(self):
        self.text = (DOCS / "maps.md").read_text(encoding="utf-8")
        self.maps = _map_blocks(self.text)

    def test_shared_library_section(self):
        self.assertIn("## Shared library", self.text)

    def test_every_map_has_every_field(self):
        for mod, mapdef in MAPS:
            with self.subTest(map=mapdef.name):
                block = self.maps[mapdef.name]
                for field in FIELDS:
                    self.assertIn(field, block)
                self.assertIn(" → ".join(mod.SECTION_NAMES), block)
                for name, variants in zip(mod.SECTION_NAMES, mapdef.sections):
                    for v in variants:
                        self.assertIn(f"| {name} | `{v.name}` |", block)

    def test_every_map_lists_cc0_packs_from_each_publisher(self):
        for _, mapdef in MAPS:
            with self.subTest(map=mapdef.name):
                urls = links(self.maps[mapdef.name])
                self.assertGreaterEqual(len(urls), 3)
                for names in PUBLISHERS:
                    self.assertTrue(any(n in u.lower() for n in names for u in urls), names)


class LobbyDocTest(unittest.TestCase):
    def test_load_flow(self):
        text = (DOCS / "lobby.md").read_text(encoding="utf-8")
        for phrase in ("podium", "Gate opens", "load screen", "20 s", "seed", "Start", "Late joiners"):
            self.assertIn(phrase, text)


class LinkParseTest(unittest.TestCase):
    def test_markdown_and_bare_links(self):
        self.assertEqual(links("[a](https://a.example/x) and https://b.example/y."),
                         ["https://a.example/x", "https://b.example/y"])


if __name__ == "__main__":
    unittest.main()
