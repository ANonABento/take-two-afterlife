import ast
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))
import release_checks  # noqa: E402
import toon_spec  # noqa: E402
import zip_package  # noqa: E402

ROOT = SCRIPTS.parent
ENGINE = (ROOT / "Config" / "DefaultEngine.ini").read_text(encoding="utf-8")
GAME = (ROOT / "Config" / "DefaultGame.ini").read_text(encoding="utf-8")


class MapListTests(unittest.TestCase):
    def test_match_maps_are_title_backyard_heaven(self):
        self.assertEqual(release_checks.match_maps(GAME),
                         ["/Game/Maps/Title", "/Game/Maps/Backyard", "/Game/Maps/Heaven"])

    def test_look_covers_every_match_map(self):
        self.assertEqual(sorted(toon_spec.LOOK_MAPS), sorted(release_checks.match_maps(GAME)))
        self.assertLessEqual(set(toon_spec.LOOK_MAPS.values()), set(toon_spec.load_world_presets()))

    def test_repo_config_passes(self):
        self.assertEqual(release_checks.problems(ENGINE, GAME), [])
        release_checks.check(ROOT)


class CookCheckTests(unittest.TestCase):
    def test_missing_match_map_fails(self):
        game = GAME.replace('+MapsToCook=(FilePath="/Game/Maps/Heaven")\n', "")
        self.assertEqual(release_checks.problems(ENGINE, game), ["/Game/Maps/Heaven is not in MapsToCook"])

    def test_title_not_default_fails(self):
        engine = ENGINE.replace("GameDefaultMap=/Game/Maps/Title.Title", "GameDefaultMap=/Game/Maps/Backyard.Backyard")
        self.assertEqual(len(release_checks.problems(engine, GAME)), 1)
        self.assertIn("GameDefaultMap", release_checks.problems(engine, GAME)[0])

    def test_check_raises_on_bad_config(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg = Path(tmp) / "Config"
            cfg.mkdir()
            (cfg / "DefaultEngine.ini").write_text(ENGINE, encoding="utf-8")
            (cfg / "DefaultGame.ini").write_text(GAME.replace("Maps/Backyard\")", "Maps/Backyard2\")"), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Backyard is not in MapsToCook"):
                release_checks.check(tmp)

    def test_package_bat_runs_checks_before_cook(self):
        bat = (SCRIPTS / "package_win64.bat").read_text()
        self.assertLess(bat.index("release_checks.py"), bat.index("BuildCookRun"))


class SizeCheckTests(unittest.TestCase):
    def test_zip_at_exact_limit_fails_and_is_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            build = Path(tmp) / "Windows"
            build.mkdir()
            (build / "TakeTwoAfterlife.exe").write_bytes(b"MZ")
            probe = zip_package.make_zip(build, "Bento", Path(tmp) / "a")
            size = probe.stat().st_size
            with self.assertRaises(ValueError):
                zip_package.make_zip(build, "Bento", Path(tmp) / "b", max_bytes=size)
            self.assertFalse((Path(tmp) / "b" / "Bento_TakeTwoAfterlife.zip").exists())
            self.assertTrue(zip_package.make_zip(build, "Bento", Path(tmp) / "c", max_bytes=size + 1).is_file())

    def test_limit_is_one_gib(self):
        self.assertEqual(zip_package.MAX_BYTES, 1024 ** 3)


class LookScriptTests(unittest.TestCase):
    def test_look_script_uses_outline_unbound_volume(self):
        src = (SCRIPTS / "apply_world_look.py").read_text()
        ast.parse(src)
        for needle in ("OUTLINE_MATERIAL", '"unbound", True', "load_world_presets", "LOOK_TAG"):
            self.assertIn(needle, src)
