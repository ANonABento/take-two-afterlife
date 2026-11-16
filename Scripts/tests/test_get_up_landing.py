"""Get up where you landed: StopStun lands the capsule at the floor under the ragdoll pelvis
on the server and sends that spot to everyone; it never respawns or restores an old location."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
H = (ROOT / "Source/TakeTwoAfterlife/Public/TTARunnerPawn.h").read_text(encoding="utf-8-sig")
CPP = (ROOT / "Source/TakeTwoAfterlife/Private/TTARunnerPawn.cpp").read_text(encoding="utf-8-sig")


def body(signature):
    return CPP.split(signature, 1)[1].split("\n}\n", 1)[0]


class GetUpLandingTest(unittest.TestCase):
    def test_server_reads_pelvis_and_traces_floor(self):
        b = body("void ATTARunnerPawn::StopStun()")
        self.assertIn("HasAuthority()", b)
        self.assertIn("GetBoneLocation(PelvisBone)", b)
        self.assertIn("LineTraceSingleByChannel", b)
        self.assertIn("MulticastGetUp(", b)

    def test_stop_stun_never_respawns_or_restores_old_spot(self):
        for name in ("StopStun()", "MulticastGetUp_Implementation"):
            b = body("void ATTARunnerPawn::" + name)
            for banned in ("Respawn(", "StartLocation", "SetStartLocation", "Saved", "LastLocation"):
                self.assertNotIn(banned, b, (name, banned))

    def test_multicast_puts_mesh_on_capsule_at_landing(self):
        self.assertRegex(H, r"UFUNCTION\(NetMulticast, Reliable\)\s*void MulticastGetUp\(")
        b = body("void ATTARunnerPawn::MulticastGetUp_Implementation")
        self.assertLess(b.index("RestoreBody();"), b.index("SetActorLocation(Landing"))

    def test_trap_path_unchanged(self):
        self.assertIn("case tta::LifeEvent::Respawn: Respawn(); break;", CPP)
        self.assertIn("SetActorLocation(StartLocation", body("void ATTARunnerPawn::Respawn()"))


if __name__ == "__main__":
    unittest.main()
