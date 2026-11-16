import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import cosmetics_rows as C  # noqa: E402
import toon_spec  # noqa: E402

ROOT = C.REPO


class CosmeticsRowTests(unittest.TestCase):
    def test_one_row_per_core_hat_in_core_order(self):
        rows = C.build_rows()
        self.assertEqual(len(rows), 8)
        self.assertEqual(rows[0]["HatMesh"], "/Game/Characters/Hats/SM_Hat_BunnyEars.SM_Hat_BunnyEars")
        self.assertEqual(rows[1]["FaceTexture"], "/Game/Characters/Faces/T_Face_Happy.T_Face_Happy")
        self.assertEqual(rows[6]["FaceTexture"], "/Game/Characters/Faces/T_Face_Dizzy.T_Face_Dizzy")
        self.assertEqual(rows[7]["HatMesh"], "/Game/Characters/Hats/SM_Hat_VikingHelmet.SM_Hat_VikingHelmet")
        self.assertEqual([r["Name"] for r in rows], [f"Look{i}" for i in range(8)])

    def test_every_row_has_its_source_file(self):
        for src, _ in C.source_files(C.build_rows()):
            self.assertTrue(src.is_file(), src)

    def test_palette_index_in_range(self):
        for r in C.build_rows():
            self.assertIn(r["PaletteIndex"], range(len(toon_spec.PALETTE)))

    def test_csv_has_header_and_ue_struct_text(self):
        lines = C.to_csv(C.build_rows()).splitlines()
        self.assertEqual(lines[0], "---,HatMesh,SocketOffset,Rotation,Scale,FaceTexture,PaletteIndex")
        self.assertEqual(len(lines), 9)
        self.assertIn('"(X=0,Y=0,Z=12)"', lines[1])
        self.assertIn('"(Pitch=0,Yaw=0,Roll=0)"', lines[1])

    def test_mismatch_and_unknown_hat_fail_loudly(self):
        with self.assertRaises(ValueError):
            C.build_rows(["Crown"], [])
        with self.assertRaises(ValueError):
            C.build_rows(["Tophat"], ["Happy"])

    def test_struct_fields_match_cpp(self):
        header = (ROOT / "Source/TakeTwoAfterlife/Public/TTACosmetics.h").read_text(encoding="utf-8")
        found = re.findall(r"UPROPERTY\([^)]*\)\s*[\w<>:]+\s+(\w+)", header)
        self.assertEqual(tuple(found), C.FIELDS[1:])
        self.assertIn("FTTACosmeticRow", header)

    def test_runner_reads_table(self):
        game = (ROOT / "Config/DefaultGame.ini").read_text(encoding="utf-8")
        self.assertIn("CosmeticsTable=/Game/Data/DT_Cosmetics.DT_Cosmetics", game)


if __name__ == "__main__":
    unittest.main()
