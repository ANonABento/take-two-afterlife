"""Bots build on screen: an angel each round, and each piece appears when the angel reaches it."""
import pathlib
import unittest

SRC = pathlib.Path(__file__).resolve().parents[2] / "Source" / "TakeTwoAfterlife"


def body(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start)]


class BotBuildShowTest(unittest.TestCase):
    def setUp(self):
        self.mode = (SRC / "Private" / "TTAGameMode.cpp").read_text(encoding="utf-8-sig")
        self.bot = (SRC / "Private" / "TTABotController.cpp").read_text(encoding="utf-8-sig")

    def test_every_build_gives_bots_a_body(self):
        build = body(self.mode, "void ATTAGameMode::EnterBuild()")
        spawn = build.index("SpawnActor<ATTARunnerPawn>(RunnerPawnClass")
        self.assertLess(build.index("if (!Cast<ATTARunnerPawn>(It->GetPawn()))"), spawn)
        self.assertLess(spawn, build.index("It->BuildTraps(Layout);"))

    def test_bot_pieces_start_hidden_and_show_on_arrival(self):
        build = body(self.mode, "void ATTAGameMode::EnterBuild()")
        self.assertIn("P.owner < ATTABotController::FirstBotId || P.round != Layout.round()) continue;", build)
        self.assertIn("Marker->SetActorHiddenInGame(true);", build)
        walk = body(self.bot, "void ATTABotController::WalkBuildStops(float DeltaSeconds)")
        self.assertIn("Mode->ShowBotPlacement(BuildStops[BuildStop].Id)", walk)
        self.assertIn("VInterpConstantTo", walk)
        self.assertNotIn("% BuildStops.Num()", walk)
        # Anything not reached by the end of Build is shown when the race starts.
        self.assertIn("ShowBotPlacement(Pair.Key);", body(self.mode, "void ATTAGameMode::RevealPlacements()"))

    def test_tv_captures_skip_the_outline_pass(self):
        for name in ("TTAHeavenWatch.cpp", "TTAEditStage.cpp"):
            text = (SRC / "Private" / name).read_text(encoding="utf-8-sig")
            self.assertIn("ShowFlags.SetPostProcessMaterial(false)", text, name)


if __name__ == "__main__":
    unittest.main()
