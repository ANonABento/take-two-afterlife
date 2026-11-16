"""Punch arm task: bone motion on the right arm replaces the wind-up scale stand-in."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source/TakeTwoAfterlife"
H = (SRC / "Public/TTARunnerPawn.h").read_text(encoding="utf-8-sig")
CPP = (SRC / "Private/TTARunnerPawn.cpp").read_text(encoding="utf-8-sig")
ANIM_H = (SRC / "Public/TTARunnerAnimInstance.h").read_text(encoding="utf-8-sig")

PUNCH_PATH = (
    "void ATTARunnerPawn::ServerPunch_Implementation()",
    "void ATTARunnerPawn::MulticastWindUp_Implementation()",
    "void ATTARunnerPawn::AdvancePunch(",
    "void ATTARunnerPawn::UpdatePunchArm(",
    "void ATTARunnerPawn::MulticastPunched_Implementation",
    "void ATTARunnerPawn::Tick(",
)


def body(signature):
    return CPP.split(signature, 1)[1].split("\n}\n", 1)[0]


class PunchArmTest(unittest.TestCase):
    def test_punch_path_has_no_scale(self):
        for sig in PUNCH_PATH:
            self.assertNotIn("SetRelativeScale3D", body(sig), sig)

    def test_scale_stand_in_removed(self):
        for name in ("WindUpScale", "WindUpLeft", "UpdateWindUpPose"):
            self.assertNotIn(name, H)
            self.assertNotIn(name, CPP)

    def test_anim_instance_properties(self):
        self.assertIn("class TAKETWOAFTERLIFE_API UTTARunnerAnimInstance : public UAnimInstance", ANIM_H)
        for prop in ("FRotator UpperArmRotation", "FRotator ForearmRotation", "float PunchArmAlpha"):
            self.assertIn(prop, ANIM_H)

    def test_pawn_writes_pose_from_replicated_phase(self):
        self.assertRegex(H, r"UPROPERTY\(Replicated\)\s*uint8 PunchPhaseRep")
        self.assertIn("DOREPLIFETIME(ATTARunnerPawn, PunchPhaseRep);", CPP)
        self.assertIn("PunchPhaseRep = static_cast<uint8>(PunchState.phase());", body("void ATTARunnerPawn::Tick("))
        b = body("void ATTARunnerPawn::UpdatePunchArm(")
        self.assertIn("tta::punchArmPose(", b)
        for prop in ("UpperArmRotation", "ForearmRotation", "PunchArmAlpha"):
            self.assertIn("Anim->%s =" % prop, b)

    def test_warns_once_without_anim_class(self):
        b = body("void ATTARunnerPawn::UpdatePunchArm(")
        self.assertIn("bWarnedAnimClass = true;", b)
        self.assertEqual(len(re.findall(r"UE_LOG\(", b)), 1)

    def test_corelib_wrapper(self):
        self.assertEqual((SRC / "Private/CoreLib/Core_PunchArm.cpp").read_text().strip(),
                         '#include "Core/src/PunchArm.cpp"')

    def test_stop_stun_fix_untouched(self):
        b = body("void ATTARunnerPawn::MulticastGetUp_Implementation(FVector_NetQuantize Landing)")
        self.assertIn("SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics);", b)


if __name__ == "__main__":
    unittest.main()
