import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
TITLE_BUTTONS = ["PLAY SOLO", "HOST GAME", "JOIN (CODE OR IP)", "SETTINGS", "QUIT"]
ART_SLOTS = ["MenuBackground", "MenuLogo", "ButtonNormal", "ButtonHovered", "ButtonPressed"]
BUTTON = re.compile(r'MenuButton\(Box, TEXT\("([^"]+)"\)[^;]*?(?:->OnClicked\.AddDynamic\(this, &UTTAMenuWidget::(\w+)\))?;')


def section(cpp, name, next_name):
    """Body of the `\t// <name>...` block in NativeOnInitialized, up to the next screen's comment."""
    return cpp[cpp.index(f"\t// {name}"):cpp.index(f"\t// {next_name}")]


class MenuStructureTests(unittest.TestCase):
    """Title menu is Play Solo / Host / Join / Settings / Quit, art drops in via TTAUIStyle."""

    def setUp(self):
        cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        self.cpp = cpp
        self.title = section(cpp, "Title", "Join")
        self.header = (SRC / "Public" / "TTAMenuWidget.h").read_text()

    def test_title_buttons_in_order(self):
        self.assertEqual([m.group(1) for m in BUTTON.finditer(self.title)], TITLE_BUTTONS)

    def test_title_buttons_share_one_width_and_host_is_primary(self):
        kinds = re.findall(r'MenuButton\(Box, TEXT\("([^"]+)"\), ETTAButtonKind::(\w+)\)', self.title)
        self.assertEqual([label for label, _ in kinds], TITLE_BUTTONS, "every title button uses the kind overload, no width arg")
        self.assertEqual([label for label, kind in kinds if kind == "Primary"], ["HOST GAME"])
        body = self.cpp[self.cpp.index("UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind)"):]
        body = body[:body.index("\n}\n")]
        self.assertIn("SetWidthOverride(kButtonW)", body)
        self.assertIn("SetHeightOverride(kButtonH)", body)

    def test_title_logo_top_left_and_panels_centred(self):
        self.assertIn("ScreenPanel(kButtonW, HAlign_Left, VAlign_Top)", self.title)
        self.assertLess(self.title.index("MenuLogo"), self.title.index("MenuButton("))
        self.assertIn("JoinCard = Card(Box", section(self.cpp, "Join", "Lobby"))
        self.assertRegex(self.header, r"ScreenPanel\(float Width, EHorizontalAlignment HAlign = HAlign_Center, EVerticalAlignment VAlign = VAlign_Center\)")
        how = section(self.cpp, "How to play", "Reels")
        self.assertIn("ScreenPanel(kWideW)", how)
        self.assertIn("MakePanel(WidgetTree, ETTAPanelKind::Panel)", how)
        self.assertIn('MenuButton(PanelBox, TEXT("BACK")', how)

    def test_every_screen_sits_inside_the_safe_area(self):
        self.assertIn("SwitcherSlot->SetPadding(UTTAUIStyle::Get().SafeArea);", self.cpp)
        panel = self.cpp[self.cpp.index("UVerticalBox* UTTAMenuWidget::ScreenPanel("):]
        panel = panel[:panel.index("\n}\n")]
        self.assertIn("Switcher->AddChild(", panel)
        self.assertNotIn("SetPadding", panel, "screens add no padding that could push past the safe area")
        self.assertEqual(self.cpp.count("Switcher->AddChild("), 1, "every screen goes through ScreenPanel")
        ini = (ROOT / "Config" / "DefaultGame.ini").read_text()
        self.assertIn("SafeArea=(Left=96,Top=54,Right=96,Bottom=54)", ini)
        for screen, nxt in [("Title", "Join"), ("Join", "Lobby"), ("How to play", "Reels")]:
            self.assertNotIn("SetRenderTranslation", section(self.cpp, screen, nxt), screen)

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
