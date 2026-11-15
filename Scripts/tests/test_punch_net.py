"""Punch plan task 2: the Punch input, Server RPC and Multicast stun exist on the runner pawn."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INI = (ROOT / "Config/DefaultInput.ini").read_text(encoding="utf-8-sig")
H = (ROOT / "Source/TakeTwoAfterlife/Public/TTARunnerPawn.h").read_text(encoding="utf-8-sig")
CPP = (ROOT / "Source/TakeTwoAfterlife/Private/TTARunnerPawn.cpp").read_text(encoding="utf-8-sig")


def body(signature):
    return CPP.split(signature, 1)[1].split("\n}\n", 1)[0]


class PunchNetTest(unittest.TestCase):
    def test_ini_maps_lmb_and_gamepad(self):
        for key in ("LeftMouseButton", "Gamepad_FaceButton_Right"):
            self.assertRegex(INI, r'\+ActionMappings=\(ActionName="Punch",[^)]*Key=%s\)' % key)

    def test_bound_like_dive(self):
        self.assertIn('Input->BindAction(TEXT("Punch"), IE_Pressed, this, &ATTARunnerPawn::Punch);', CPP)

    def test_server_rpc(self):
        self.assertRegex(H, r"UFUNCTION\(Server, Reliable\)\s*void ServerPunch\(\);")
        self.assertIn("PunchState.start(Life.state())", body("void ATTARunnerPawn::ServerPunch_Implementation()"))

    def test_multicast_ragdolls(self):
        self.assertRegex(H, r"UFUNCTION\(NetMulticast, Reliable\)\s*void MulticastPunched\(")
        b = body("void ATTARunnerPawn::MulticastPunched_Implementation")
        self.assertIn("Life.stun(tta::kStunSeconds)", b)
        self.assertIn("StartRagdoll();", b)

    def test_get_up_in_place(self):
        self.assertIn("case tta::LifeEvent::GetUp: StopStun(); break;", CPP)
        self.assertNotIn("StartLocation", body("void ATTARunnerPawn::StopStun()"))
        self.assertIn("MulticastGetUp(", body("void ATTARunnerPawn::StopStun()"))

    def test_wind_up_and_hit_feel(self):
        self.assertRegex(H, r"UFUNCTION\(NetMulticast, Unreliable\)\s*void MulticastWindUp\(\);")
        self.assertIn("MulticastWindUp();", body("void ATTARunnerPawn::ServerPunch_Implementation()"))
        self.assertIn("tta::AudioEvent::PunchWindUp", body("void ATTARunnerPawn::MulticastWindUp_Implementation()"))
        self.assertIn("SetRelativeScale3D", body("void ATTARunnerPawn::UpdateWindUpPose"))
        b = body("void ATTARunnerPawn::MulticastPunched_Implementation")
        self.assertIn("tta::AudioEvent::PunchHit", b)
        self.assertIn("ClientStartCameraShake(ImpactShake)", b)


if __name__ == "__main__":
    unittest.main()
