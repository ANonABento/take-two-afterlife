"""Premiere must end in game, and build items must not use engine shapes."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


def body(text, signature):
    """Body of the function whose definition starts with `signature`."""
    start = text.index(signature)
    i = text.index("{", start)
    depth = 0
    for j in range(i, len(text)):
        depth += {"{": 1, "}": -1}.get(text[j], 0)
        if depth == 0:
            return text[i:j + 1]
    raise AssertionError(f"unbalanced {signature}")


def item_meshes():
    lines = (ROOT / "Config" / "DefaultGame.ini").read_text(encoding="utf-8-sig").splitlines()
    out, inside = [], False
    for line in lines:
        s = line.strip()
        if s.startswith("["):
            inside = s == "[/Script/TakeTwoAfterlife.TTAPlacementMarker]"
        elif inside and s.startswith("+ItemMeshes="):
            out.append(s.split("=", 1)[1])
    return out


class PremiereEnds(unittest.TestCase):
    def setUp(self):
        self.mode = (SRC / "Private" / "TTAGameMode.cpp").read_text(encoding="utf-8-sig")

    def test_end_premiere_writes_player_scores(self):
        end = body(self.mode, "bool ATTAGameMode::EndPremiere(")
        self.assertLess(end.index("Score.addRatings"), end.index("SetScore("))
        self.assertIn("Score.total(PS->GetPlayerId())", end)

    def test_cinema_finish_calls_end_premiere(self):
        finish = body(self.mode, "void ATTAGameMode::OnCinemaStep(")
        self.assertIn("PremiereStep::Podium", finish)
        self.assertIn("PremiereStep::Done", finish)
        self.assertIn("FinishPremiere(", finish)
        self.assertIn("EndPremiere(", body(self.mode, "void ATTAGameMode::FinishPremiere("))

    def test_cinema_is_started_and_bound_on_server(self):
        begin = body(self.mode, "void ATTAGameMode::BeginPremiere(")
        self.assertIn("StartPremiere(", begin)
        self.assertIn("OnStep.AddUObject(this, &ATTAGameMode::OnCinemaStep)", begin)
        self.assertIn("BeginPremiere()", body(self.mode, "void ATTAGameMode::PostSeamlessTravel("))

    def test_timeout_is_premiere_length_plus_three_seconds(self):
        begin = body(self.mode, "void ATTAGameMode::BeginPremiere(")
        self.assertIn("PremiereTimeoutSlack", begin)
        header = (SRC / "Public" / "TTAGameMode.h").read_text(encoding="utf-8-sig")
        self.assertRegex(header, r"PremiereTimeoutSlack\s*=\s*3\.f")
        self.assertIn("FinishPremiere(", body(self.mode, "void ATTAGameMode::Tick("))

    def test_bots_rate_in_cinema(self):
        self.assertIn("RateCurrent(", body(self.mode, "void ATTAGameMode::OnCinemaStep("))


class BuildItemMeshes(unittest.TestCase):
    def test_ten_items_none_from_engine(self):
        meshes = item_meshes()
        self.assertEqual(len(meshes), 10)
        for m in meshes:
            self.assertFalse(m.startswith("/Engine/"), m)

    def test_meshes_are_textured_traps_and_cams(self):
        content = ROOT / "Content"
        for m in item_meshes():
            pkg, _, obj = m.partition(".")
            self.assertTrue(pkg.startswith("/Game/Art/Traps/") or pkg.startswith("/Game/Art/Cams/"), m)
            name = pkg.rsplit("/", 1)[1]
            self.assertEqual(name, obj, m)
            rel = pkg.removeprefix("/Game/")
            self.assertTrue((content / f"{rel}.uasset").is_file(), m)


class DevlogSoloCheck(unittest.TestCase):
    def test_solo_round_reaches_heaven_podium(self):
        log = (ROOT / "DEVLOG.md").read_text(encoding="utf-8")
        section = log[log.index("## Solo round to the Heaven podium"):]
        self.assertIn("podium", section.lower())
        self.assertIn("EndPremiere", section)


if __name__ == "__main__":
    unittest.main()
