"""Trap swap task 6: the lawnmower in UE (build item mesh, Core-driven actor, blade spin)."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


def read(rel):
    return (SRC / rel).read_text(encoding="utf-8-sig")


class LawnmowerUE(unittest.TestCase):
    def test_build_item_mesh_is_not_the_cannon_barrel(self):
        ini = (ROOT / "Config" / "DefaultGame.ini").read_text(encoding="utf-8-sig")
        meshes = re.findall(r"^\+ItemMeshes=(\S+)", ini, re.M)
        self.assertEqual(meshes[5], "/Game/ThirdParty/KayKitPrototypeBits/Pallet_Small.Pallet_Small")
        self.assertNotIn("Barrel_A", ini)

    def test_actor_steps_core_mower_each_tick(self):
        cpp = read("Private/TTATrapActor.cpp")
        self.assertIn("Mower->update(", cpp)
        self.assertIn("if (Mower) UpdateMower(T);", cpp)
        self.assertIn("TryHit(Runners[E.victim])", cpp)
        self.assertIn("SetGenerateOverlapEvents(!bMower)", cpp)
        self.assertIn('#include "tta/RunRecorder.h"', cpp)

    def test_blade_spins_only_while_driving(self):
        cpp = read("Private/TTATrapActor.cpp")
        self.assertIn("if (State != tta::LawnmowerState::Driving) return FTransform::Identity;", cpp)
        self.assertIn("Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision)", cpp)
        self.assertIn("TObjectPtr<UStaticMeshComponent> Blade", read("Public/TTATrapActor.h"))

    def test_automation_test_covers_mower(self):
        self.assertIn("TTA.Traps.Lawnmower", read("Private/Tests/TTATrapsTest.cpp"))


if __name__ == "__main__":
    unittest.main()
