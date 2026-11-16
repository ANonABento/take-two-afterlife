import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
FIELDS = ["master", "music", "vo", "sfx", "sensitivity"]


def body(cpp, signature):
    text = cpp[cpp.index(signature):]
    return text[:text.index("\n}\n")]


class SettingsSlidersTests(unittest.TestCase):
    """Settings: one labelled slider per ClientSettings value, number beside it, no text summary line."""

    def setUp(self):
        self.cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        self.header = (SRC / "Public" / "TTAMenuWidget.h").read_text()
        self.screen = self.cpp[self.cpp.index("\t// Settings\n"):self.cpp.index("\t// How to play")]

    def test_five_sliders_bound_to_five_fields(self):
        calls = re.findall(r'SettingSlider\(Inner, TEXT\("([^"]+)"\), Settings\.(\w+) / \w+\)'
                           r'->OnValueChanged\.AddDynamic\(this, &UTTAMenuWidget::(\w+)\)', self.screen)
        self.assertEqual(len(calls), 5)
        self.assertEqual([c[0] for c in calls], ["Master", "Music", "Voice", "SFX", "Sensitivity"])
        self.assertEqual([c[1] for c in calls], FIELDS, "each slider starts from its own field")
        for _, field, handler in calls:
            m = re.search(rf"void {handler}\(float V\) \{{ Settings\.(\w+) = ", self.header)
            self.assertIsNotNone(m, handler)
            self.assertEqual(m.group(1), field, f"{handler} writes {field}")

    def test_sliders_use_style_art_and_show_number(self):
        make = body(self.cpp, "USlider* UTTAMenuWidget::SettingSlider(")
        self.assertIn("UTTAUIStyle::MakeSlider(WidgetTree)", make)
        self.assertIn("SettingValues.Add(Label(", make)
        style = body((SRC / "Private" / "TTAUIStyle.cpp").read_text(), "USlider* UTTAUIStyle::MakeSlider(")
        self.assertIn("ETTAAsset::SliderBar", style)
        self.assertIn("ETTAAsset::SliderKnob", style)

    def test_refresh_updates_numbers_and_keeps_clamp(self):
        refresh = body(self.cpp, "void UTTAMenuWidget::RefreshSettings(")
        self.assertIn("tta::clampClientSettings(Settings)", refresh)
        self.assertIn("Settings.master, Settings.music, Settings.vo, Settings.sfx, Settings.sensitivity", refresh)
        self.assertIn("SettingValues[i]->SetText(", refresh)

    def test_no_text_summary_line(self):
        self.assertNotIn("SettingsText", self.cpp + self.header)


if __name__ == "__main__":
    unittest.main()
