import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
DOC = ROOT / "Docs" / "design" / "ui-style.md"
ART = ROOT / "Art" / "UI"
HEADER = (SRC / "Public" / "TTAUIStyle.h").read_text()
CPP = (SRC / "Private" / "TTAUIStyle.cpp").read_text()


def section(title):
    text = DOC.read_text()
    body = text.split(f"\n## {title}\n", 1)[1].split("\n## ", 1)[0]
    return [[c.strip() for c in line.strip("|").split("|")] for line in body.splitlines()
            if line.startswith("| `")]


def tick(cell):
    return cell.strip("`")


def enum(name):
    body = re.search(rf"enum class {name} : uint8\s*\{{(.*?)\}};", HEADER, re.S).group(1)
    return re.findall(r"^\s*(\w+),", body, re.M)


def png_has_alpha(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return False
    colour_type = data[25]
    return colour_type in (4, 6) or b"tRNS" in data[:data.find(b"IDAT")]


def widget_sources():
    files = set((SRC / "Private").glob("*Widget*.cpp")) | set((SRC / "Private").glob("*HUD*.cpp"))
    return sorted(f for f in files if f.name != "TTAUIStyle.cpp")


class UIStyleGuideTests(unittest.TestCase):
    def test_colours_match_both_ways(self):
        rows = section("Colours")
        self.assertEqual([tick(r[0]) for r in rows], enum("ETTAColor"))
        palette = re.search(r"kPaletteHex = \{(.*?)\};", (ROOT / "Core" / "include" / "tta" / "MenuFlow.h").read_text(), re.S).group(1)
        self.assertEqual([r[1].lstrip("#") for r in rows], re.findall(r'"([0-9A-F]{6})"', palette))

    def test_fonts_match_both_ways(self):
        rows = section("Fonts")
        self.assertEqual([tick(r[0]) for r in rows], enum("ETTAFont"))
        sizes = dict(re.findall(r"static constexpr int32 (k\w+) = (\d+);", HEADER))
        for token, size, _ in rows:
            const = re.search(rf"case ETTAFont::{tick(token)}: return MakeFont\([^;]*, (k\w+)\);", CPP).group(1)
            self.assertEqual(sizes[const], size, token)

    def test_spacing_matches_both_ways(self):
        rows = section("Spacing")
        scale = re.search(r"kSpaceScale\[\] = \{(.*?)\};", CPP).group(1)
        self.assertEqual([r[1] for r in rows], [v.strip().rstrip("f").rstrip(".") for v in scale.split(",")])
        self.assertEqual([tick(r[0]) for r in rows], [f"Space({i})" for i in range(len(rows))])

    def test_assets_match_code_and_art_both_ways(self):
        rows = section("Assets")
        names = [tick(r[0]) for r in rows]
        self.assertEqual(names, ["T_UI_" + n for n in enum("ETTAAsset")])
        self.assertEqual(sorted(names), sorted(p.stem for p in ART.glob("*.png")))
        table = re.findall(r"\{ETTAAsset::(\w+), ETTAColor::(\w+), ([\d.]+)f, ([\d.]+)f\}", CPP)
        self.assertEqual([("T_UI_" + a, c, float(x), float(y)) for a, c, x, y in table],
                         [(tick(r[0]), r[3], float(r[1]), float(r[2])) for r in rows])
        colours = enum("ETTAColor")
        for row in rows:
            self.assertIn(row[3], colours, row[0])

    def test_builders_and_kinds_match_both_ways(self):
        declared = re.findall(r"^\tstatic [\w*: ]+?\b(Color|Font|Space|PadAfter|Brush|Make(?!Font)\w+)\(", HEADER, re.M)
        self.assertEqual(sorted(tick(r[0]) for r in section("Builders")), sorted(declared))
        for name in declared:
            self.assertRegex(CPP, rf"UTTAUIStyle::{name}\(", name)
        self.assertEqual(sorted(tick(r[0]) for r in section("Component rules")),
                         sorted(enum("ETTAButtonKind") + enum("ETTAPanelKind")))

    def test_every_png_has_alpha(self):
        pngs = sorted(ART.glob("*.png"))
        self.assertTrue(pngs)
        for png in pngs:
            self.assertTrue(png_has_alpha(png), png.name)

    def test_widgets_use_tokens_not_literals(self):
        banned = re.compile(r"FLinearColor\(|FSlateFontInfo\(|FMargin\(\s*[-\d.]|/Game/|\.png\b|\.uasset\b")
        self.assertTrue(widget_sources())
        for cpp in widget_sources():
            code = re.sub(r"//[^\n]*", "", cpp.read_text())
            self.assertIsNone(banned.search(code), f"{cpp.name}: {banned.search(code)}")

    def test_editor_widget_uses_builders(self):
        cpp = (SRC / "Private" / "TTAEditorWidget.cpp").read_text()
        for builder in ("MakeButton", "MakeBanner", "MakeLabel", "MakeTextField", "MakePanel"):
            self.assertIn(f"UTTAUIStyle::{builder}(", cpp)
        self.assertNotIn("ConstructWidget<UButton>", cpp)
        self.assertNotIn("ConstructWidget<UEditableTextBox>", cpp)

    def test_import_script_targets_generated_ui_textures(self):
        script = (ROOT / "Scripts" / "import_ui_assets.py").read_text()
        self.assertIn('DEST = "/Game/UI/Generated"', script)
        self.assertIn('kGeneratedDir = TEXT("/Game/UI/Generated/")', CPP)
        for setting in ("TMGS_NO_MIPMAPS", "TEXTUREGROUP_UI", "TC_EDITOR_ICON"):
            self.assertIn(setting, script)


if __name__ == "__main__":
    unittest.main()
