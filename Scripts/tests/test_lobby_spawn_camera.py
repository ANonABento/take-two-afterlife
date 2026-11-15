import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class FakeActor:
    def __init__(self, *tags):
        self._tags = list(tags)

    def get_editor_property(self, name):
        assert name == "tags"
        return self._tags


class HandDressedGuardTest(unittest.TestCase):
    def test_marker_tag_on_actor_skips(self):
        import hand_dressed
        self.assertTrue(hand_dressed.is_hand_dressed([FakeActor("TTA_Layout"), FakeActor("TTA_HandDressed")]))

    def test_marker_on_world_settings_skips(self):
        import hand_dressed
        self.assertTrue(hand_dressed.is_hand_dressed([], FakeActor("TTA_HandDressed")))

    def test_no_marker_runs(self):
        import hand_dressed
        self.assertFalse(hand_dressed.is_hand_dressed([FakeActor("TTA_Layout"), FakeActor()], FakeActor()))
        self.assertFalse(hand_dressed.is_hand_dressed([], None))

    def test_dress_script_checks_before_touching_level(self):
        src = read("Scripts/dress_heaven.py")
        guard = src.find("hand_dressed.is_hand_dressed(")
        self.assertGreater(guard, 0)
        self.assertLess(guard, src.find("S.delete_previous("))
        self.assertIn("unreal.log_warning(", src[guard:src.find("S.delete_previous(")])


class RunnerCameraTest(unittest.TestCase):
    def test_spring_arm_collides(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTARunnerPawn.cpp")
        self.assertIn("CreateDefaultSubobject<USpringArmComponent>", cpp)
        self.assertIn("CameraBoom->bDoCollisionTest = true;", cpp)
        self.assertRegex(cpp, r"CameraBoom->ProbeSize = [1-9]")
        self.assertIn("CameraBoom->ProbeChannel = ECC_Camera;", cpp)
        self.assertIn('#include "GameFramework/SpringArmComponent.h"', cpp)


class SafeSpawnSourceTest(unittest.TestCase):
    def test_game_mode_uses_core_pick(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp")
        self.assertIn("tta::pickSpawn(", cpp)
        self.assertIn("RestartPlayerAtTransform(", cpp)
        self.assertIn("OverlapBlockingTestByChannel(", cpp)

    def test_core_wrapper_and_doctest(self):
        self.assertTrue((ROOT / "Source/TakeTwoAfterlife/Private/CoreLib/Core_SpawnPlacement.cpp").exists())
        self.assertIn("tests/SpawnPlacementTests.cpp", read("Core/CMakeLists.txt"))

    def test_devlog_manual_check(self):
        log = read("DEVLOG.md")
        sec = log.split("## Lobby spawn + camera check (user PC)", 1)
        self.assertEqual(len(sec), 2)
        body = sec[1].split("\n## ", 1)[0]
        self.assertIn("Play Solo", body)
        self.assertIn("behind", body)


if __name__ == "__main__":
    unittest.main()
