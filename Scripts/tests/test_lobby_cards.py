import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"


def body(cpp, signature):
    text = cpp[cpp.index(signature):]
    return text[:text.index("\n}\n")]


class LobbyCardsTests(unittest.TestCase):
    """Lobby matches mock-lobby.png: code pill, row of up to 4 player cards, Customize left, Start right."""

    def setUp(self):
        self.cpp = (SRC / "Private" / "TTAMenuWidget.cpp").read_text()
        self.header = (SRC / "Public" / "TTAMenuWidget.h").read_text()
        self.lobby = self.cpp[self.cpp.index("\t// Lobby\n"):self.cpp.index("\t// Settings\n")]

    def test_layout_order_pill_cards_buttons(self):
        pill = self.lobby.index("ETTAPanelKind::CodePill")
        row = self.lobby.index("CardRow = WidgetTree")
        customize = self.lobby.index('TEXT("CUSTOMIZE")')
        start = self.lobby.index('TEXT("START")')
        self.assertLess(pill, row)
        self.assertLess(row, customize)
        self.assertLess(customize, start, "Customize is left of Start")
        self.assertIn("PillSlot->SetHorizontalAlignment(HAlign_Center)", self.lobby)
        self.assertIn("StartSlot->SetHorizontalAlignment(HAlign_Right)", self.lobby)
        self.assertIn("ESlateSizeRule::Fill", self.lobby)

    def test_card_row_has_four_slots_with_avatar_name_tick(self):
        self.assertIn("static constexpr int32 kMaxLobbyCards = 4;", self.header)
        self.assertIn("for (int32 i = 0; i < kMaxLobbyCards; ++i)", self.lobby)
        loop = self.lobby[self.lobby.index("for (int32 i = 0; i < kMaxLobbyCards"):self.lobby.index("GapAfter(Box->AddChild(CardRow))")]
        order = [loop.index(k) for k in ("Avatar", "ETTAFont::Body", "ETTAAsset::IconReady")]
        self.assertEqual(order, sorted(order), "avatar, then name, then ready tick")
        for arr in ("PlayerCards", "CardAvatars", "CardNames", "CardReady"):
            self.assertIn(f"{arr}.Add(", loop)
        self.assertNotIn("Hat: %s", self.lobby, "no text-line customization in the lobby screen")

    def test_set_player_cards_hides_extra_and_drives_start(self):
        fill = body(self.cpp, "void UTTAMenuWidget::SetPlayerCards(")
        self.assertIn("InPlayers.IsValidIndex(i)", fill)
        self.assertIn("ESlateVisibility::Hidden", fill)
        self.assertIn("SetPlayers(InPlayers.Num(), Ready)", fill)
        players = body(self.cpp, "void UTTAMenuWidget::SetPlayers(")
        self.assertIn("StartButton->SetIsEnabled(CanStart())", players)
        self.assertIn("bHost && StartBlockedReason.IsEmpty()", self.header)
        self.assertRegex(self.lobby, r'StartButton = MenuButton\(Bottom, TEXT\("START"\), ETTAButtonKind::Primary\)')
        self.assertEqual(len(re.findall(r"ETTAButtonKind::Primary", self.lobby)), 1, "Start is the only Primary")

    def test_customize_toggles_pickers(self):
        self.assertIn("OnCustomizeClicked", self.lobby)
        self.assertIn("UFUNCTION() void OnCustomizeClicked();", self.header)
        self.assertIn("CustomizePanel->SetVisibility(ESlateVisibility::Collapsed)", self.lobby)


if __name__ == "__main__":
    unittest.main()
