import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
CORE = ROOT / "Core" / "include" / "tta" / "MenuFlow.h"


class HudLayoutTests(unittest.TestCase):
    """Build HUD / results / podium follow plans/tta-ui-redo/mock-hud.png."""

    def setUp(self):
        self.hud = (SRC / "Private" / "TTABuildHUD.cpp").read_text()

    def test_colour_switches_come_from_core(self):
        self.assertIn("tta::hudTimerFill(SecondsLeft)", self.hud)
        self.assertIn("tta::hudBudgetFill(Own.remaining(Me))", self.hud)
        self.assertNotIn("kTimerWarnSeconds", self.hud, "threshold lives only in Core")
        self.assertRegex(CORE.read_text(), r"constexpr int kTimerWarnSeconds = 10;")

    def test_top_bar_fills(self):
        self.assertRegex(self.hud, r"DrawCard\(Left, Top, [^;]*tta::kPurple\)")
        self.assertRegex(self.hud, r"DrawCard\(TimerX, Top, [^;]*hudTimerFill")

    def test_build_tiles_are_equal_squares(self):
        body = self.hud[self.hud.index("float ATTABuildHUD::DrawBuildTiles("):]
        self.assertIn("DrawCard(TileX, Y, kTileSize, kTileSize,", body)
        self.assertIn("TTABuildItem::Count()", body)
        self.assertLess(self.hud.index("DrawBuildTiles(Left, Y"), self.hud.index("TArray<TPair<FString, int32>> Lines;"))

    def test_results_and_podium_use_banner(self):
        for name in ("TTAResultsWidget.cpp", "TTAPodiumWidget.cpp"):
            self.assertIn("UTTAUIStyle::MakeBanner(", (SRC / "Private" / name).read_text(), name)


if __name__ == "__main__":
    unittest.main()
