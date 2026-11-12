import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import export_devlog  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]


class ExportDevlogTests(unittest.TestCase):
    def test_repo_devlog_covers_every_plan_task(self):
        plan = (ROOT / "plan.md").read_text(encoding="utf-8")
        devlog = (ROOT / "DEVLOG.md").read_text(encoding="utf-8")
        self.assertIn("26", export_devlog.plan_tasks(plan))
        self.assertIn("11b", export_devlog.plan_tasks(plan))
        self.assertEqual(export_devlog.missing_entries(plan, devlog), [])

    def test_missing_entry_is_reported(self):
        plan = "## Tasks\n1. [code] A\n2. [code] B\n\n## Next\n"
        self.assertEqual(export_devlog.missing_entries(plan, "## Task 1 \u2014 A\n"), ["2"])

    def test_html(self):
        out = export_devlog.to_html("## Task 1 \u2014 A\n- **Prompt:** use `x<y`\n")
        self.assertIn("<h2>Task 1 \u2014 A</h2>", out)
        self.assertIn("<li><strong>Prompt:</strong> use <code>x&lt;y</code></li>", out)
        self.assertTrue(out.rstrip().endswith("</ul>"))


if __name__ == "__main__":
    unittest.main()
