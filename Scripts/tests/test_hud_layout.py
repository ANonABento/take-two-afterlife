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
        body = self.hud[self.hud.index("float ATTABuildHUD::DrawBuildTiles("):self.hud.index("UTexture* ATTABuildHUD::TrapThumbnail")]
        self.assertIn("DrawCard(TileX, Y, kTileSize, kTileSize,", body)
        self.assertIn("TTABuildItem::Count()", body)
        self.assertIn("TrapThumbnail(Item)", body)
        self.assertNotIn("IsCamera(Item)", body)
        self.assertLess(self.hud.index("DrawBuildTiles(Left, Y"), self.hud.index("TArray<TPair<FString, int32>> Lines;"))

    def test_thumbnails_are_framed_to_the_tile(self):
        shot = self.hud[self.hud.index("UTexture* ATTABuildHUD::TrapThumbnail"):]
        self.assertIn("ECameraProjectionMode::Orthographic", shot)
        self.assertIn("OrthoWidth = Span", shot)
        self.assertIn("ADirectionalLight", shot)
        self.assertIn("SetCastShadows(false)", shot)
        self.assertIn("SetEyeAdaptation(false)", shot)
        self.assertIn("SetPostProcessMaterial(false)", shot)

    def test_number_keys_cover_lawnmower_and_camera(self):
        pawn = (SRC / "Private" / "TTABuildPawn.cpp").read_text(encoding="utf-8")
        ini = (ROOT / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        for n, key in ((6, "Six"), (7, "Seven")):
            self.assertIn(f'BindAction("BuildItem{n}"', pawn)
            self.assertIn(f'ActionName="BuildItem{n}"', ini)
            self.assertIn(f"Key={key}", ini)

    def test_results_and_podium_use_banner(self):
        for name in ("TTAResultsWidget.cpp", "TTAPodiumWidget.cpp"):
            self.assertIn("UTTAUIStyle::MakeBanner(", (SRC / "Private" / name).read_text(), name)


if __name__ == "__main__":
    unittest.main()
