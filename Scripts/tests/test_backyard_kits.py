import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import backyard_layout as L  # noqa: E402

THIRDPARTY = L.REPO / "Content" / "ThirdParty"


def listing():
    return {kit: [p.stem for p in (THIRDPARTY / kit).rglob("*.uasset")] for kit in L.KITS}


class KitResolveTests(unittest.TestCase):
    def test_every_layout_mesh_is_in_thirdparty(self):
        found, misses = L.resolve_all(listing())
        report = "\n".join(f"{k}/{m} -> closest {c}" for k, m, c in misses)
        self.assertEqual(misses, [], f"layout meshes missing from Content/ThirdParty:\n{report}")
        self.assertEqual(len(found), sum(len(v) for v in L.meshes_by_kit().values()))

    def test_dash_and_case_match(self):
        self.assertEqual(L.resolve("building-type-a", ["Building_Type_A", "fence"]), "Building_Type_A")

    def test_miss_names_closest(self):
        _, misses = L.resolve_all({"KayKitPrototypeBits": ["Coin_B", "Zzz"]})
        self.assertIn(("KayKitPrototypeBits", "Coin_A", "Coin_B"), misses)
        self.assertIn(("KenneyNature", L.meshes_by_kit()["KenneyNature"][0], None), misses)

    def test_miss_report_lists_each_miss(self):
        self.assertEqual(L.miss_report([("K", "m", "placeholder cube")]), "K/m: placeholder cube\n")
        self.assertEqual(L.miss_report([]), "no misses\n")
