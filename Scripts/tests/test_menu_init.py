import pathlib, unittest

SRC = pathlib.Path(__file__).resolve().parents[2] / "Source/TakeTwoAfterlife/Private"


def body(text, sig):
    start = text.index(sig)
    i = text.index("{", start)
    depth = 0
    for j in range(i, len(text)):
        depth += {"{": 1, "}": -1}.get(text[j], 0)
        if depth == 0:
            return text[i:j + 1]
    raise AssertionError(sig)


class MenuInitTests(unittest.TestCase):
    """TTA.UI.MenuFlow crashed inside Menu->Initialize in a bare automation world."""

    def setUp(self):
        self.cpp = (SRC / "TTAMenuWidget.cpp").read_text()

    def test_initialize_does_not_touch_video_settings(self):
        init = body(self.cpp, "void UTTAMenuWidget::NativeOnInitialized()")
        self.assertNotIn("RefreshVideo()", init)

    def test_video_helpers_have_no_hard_asserts(self):
        for sig in ("void UTTAMenuWidget::RefreshVideo()", "void UTTAMenuWidget::ApplyWindowMode(",
                    "bool UTTAMenuWidget::ApplyResolution("):
            self.assertNotIn("check(Video)", body(self.cpp, sig), sig)

    def test_settings_screen_refreshes_video(self):
        self.assertIn("RefreshVideo()", body(self.cpp, "bool UTTAMenuWidget::Open("))

    def test_automation_local_player_belongs_to_game_instance(self):
        t = (SRC / "Tests/TTAUIMenuFlowTest.cpp").read_text()
        self.assertNotIn("NewObject<ULocalPlayer>(GEngine", t)


if __name__ == "__main__":
    unittest.main()
