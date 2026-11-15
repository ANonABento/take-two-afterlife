import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
TITLE_BUTTONS = ["PLAY SOLO", "HOST GAME", "JOIN (CODE OR IP)", "SETTINGS", "QUIT"]
ART_SLOTS = ["MenuBackground", "MenuLogo", "ButtonNormal", "ButtonHovered", "ButtonPressed"]
BUTTON = re.compile(r'MenuButton\(Box, TEXT\("([^"]+)"\)[^;]*?(?:->OnClicked\.AddDynamic\(this, &UTTAMenuWidget::(\w+)\))?;')


class MenuStructureTests(unittest.TestCase):
    """Title menu is Play Solo / Host / Join / Settings / Quit, art drops in via TTAUIStyle."""

    def setUp(self):
        cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        self.title = cpp[cpp.index("\t// Title\n"):cpp.index("\t// Join\n")]
        self.header = (SRC / "Public" / "TTAMenuWidget.h").read_text()

    def test_title_buttons_in_order(self):
        self.assertEqual([m.group(1) for m in BUTTON.finditer(self.title)], TITLE_BUTTONS)

    def test_every_title_button_has_a_declared_handler(self):
        for label, handler in BUTTON.findall(self.title):
            self.assertTrue(handler, f"{label} has no OnClicked handler")
            self.assertRegex(self.header, rf"UFUNCTION\(\) void {handler}\(", label)

    def test_art_slots_exist_in_style_and_config(self):
        style = (SRC / "Public" / "TTAUIStyle.h").read_text()
        ini = (ROOT / "Config" / "DefaultGame.ini").read_text()
        section = ini.split("[/Script/TakeTwoAfterlife.TTAUIStyle]", 1)[1].split("\n[", 1)[0]
        for slot in ART_SLOTS:
            self.assertRegex(style, rf"UPROPERTY\(Config\) FSoftObjectPath {slot};", slot)
            self.assertRegex(section, rf"\n{slot}=", slot)

    def test_menu_reads_logo_and_button_slots(self):
        cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        for slot in ART_SLOTS[1:]:
            self.assertIn(f"UIStyle.{slot}" if slot.startswith("Button") else f".{slot})", cpp, slot)


if __name__ == "__main__":
    unittest.main()
