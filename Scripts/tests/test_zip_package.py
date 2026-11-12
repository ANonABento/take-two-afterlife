import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import zip_package  # noqa: E402


class ZipPackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.build = self.root / "Windows"
        (self.build / "TakeTwoAfterlife" / "Content" / "Paks").mkdir(parents=True)
        (self.build / "TakeTwoAfterlife.exe").write_bytes(b"MZ")
        (self.build / "TakeTwoAfterlife" / "Content" / "Paks" / "a.pak").write_bytes(b"x" * 100)

    def tearDown(self):
        self.tmp.cleanup()

    def test_names_zip_and_root_folder_after_team(self):
        out = zip_package.make_zip(self.build, "Bento", self.root / "out")
        self.assertEqual(out.name, "Bento_TakeTwoAfterlife.zip")
        with zipfile.ZipFile(out) as zf:
            names = zf.namelist()
        self.assertIn("Bento_TakeTwoAfterlife/TakeTwoAfterlife.exe", names)
        self.assertIn("Bento_TakeTwoAfterlife/TakeTwoAfterlife/Content/Paks/a.pak", names)

    def test_rejects_zip_at_or_over_limit_and_removes_it(self):
        with self.assertRaises(ValueError):
            zip_package.make_zip(self.build, "Bento", self.root / "out", max_bytes=10)
        self.assertFalse((self.root / "out" / "Bento_TakeTwoAfterlife.zip").exists())

    def test_rejects_missing_exe(self):
        (self.build / "TakeTwoAfterlife.exe").unlink()
        with self.assertRaises(FileNotFoundError):
            zip_package.make_zip(self.build, "Bento", self.root / "out")

    def test_rejects_bad_team_name(self):
        for bad in ["", "Team Name", "a/b", "x_y"]:
            with self.assertRaises(ValueError):
                zip_package.zip_name(bad)

    def test_limit_is_one_gib(self):
        self.assertEqual(zip_package.MAX_BYTES, 1024 ** 3)


if __name__ == "__main__":
    unittest.main()
