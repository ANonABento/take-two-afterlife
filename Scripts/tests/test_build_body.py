"""Build mode takes the fly camera and used to leave the runner standing at the start."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


def read(rel):
    return (SRC / rel).read_text(encoding="utf-8-sig")


class BuildBodyFollowsCamera(unittest.TestCase):
    def test_enter_build_carries_the_runner(self):
        mode = read("Private/TTAGameMode.cpp")
        enter = mode[mode.index("void ATTAGameMode::EnterBuild("):]
        self.assertIn("Cast<ATTARunnerPawn>(PC->GetPawn())", enter)
        self.assertLess(enter.index("CarryBody(Body)"), enter.index("PC->Possess(Pawn)"))

    def test_body_flies_beside_the_camera_and_is_not_aimed_at(self):
        pawn = read("Private/TTABuildPawn.cpp")
        self.assertIn("Body->SitOut()", pawn)
        self.assertIn("OnRep_Body", pawn)
        self.assertIn("SetActorEnableCollision(false)", pawn)
        self.assertIn("if (IsLocallyControlled()) FollowBody();", pawn)
        self.assertIn("Params.AddIgnoredActor(Body)", pawn)
        self.assertIn("Body->Destroy()", pawn)


if __name__ == "__main__":
    unittest.main()
