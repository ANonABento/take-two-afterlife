"""Demo heaven is Heaven.umap streamed into the race map, not the engine-shape stand-in."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


def read(rel):
    return (SRC / rel).read_text(encoding="utf-8-sig")


def body(text, signature):
    """Text of the function that starts at `signature`, up to the next top-level closing brace."""
    start = text.index(signature)
    return text[start:text.index("\n}\n", start)]


class HeavenStreamTest(unittest.TestCase):
    def setUp(self):
        self.mode = read("Private/TTAGameMode.cpp")
        self.watch = read("Private/TTAHeavenWatch.cpp")

    def test_demo_streams_the_heaven_map(self):
        load = body(self.mode, "void ATTAGameMode::LoadHeavenSet()")
        self.assertIn("ULevelStreamingDynamic::LoadLevelInstance(this, HeavenMap", load)
        self.assertIn("LoadHeavenSet();", body(self.mode, "void ATTAGameMode::StartPlay()"))
        self.assertIn('#include "Engine/LevelStreamingDynamic.h"', self.mode)
        game_ini = (ROOT / "Config" / "DefaultGame.ini").read_text(encoding="utf-8")
        self.assertIn("HeavenMap=/Game/Maps/Heaven", game_ini)
        self.assertIn('+MapsToCook=(FilePath="/Game/Maps/Heaven")', game_ini)

    def test_streamed_map_loses_its_sky_and_starts(self):
        adopt = body(self.mode, "void ATTAGameMode::AdoptHeavenSet()")
        for cls in ("ALight", "ASkyLight", "ASkyAtmosphere", "AExponentialHeightFog", "APostProcessVolume", "APlayerStart"):
            self.assertIn(f"IsA<{cls}>()", adopt)
        self.assertIn("HeavenWatch->UseStage(Stage)", adopt)

    def test_race_ignores_the_heaven_gate(self):
        run = body(self.mode, "void ATTAGameMode::EnterRun()")
        self.assertRegex(run, r"GetLevel\(\) != GetWorld\(\)->PersistentLevel\) continue;\s+Gate = ")
        # The map's own gate is the round starter; no cube gate is spawned on top of it.
        spawn = body(self.watch, "void ATTAHeavenWatch::SpawnContinueGate()")
        self.assertLess(spawn.index("if (Stage) return;"), spawn.index("SpawnActor<ATTAFinishGate>"))

    def test_streamed_cinema_leaves_the_world_look_alone(self):
        begin = body(read("Private/TTACinema.cpp"), "void ATTACinema::BeginPlay()")
        self.assertLess(begin.index("GetLevel() != GetWorld()->PersistentLevel) return;"), begin.index("ApplyWorldLook"))

    def test_fallers_go_back_on_the_cloud(self):
        rescue = body(self.mode, "void ATTAGameMode::RescueHeavenFallers()")
        self.assertIn("GetWalkTransform()", rescue)
        self.assertIn("IsOnMap(HeavenMap)", rescue)
        self.assertIn("ETeleportType::TeleportPhysics", rescue)
        self.assertIn("RescueHeavenFallers();", body(self.mode, "void ATTAGameMode::Tick(float DeltaSeconds)"))

    def test_tv_is_an_opaque_unscaled_widget(self):
        self.assertIn("EWidgetBlendMode::Opaque", self.watch)
        self.assertNotIn("EWidgetBlendMode::Transparent", self.watch)
        # The feed hangs off the unscaled root, so 960 x 540 px stays 9.6 x 5.4 m.
        self.assertIn("ScreenWidget->SetupAttachment(RootComponent)", self.watch)
        self.assertRegex(self.watch, r'RootComponent = CreateDefaultSubobject<USceneComponent>\(TEXT\("Root"\)\)')

    def test_spots_match_the_heaven_layout(self):
        spots = dict(re.findall(r"const FVector (\w+)\(([^)]*)\)", self.watch))
        xyz = {name: [float(v.strip().rstrip("f")) for v in args.split(",")] for name, args in spots.items()}
        import json
        layout = json.loads((ROOT / "Scripts" / "heaven_layout.json").read_text(encoding="utf-8"))
        gate = next(p for p in layout["protected"] if p["label"] == "Gate")
        screen = next(p for p in layout["protected"] if p["label"] == "Screen")
        self.assertEqual(xyz["GateSpot"][:2], [gate["x"], gate["y"]])
        self.assertEqual(xyz["ScreenCentre"][:2], [screen["x"], screen["y"]])
        # The walk spot is on the cloud (15 m disc) and clear of every prop footprint.
        wx, wy = xyz["WalkSpot"][:2]
        self.assertLess((wx * wx + wy * wy) ** 0.5, 1400.0)
        pieces = [p for p in layout["protected"] if p["label"] not in ("ScreenView", "PlayerStarts")]
        pieces += layout["podium_dressing"] + layout["spectator_ring"]
        for piece in pieces:
            inside = abs(wx - piece["x"]) < piece["half"][0] + 25 and abs(wy - piece["y"]) < piece["half"][1] + 25
            self.assertFalse(inside, piece["label"])


if __name__ == "__main__":
    unittest.main()
