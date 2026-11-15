"""Score board after each vote, podium statues, back to the Heaven lobby."""
import pathlib
import unittest

from test_premiere_end import body

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


class PodiumFlow(unittest.TestCase):
    def setUp(self):
        self.mode = (SRC / "Private" / "TTAGameMode.cpp").read_text(encoding="utf-8-sig")

    def test_podium_runs_the_show_and_ends_in_lobby(self):
        self.assertIn("StartPodium(IsOnMap(HeavenMap))", body(self.mode, "bool ATTAGameMode::RequestPhase("))
        tick = body(self.mode, "void ATTAGameMode::Tick(")
        self.assertIn("PodiumStep::Lobby", tick)
        self.assertIn("RequestPhase(ETTAPhase::Lobby)", tick)

    def test_statues_survive_lobby_and_clear_on_build(self):
        lobby = body(self.mode, "bool ATTAGameMode::ReturnToLobby(")
        self.assertIn("!IsOnMap(Session->GetLobbyMap())", lobby)
        self.assertNotIn("ClearStatues", lobby)
        self.assertIn("statuesStand", body(self.mode, "bool ATTAGameMode::RequestPhase("))

    def test_statues_are_frozen_angels_on_tagged_spots(self):
        spawn = body(self.mode, "void ATTAGameMode::SpawnStatues(")
        self.assertIn("FreezeAsStatue", spawn)
        self.assertIn("ATargetPoint", spawn)
        pawn = (SRC / "Private" / "TTARunnerPawn.cpp").read_text(encoding="utf-8-sig")
        freeze = body(pawn, "void ATTARunnerPawn::FreezeAsStatue(")
        self.assertIn("bPauseAnims = true", freeze)
        self.assertIn("ETTAForm::Angel", freeze)

    def test_cinema_feeds_score_board_on_reveal(self):
        cinema = (SRC / "Private" / "TTACinema.cpp").read_text(encoding="utf-8-sig")
        self.assertIn("tta::scoreBoard(", body(cinema, "void ATTACinema::EnterStep("))
        hud = (SRC / "Private" / "TTABuildHUD.cpp").read_text(encoding="utf-8-sig")
        self.assertIn("tta::scoreBarAt(", body(hud, "void ATTABuildHUD::DrawScoreBoard("))


if __name__ == "__main__":
    unittest.main()
