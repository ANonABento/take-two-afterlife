import pathlib
import unittest

PAGE = pathlib.Path(__file__).resolve().parents[2] / "Docs" / "ITCH_PAGE.md"
SECTIONS = ["## Pitch", "## Short description", "## Long description", "## Controls",
            "## How to play", "## Tools used", "## Team credits"]


class ItchPageTest(unittest.TestCase):
    def test_sections_present(self):
        text = PAGE.read_text(encoding="utf-8")
        for s in SECTIONS:
            self.assertIn(s, text)

    def test_required_content(self):
        text = PAGE.read_text(encoding="utf-8")
        for word in ["Director's Cut", "Rounds", "Traps", "Replay", "Premiere",
                     "Unreal Engine 5.5", "Epic Online Services", "Cursor + Grok",
                     "KayKit", "Kenney", "Meshy"]:
            self.assertIn(word, text)


if __name__ == "__main__":
    unittest.main()
