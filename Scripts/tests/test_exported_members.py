"""C2487 guard: members of an exported (_API) class/struct must not carry their own
dll-interface, and static data members must be one declarator per declaration
(MSVC reports C2487 on the 2nd+ declarator of `static constexpr T a = 1, b = 2;`
inside a TAKETWOAFTERLIFE_API class)."""
import re, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source"
CLASS = re.compile(r"\b(class|struct)\s+TAKETWOAFTERLIFE_API\s+\w+[^;{]*\{")


def exported_bodies():
    for f in list(SRC.rglob("*.h")) + list(SRC.rglob("*.cpp")):
        text = f.read_text(encoding="utf-8-sig")
        for m in CLASS.finditer(text):
            depth, i = 1, m.end()
            while depth and i < len(text):
                depth += {"{": 1, "}": -1}.get(text[i], 0)
                i += 1
            yield f, text[m.end():i - 1]


def strip_comments(s):
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", s, flags=re.S)


class ExportedMembers(unittest.TestCase):
    def test_no_member_has_own_api_macro(self):
        hits = [f"{f.name}: {l.strip()}" for f, body in exported_bodies()
                for l in strip_comments(body).splitlines() if "TAKETWOAFTERLIFE_API" in l]
        self.assertEqual(hits, [])

    def test_static_data_members_one_per_declaration(self):
        hits = []
        for f, body in exported_bodies():
            for stmt in re.split(r"[;{}]", strip_comments(body)):
                s = " ".join(stmt.split())
                s = re.sub(r"^(public|private|protected):\s*", "", s)
                if re.match(r"^static\s+(constexpr|const|inline)\b", s) and "(" not in s.split("=")[0] and "," in s:
                    hits.append(f"{f.name}: {s}")
        self.assertEqual(hits, [])

    def test_ui_style_sizes_unchanged(self):
        h = (SRC / "TakeTwoAfterlife/Public/TTAUIStyle.h").read_text(encoding="utf-8-sig")
        for name, val in [("kH1", "120"), ("kH2", "64"), ("kTimer", "96"), ("kBody", "32"), ("kSmall", "24"),
                          ("kDesignHeight", "1080.f"), ("kOutline", "6.f"), ("kCardPadding", "24.f")]:
            self.assertRegex(h, rf"static constexpr \w+ {name} = {re.escape(val)};")


if __name__ == "__main__":
    unittest.main()
