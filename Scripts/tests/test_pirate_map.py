"""Pirate Dungeon map (Req 5-6): five themed sections, >= 2 variants each, seeds 0-99 valid."""
import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from maps.pirate import PIRATE, SECTION_NAMES, build, validate  # noqa: E402
from maplib.grid import budget  # noqa: E402


def _in(box, pt):
    return box.min_x <= pt[0] <= box.max_x and box.min_y <= pt[1] <= box.max_y


class PirateMapTest(unittest.TestCase):
    def test_sections_in_theme_order_with_variants(self):
        self.assertEqual(SECTION_NAMES, ("jail cell", "torch corridor", "sewer", "dock planks", "ship deck"))
        self.assertEqual(len(PIRATE.sections), 5)
        for variants in PIRATE.sections:
            self.assertGreaterEqual(len(variants), 2)

    def test_seeds_0_to_99_validate(self):
        for seed in range(100):
            with self.subTest(seed=seed):
                h = build(seed)
                validate(h)
                first, last = h.layout.placed[0], h.layout.placed[-1]
                self.assertLess(math.dist(h.start[:2], first.entry[:2]), first.segment.length)
                self.assertLess(math.dist(h.gate[:2], last.entry[:2]), last.segment.length)
                self.assertTrue(_in(h.kill_plane, h.start) and _in(h.kill_plane, h.gate))

    def test_same_seed_same_pirate(self):
        self.assertEqual(build(17), build(17))
        self.assertGreater(len({tuple(p.segment.name for p in build(s).layout.placed) for s in range(100)}), 8)

    def test_budget_read_from_core(self):
        self.assertEqual(budget(), 100)

    def test_every_sewer_variant_has_water(self):
        for seg in PIRATE.sections[2]:
            self.assertTrue(seg.kills, seg.name)

    def test_missing_extra_kill_fails(self):
        h = build(0)
        with self.assertRaisesRegex(ValueError, "kill volume"):
            validate(h._replace(kills=[]))


if __name__ == "__main__":
    unittest.main()
