import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"
TITLE_BUTTONS = ["PLAY", "SETTINGS", "REELS", "QUIT"]
LOBBY_BUTTONS = ["COLOUR >", "HAT >", "FACE >", "START", "SETTINGS", "LEAVE"]
PLAY_BUTTONS = ["HOST", "PLAY SOLO", "JOIN", "BACK"]
ART_SLOTS = ["MenuBackground", "MenuLogo", "ButtonNormal", "ButtonHovered", "ButtonPressed"]
BUTTON = re.compile(r'MenuButton\(\w+, TEXT\("([^"]+)"\)[^;]*?(?:->OnClicked\.AddDynamic\(this, &UTTAMenuWidget::(\w+)\))?;')


class MenuStructureTests(unittest.TestCase):
    """Title is Play / Settings / Reels / Quit; PLAY opens the Play page (Host / Solo / Join / Back)."""

    def setUp(self):
        cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        self.title = cpp[cpp.index("\t// Title"):cpp.index("\t// Join")]
        self.play = cpp[cpp.index("\t// Join"):cpp.index("\t// Lobby\n")]
        self.lobby = cpp[cpp.index("\t// Lobby\n"):cpp.index("\t// Settings\n")]
        self.cpp = cpp
        self.header = (SRC / "Public" / "TTAMenuWidget.h").read_text()

    def test_title_buttons_in_order(self):
        self.assertEqual([m.group(1) for m in BUTTON.finditer(self.title)], TITLE_BUTTONS)

    def test_play_page_buttons_in_order(self):
        self.assertEqual([m.group(1) for m in BUTTON.finditer(self.play)], PLAY_BUTTONS)
        self.assertIn("MakeTextField(", self.play)
        self.assertIn("OnPlayClicked() { Open(tta::MenuScreen::Join); }", self.header)

    def test_lobby_is_compact_left_panel(self):
        self.assertEqual([m.group(1) for m in BUTTON.finditer(self.lobby)], LOBBY_BUTTONS)
        self.assertIn("ScreenPanel(kLobbyW)", self.lobby)
        self.assertIn("SetHorizontalAlignment(HAlign_Left)", self.lobby)
        self.assertIn("MakePanel(WidgetTree, ETTAPanelKind::CodePill)", self.lobby)
        self.assertIn("MakeIconCount(WidgetTree, ETTAAsset::IconReady", self.lobby)
        self.assertIn("StartButton->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnStartClicked);", self.lobby)
        for bound in ("RoomCodeText =", "WaitingText =", "LookSwatch =", "LookText =", "StartButton ="):
            self.assertIn(bound, self.lobby, bound)

    def test_menu_uses_only_style_builders(self):
        for raw in ("ConstructWidget<UButton>", "ConstructWidget<USlider>", "ConstructWidget<UEditableTextBox>",
                    "ConstructWidget<UTextBlock>", "FSlateRoundedBoxBrush", "MakeFont("):
            self.assertNotIn(raw, self.cpp, raw)
        self.assertIn("UTTAUIStyle::MakeAngel(", self.title)

    def test_every_title_button_has_a_declared_handler(self):
        for label, handler in BUTTON.findall(self.title + self.play + self.lobby.replace("StartButton = MenuButton", "StartButton = ")):
            self.assertTrue(handler, f"{label} has no OnClicked handler")
            self.assertRegex(self.header, rf"UFUNCTION\(\) void {handler}\(", label)

    def test_art_slots_exist_in_style_and_config(self):
        style = (SRC / "Public" / "TTAUIStyle.h").read_text()
        ini = (ROOT / "Config" / "DefaultGame.ini").read_text()
        section = ini.split("[/Script/TakeTwoAfterlife.TTAUIStyle]", 1)[1].split("\n[", 1)[0]
        for slot in ART_SLOTS:
            self.assertRegex(style, rf"UPROPERTY\(Config\) FSoftObjectPath {slot};", slot)
            self.assertRegex(section, rf"\n{slot}=", slot)

    def test_menu_reads_logo_slot(self):
        self.assertIn(".MenuLogo)", self.cpp)


if __name__ == "__main__":
    unittest.main()
