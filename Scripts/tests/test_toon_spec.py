import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import toon_spec  # noqa: E402


class PaletteTests(unittest.TestCase):
    def test_twelve_distinct_colours(self):
        self.assertEqual(len(toon_spec.PALETTE), 12)
        self.assertEqual(len(set(toon_spec.palette_rgb())), 12)

    def test_png_is_12x1_and_holds_the_palette(self):
        png = toon_spec.palette_png()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        self.assertEqual(struct.unpack(">II", png[16:24]), (12, 1))
        idat_len = struct.unpack(">I", png[33:37])[0]
        raw = zlib.decompress(png[41:41 + idat_len])
        self.assertEqual(raw[0], 0)
        self.assertEqual(tuple(raw[1:4]), toon_spec.palette_rgb()[0])
        self.assertEqual(tuple(raw[-3:]), toon_spec.palette_rgb()[11])

    def test_palette_u_hits_texel_centres(self):
        self.assertAlmostEqual(toon_spec.palette_u(0), 1 / 24)
        self.assertAlmostEqual(toon_spec.palette_u(11), 23 / 24)
        with self.assertRaises(ValueError):
            toon_spec.palette_u(12)

    def test_nearest_index(self):
        self.assertEqual(toon_spec.nearest_palette_index((250, 0, 0)), 2)
        self.assertEqual(toon_spec.nearest_palette_index((0, 0, 0)), 0)
        self.assertEqual(toon_spec.nearest_palette_index((255, 255, 255)), 1)

    def test_bad_hex_rejected(self):
        with self.assertRaises(ValueError):
            toon_spec.hex_to_rgb("12345")


class NamingTests(unittest.TestCase):
    def test_instance_names(self):
        self.assertEqual(toon_spec.instance_name("M_Crate"), "MI_TTA_Toon_Crate")
        self.assertEqual(toon_spec.instance_name("wood.001"), "MI_TTA_Toon_wood_001")
        with self.assertRaises(ValueError):
            toon_spec.instance_name("M_")


class PresetTests(unittest.TestCase):
    def test_repo_presets_match_the_look(self):
        p = toon_spec.load_world_presets()
        self.assertLess(p["Heaven"]["Saturation"], p["Course"]["Saturation"])
        self.assertLess(p["Heaven"]["Contrast"], p["Course"]["Contrast"])
        self.assertGreater(p["Heaven"]["ShadowFloor"], p["Course"]["ShadowFloor"])
        self.assertGreater(p["Heaven"]["Warmth"], p["Course"]["Warmth"])
        for preset in p.values():
            self.assertIn(preset["Bands"], (2.0, 3.0))

    def test_missing_field_fails_loudly(self):
        with tempfile.TemporaryDirectory() as tmp:
            ini = Path(tmp) / "g.ini"
            ini.write_text("[/Script/TakeTwoAfterlife.TTAWorldLookSettings]\nHeaven=(Saturation=1)\n")
            with self.assertRaisesRegex(ValueError, "missing"):
                toon_spec.load_world_presets(ini)
