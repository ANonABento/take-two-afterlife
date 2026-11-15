import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import backyard_layout as L  # noqa: E402

PLAYER_HALF = 0.5  # PlayerStart capsule radius (m)


def _overlap(a, b):
    """a, b: (min_x, max_x, min_y, max_y)."""
    return a[0] < b[1] and b[0] < a[1] and a[2] < b[3] and b[2] < a[3]


def _box(p):
    hx, hy = L.footprint(p)
    return (p.x - hx, p.x + hx, p.y - hy, p.y + hy)


class DressingClearance(unittest.TestCase):
    def setUp(self):
        self.dressing = [p for p in L.props() if p.role == "dressing"]
        self.corridor = (0.0, L.CORRIDOR_LENGTH, -L.HALF_WIDTH - L.DITCH, L.HALF_WIDTH + L.DITCH)
        gx, gy, _ = L.GATE_TRIGGER_HALF
        self.protected = {
            "PlayerStart": (L.START.x - PLAYER_HALF, L.START.x + PLAYER_HALF,
                            L.START.y - PLAYER_HALF, L.START.y + PLAYER_HALF),
            "TTAFinishGate": (L.GATE.x - gx, L.GATE.x + gx, L.GATE.y - gy, L.GATE.y + gy),
        }

    def test_no_dressing_in_route_corridor(self):
        for p in self.dressing:
            self.assertFalse(_overlap(_box(p), self.corridor), p)

    def test_no_dressing_on_protected_actors(self):
        for p in self.dressing:
            for name, box in self.protected.items():
                self.assertFalse(_overlap(_box(p), box), (name, p))

    def test_dressing_stays_above_kill_plane(self):
        for p in self.dressing:
            self.assertGreater(p.z, L.KILL_PLANE.max_z)

    def test_two_shortcut_branches_between_markers(self):
        markers = {p.x for p in L.props() if p.role == "marker"}
        stones = [p for p in L.props() if p.role == "shortcut"]
        self.assertEqual(len(L.SHORTCUTS), 2)
        for x0, x1, side in L.SHORTCUTS:
            self.assertIn(x0, markers)
            self.assertIn(x1, markers)
            branch = [p for p in stones if x0 < p.x < x1]
            self.assertTrue(branch and all(p.y * side > 0 and L.in_corridor(p.x, p.y) for p in branch))
        for p in stones:
            for name, box in self.protected.items():
                self.assertFalse(_overlap(_box(p), box), (name, p))

    def test_overlap_check_catches_corridor_intrusion(self):
        self.assertTrue(_overlap((29.0, 31.0, 5.0, 7.0), self.corridor))


if __name__ == "__main__":
    unittest.main()
