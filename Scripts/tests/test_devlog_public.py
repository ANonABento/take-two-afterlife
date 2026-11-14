import unittest
from pathlib import Path

DOC = Path(__file__).resolve().parents[2] / "Docs" / "DEVLOG_PUBLIC.md"
SECTIONS = [
    "## The concept",
    "## How it fits \"Director's Cut\"",
    "## Plan v3 timeline",
    "## Core / Unreal split",
    "## Multi-agent workflow",
    "## What went wrong and how it was fixed",
    "### Shipping link error",
    "### MSVC fixes",
    "### Automation crashes",
]


class PublicDevlogTest(unittest.TestCase):
    def test_has_sections_and_screenshot_todos(self):
        text = DOC.read_text(encoding="utf-8")
        for s in SECTIONS:
            self.assertIn(s, text)
        self.assertIn("TODO(screenshot)", text)


if __name__ == "__main__":
    unittest.main()
