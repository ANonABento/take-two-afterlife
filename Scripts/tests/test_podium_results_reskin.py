import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "Source" / "TakeTwoAfterlife" / "Private"


class PodiumResultsReskin(unittest.TestCase):
    def setUp(self):
        self.podium = (SRC / "TTAPodiumWidget.cpp").read_text()
        self.results = (SRC / "TTAResultsWidget.cpp").read_text()

    def test_built_from_style_builders_only(self):
        for name, code in (("podium", self.podium), ("results", self.results)):
            self.assertIn("UTTAUIStyle::MakePanel(", code, name)
            self.assertIn("UTTAUIStyle::MakeLabel(", code, name)
            self.assertNotRegex(code, r"UTTAUIStyle::(AddLabel|AddCard)\(", f"{name}: legacy builder")
            self.assertNotRegex(code, r"UTTAUIStyle::k(H1|H2|Body|Small)\b", f"{name}: raw font size")

    def test_banner_and_results_content(self):
        self.assertIn("UTTAUIStyle::MakeBanner(", self.podium)
        self.assertIn('"AND THE WINNER IS..."', self.podium)
        self.assertIn('"RESULTS"', self.results)
        self.assertIn('"{0} pts"', self.results)
        self.assertIn("i + 1", self.results)

    def test_no_buttons(self):
        for name, code in (("podium", self.podium), ("results", self.results)):
            self.assertNotRegex(code, r"MakeButton|UButton|OnClicked", name)


if __name__ == "__main__":
    unittest.main()
