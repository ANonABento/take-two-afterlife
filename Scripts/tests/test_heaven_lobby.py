import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class HeavenLobbyTest(unittest.TestCase):
    def test_lobby_map_is_heaven(self):
        self.assertIn('LobbyMap = TEXT("/Game/Maps/Heaven")', read("Source/TakeTwoAfterlife/Public/TTASessionSubsystem.h"))

    def test_heaven_has_a_gate_trigger(self):
        self.assertIn("unreal.TTAFinishGate", read("Scripts/dress_heaven.py"))

    def test_gate_feeds_game_mode_and_replicates(self):
        self.assertIn("MarkReady(Runner->GetController())", read("Source/TakeTwoAfterlife/Private/TTAFinishGate.cpp"))
        state = read("Source/TakeTwoAfterlife/Private/TTAGameState.cpp")
        for prop in ("bGateOpen", "ReadyCount", "ReadyMax"):
            self.assertIn(f"DOREPLIFETIME(ATTAGameState, {prop})", state)

    def test_solo_opens_gate_and_lobby_travels_to_build(self):
        mode = read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp")
        self.assertIn("if (BotCount > 0) OpenGate();", mode)
        self.assertIn("Gate.shouldTravel()", mode)
        self.assertIn("RequestPhase(ETTAPhase::Build)", mode)


if __name__ == "__main__":
    unittest.main()
