import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class RunnerSpawnConfigTest(unittest.TestCase):
    def test_runner_mesh_is_angel(self):
        game = read("Config/DefaultGame.ini")
        m = re.search(r"\[/Script/TakeTwoAfterlife\.TTARunnerPawn\][^\[]*?RunnerMesh=(\S+)", game)
        self.assertIsNotNone(m)
        self.assertEqual(m.group(1).split(".")[0], "/Game/Characters/Angel/SK_Angel")

    def test_three_maps_cooked(self):
        game = read("Config/DefaultGame.ini")
        for name in ("Title", "Backyard", "Heaven"):
            self.assertIn('+MapsToCook=(FilePath="/Game/Maps/%s")' % name, game)

    def test_move_and_jump_mappings(self):
        inp = read("Config/DefaultInput.ini")
        self.assertRegex(inp, r'AxisName="MoveForward",Scale=1\.0+,Key=W\)')
        self.assertRegex(inp, r'AxisName="MoveForward",Scale=-1\.0+,Key=S\)')
        self.assertRegex(inp, r'ActionName="Jump".*Key=SpaceBar\)')


class RunnerSpawnSourceTest(unittest.TestCase):
    def test_game_mode_spawns_runner_by_default(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp")
        self.assertIn("DefaultPawnClass = ATTARunnerPawn::StaticClass();", cpp)
        self.assertIn("void ATTAGameMode::RestartPlayer(", cpp)

    def test_match_controller_restores_game_input(self):
        cpp = read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp")
        self.assertIn("void ATTAPlayerController::BeginPlay()", cpp)
        self.assertIn("FInputModeGameOnly", cpp)
        self.assertIn("bShowMouseCursor = false;", cpp)

    def test_devlog_has_pc_check(self):
        log = read("DEVLOG.md")
        sec = log.split("## Runner spawn check (user PC)", 1)
        self.assertEqual(len(sec), 2)
        steps = re.findall(r"^\d\. ", sec[1].split("\n## ", 1)[0], re.M)
        self.assertEqual(len(steps), 5)


if __name__ == "__main__":
    unittest.main()
