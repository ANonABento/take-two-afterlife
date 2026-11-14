"""Req 2-4 on a 3-segment test map: chaining, turns and steps, length range, seeded determinism."""
import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from maplib import Box, MapDef, Pose, Segment, check_layout, check_segment, generate, place  # noqa: E402

LAWN = (Segment("lawn_a", 20.0, 0.0, 0.0, (), ((2.0, 0.0), (4.5, -1.5))),
        Segment("lawn_b", 22.0, 15.0, 0.0, (), ((3.0, 1.0),)))
SHED = (Segment("shed_climb", 20.0, 30.0, 1.5, (Box(8.0, 12.0, -4.0, 4.0, -3.0, -1.0),), ((10.0, 2.5),)),
        Segment("shed_drop", 18.0, -45.0, -3.0, (), ((6.0, -2.0),)))
TUB = (Segment("tub_a", 20.0, 0.0, 0.0, (Box(5.0, 15.0, -4.0, 0.0, -2.0, -0.5),), ((1.0, 0.0),)),
       Segment("tub_b", 19.0, 90.0, 0.5, (), ((2.0, 3.5),)))
TEST_MAP = MapDef("test", (LAWN, SHED, TUB))


def _rechain(layout, entry_change):
    """Layout with section 2 placed at section 1's exit changed by entry_change."""
    first, second, third = layout.placed
    moved = place(second.segment, first.exit._replace(**entry_change), second.mirrored)
    return layout._replace(placed=[first, moved, third])


class SegmentChainTest(unittest.TestCase):
    def test_same_seed_same_output(self):
        for seed in (0, 7, 42):
            self.assertEqual(generate(TEST_MAP, seed), generate(TEST_MAP, seed))

    def test_seeds_vary_the_layout(self):
        picks = {tuple((p.segment.name, p.mirrored) for p in generate(TEST_MAP, s).placed) for s in range(100)}
        self.assertGreater(len(picks), 4)

    def test_seeds_0_to_99_chain_and_pass(self):
        for seed in range(100):
            layout = generate(TEST_MAP, seed)
            self.assertEqual(len(layout.placed), 3)
            check_layout(layout)
            for prev, nxt in zip(layout.placed, layout.placed[1:]):
                self.assertEqual(prev.exit, nxt.entry)

    def test_turn_and_step_carry_to_world(self):
        a = place(SHED[0], Pose(0.0, 0.0, 0.0, 0.0))
        self.assertEqual((a.exit.x, a.exit.z, a.exit.heading), (20.0, 1.5, 30.0))
        b = place(TUB[0], a.exit)
        self.assertAlmostEqual(b.exit.x, 20 + 20 * math.cos(math.radians(30)))
        self.assertAlmostEqual(b.exit.y, 20 * math.sin(math.radians(30)))
        self.assertAlmostEqual(b.cells[0][0], 20 + math.cos(math.radians(30)))
        self.assertEqual(b.kills[0].min_z, -0.5)
        m = place(SHED[0], Pose(0.0, 0.0, 0.0, 0.0), mirrored=True)
        self.assertEqual(m.exit.heading, 330.0)
        self.assertEqual(m.cells[0][1], -2.5)

    def test_gap_in_chain_fails(self):
        layout = generate(TEST_MAP, 3)
        with self.assertRaisesRegex(ValueError, "does not start where"):
            check_layout(_rechain(layout, {"x": layout.placed[0].exit.x + 0.5}))

    def test_misaligned_heading_fails(self):
        layout = generate(TEST_MAP, 3)
        with self.assertRaisesRegex(ValueError, "does not start where"):
            check_layout(_rechain(layout, {"heading": layout.placed[0].exit.heading + 1.0}))

    def test_tolerance_allows_under_1cm(self):
        layout = generate(TEST_MAP, 3)
        check_layout(_rechain(layout, {"x": layout.placed[0].exit.x + 0.005}))

    def test_bad_segments_fail(self):
        cases = {"multiple of 15": LAWN[0]._replace(turn=10.0),
                 "step up": LAWN[0]._replace(rise=1.6),
                 "off the path": LAWN[0]._replace(cells=((2.0, 4.5),)),
                 "grid": LAWN[0]._replace(cells=((2.2, 0.0),))}
        for msg, seg in cases.items():
            with self.assertRaisesRegex(ValueError, msg):
                check_segment(seg)

    def test_length_out_of_range_fails(self):
        short = MapDef("short", ((LAWN[0],), (LAWN[0],)))
        with self.assertRaisesRegex(ValueError, "path length"):
            check_layout(generate(short, 0))


if __name__ == "__main__":
    unittest.main()
