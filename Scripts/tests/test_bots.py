"""Play Solo wiring that UE can't check in the sandbox: wrappers, route tag, menu, devlog."""
import pathlib
import re
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))

import backyard_layout as L  # noqa: E402


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class BotsTest(unittest.TestCase):
    def test_every_core_source_is_compiled_by_ue(self):
        for src in sorted((ROOT / "Core/src").glob("*.cpp")):
            wrapper = f"Source/TakeTwoAfterlife/Private/CoreLib/Core_{src.stem}.cpp"
            with self.subTest(wrapper=wrapper):
                self.assertIn(f"Core/src/{src.name}", read(wrapper))

    def test_bots_race_the_finish_gate(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTABotController.cpp")
        header = read("Source/TakeTwoAfterlife/Public/TTABotController.h")
        self.assertIn("tta::BotRace", header)
        self.assertIn("StartRun(Gate)", read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp"))
        self.assertNotIn("TTARoute", cpp)
        self.assertIn("Sense.hazards", cpp)
        self.assertIn("Runner->Punch()", cpp)
        markers = [p for p in L.props() if p.role == "marker"]
        self.assertGreater(len(markers), 2)

    def test_play_solo_button_hosts_with_bots(self):
        self.assertIn('TEXT("PLAY SOLO")', read("Source/TakeTwoAfterlife/Private/TTAMenuWidget.cpp"))
        self.assertIn("HostSolo(ATTAGameMode::MaxBots)", read("Source/TakeTwoAfterlife/Private/TTATitleGameMode.cpp"))
        self.assertIn('"?bots=%d"', read("Source/TakeTwoAfterlife/Private/TTASessionSubsystem.cpp"))
        self.assertRegex(read("Source/TakeTwoAfterlife/Public/TTAGameMode.h"), r"MaxBots = 3;")
        self.assertIn('"AIModule"', read("Source/TakeTwoAfterlife/TakeTwoAfterlife.Build.cs"))

    def test_bot_placements_are_visible_during_build(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTAPlacementMarker.cpp")
        self.assertIn("Data.Owner >= ATTABotController::FirstBotId", cpp)
        self.assertIn('#include "TTABotController.h"', cpp)

    def test_devlog_lists_solo_checks(self):
        devlog = read("DEVLOG.md")
        self.assertIn('"TTA.Bots.SoloRound"', read("Source/TakeTwoAfterlife/Private/Tests/TTABotsSoloRoundTest.cpp"))
        self.assertIn("TTA.Bots.SoloRound", devlog)
        check = devlog.split("## Solo round check (user PC)", 1)
        self.assertEqual(len(check), 2, "DEVLOG needs a '## Solo round check (user PC)' section")
        self.assertGreaterEqual(len(re.findall(r"^\d+\. ", check[1], re.M)), 5)


if __name__ == "__main__":
    unittest.main()
