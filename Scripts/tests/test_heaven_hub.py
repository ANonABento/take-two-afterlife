"""Demo heaven hub: finish and death share the couch, the gate starts the next round."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class HeavenHubTest(unittest.TestCase):
    def test_finish_sits_and_the_gate_continues(self):
        gate = read("Source/TakeTwoAfterlife/Private/TTAFinishGate.cpp")
        mode = read("Source/TakeTwoAfterlife/Private/TTAGameMode.cpp")
        self.assertIn("SeatFinished(Runner)", gate)
        self.assertIn("ContinueFromHeaven(Runner)", gate)
        self.assertIn("SpawnContinueGate()", mode)
        self.assertIn("ApplyDemoBotBudgets()", mode)
        self.assertIn("DemoSettingsJson", mode)
        self.assertNotIn("returning to Title", mode)

    def test_owner_budget_is_opt_in(self):
        header = read("Core/include/tta/Placement.h")
        self.assertIn("int budgetFor(int owner) const;", header)
        self.assertIn("return budgetFor(owner) - spent(owner);", header)
