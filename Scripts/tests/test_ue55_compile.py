"""UE 5.5 compile guards (source-level; UE can't run in the sandbox).

Each listed engine type used in Source/ must have its header included in the same
file (IWYU, no PCH, no unity). TTAGameMode.cpp if/else pairs are balanced and no
`if (...) MACRO(...); else` (UE_LOG expands to a braced block, so the `;` orphans the
else: C2181). Build.cs lists the modules. Docs/scripts only name UE 5.5.
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "TakeTwoAfterlife"

# type -> headers that provide it (any one is enough)
HEADERS = {
    "FHitResult": ["Engine/HitResult.h"],
    "FCollisionQueryParams": ["CollisionQueryParams.h"],
    "UWorld": ["Engine/World.h"],
    "UStaticMeshComponent": ["Components/StaticMeshComponent.h"],
    "AStaticMeshActor": ["Engine/StaticMeshActor.h"],
    "UStaticMesh": ["Engine/StaticMesh.h"],
    "ULocalPlayer": ["Engine/LocalPlayer.h"],
    "UGameInstance": ["Engine/GameInstance.h"],
    "APlayerController": ["GameFramework/PlayerController.h", "TTAGameMode.h"],
    "APlayerState": ["GameFramework/PlayerState.h", "TTAPlayerState.h"],
    "AWorldSettings": ["GameFramework/WorldSettings.h"],
    "TActorIterator": ["EngineUtils.h"],
    "UGameplayStatics": ["Kismet/GameplayStatics.h"],
    "UCapsuleComponent": ["Components/CapsuleComponent.h"],
    "UCharacterMovementComponent": ["GameFramework/CharacterMovementComponent.h"],
    "USkeletalMeshComponent": ["Components/SkeletalMeshComponent.h"],
    "UBoxComponent": ["Components/BoxComponent.h"],
    "USphereComponent": ["Components/SphereComponent.h"],
    "UCameraComponent": ["Camera/CameraComponent.h"],
    "USpringArmComponent": ["GameFramework/SpringArmComponent.h"],
    "UMaterialInstanceDynamic": ["Materials/MaterialInstanceDynamic.h"],
    "UTextBlock": ["Components/TextBlock.h"],
    "UButton": ["Components/Button.h"],
    "UWidgetSwitcher": ["Components/WidgetSwitcher.h"],
    "UVerticalBox": ["Components/VerticalBox.h"],
    "UHorizontalBox": ["Components/HorizontalBox.h"],
    "UCanvasPanel": ["Components/CanvasPanel.h"],
    "UBorder": ["Components/Border.h"],
    "UImage": ["Components/Image.h"],
    "USizeBox": ["Components/SizeBox.h"],
    "UEditableTextBox": ["Components/EditableTextBox.h"],
    "USlider": ["Components/Slider.h"],
    "UProgressBar": ["Components/ProgressBar.h"],
    "UWidgetTree": ["Blueprint/WidgetTree.h"],
    "UGameUserSettings": ["GameFramework/GameUserSettings.h"],
    "UDataTable": ["Engine/DataTable.h"],
    "UCanvas": ["Engine/Canvas.h"],
    "UFont": ["Engine/Font.h"],
    "UTexture2D": ["Engine/Texture2D.h"],
    "ADirectionalLight": ["Engine/DirectionalLight.h"],
    "UDirectionalLightComponent": ["Components/DirectionalLightComponent.h"],
    "UPathFollowingComponent": ["Navigation/PathFollowingComponent.h"],
    "UNavigationSystemV1": ["NavigationSystem.h"],
}
MODULES = ["AIModule", "NavigationSystem", "UMG", "Slate", "SlateCore", "EnhancedInput", "GameplayTasks"]
FIXED = ["Private/TTABotController.cpp", "Private/TTAGameMode.cpp",
         "Private/Tests/TTABotsSoloRoundTest.cpp", "Private/Tests/TTAUIMenuFlowTest.cpp"]
DOCS = ["README.md", "DEVLOG.md", "SUBMISSION.md", "CLAUDE.md", "AGENTS.md", "OVERNIGHT.md"]


def strip(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)


def includes(path, seen=None):
    """Headers included by path, following the project's own headers."""
    seen = set() if seen is None else seen
    out = set()
    for inc in re.findall(r'#include\s+"([^"]+)"', path.read_text(encoding="utf-8-sig")):
        out.add(inc)
        own = SRC / "Public" / inc
        if own.exists() and inc not in seen:
            seen.add(inc)
            out |= includes(own, seen)
    return out


def missing(path):
    body = strip(path.read_text(encoding="utf-8-sig"))
    have = includes(path)
    if path.suffix == ".cpp":
        own = SRC / "Public" / (path.stem + ".h")
        if own.exists():
            have |= includes(own) | {own.name}
    gaps = []
    for ty, hdrs in HEADERS.items():
        if any(h in have for h in hdrs):
            continue
        uses = list(re.finditer(r"\b%s\b" % ty, body))
        if path.suffix == ".h":
            # a header may forward-declare a type it only names through a pointer or reference
            declared = re.search(r"\b(class|struct)\s+%s\s*;" % ty, body)
            uses = [u for u in uses if not re.match(r"\s*[*>]", body[u.end():])
                    and not re.search(r"\b(class|struct)\s+$", body[:u.start()])
                    and not (declared and re.match(r"\s*&", body[u.end():]))]
        if uses:
            gaps.append(ty)
    return gaps


class UE55Compile(unittest.TestCase):
    def test_fixed_files_include_their_types(self):
        for rel in FIXED:
            self.assertEqual(missing(SRC / rel), [], rel)

    def test_every_source_includes_its_types(self):
        gaps = {str(p.relative_to(SRC)): m for p in sorted(SRC.rglob("*.[ch]*"))
                if p.suffix in (".h", ".cpp") and "CoreLib" not in p.parts and (m := missing(p))}
        self.assertEqual(gaps, {})

    def test_gamemode_if_else_balanced(self):
        body = strip((SRC / "Private/TTAGameMode.cpp").read_text(encoding="utf-8-sig"))
        self.assertEqual(body.count("{"), body.count("}"))
        self.assertEqual(body.count("("), body.count(")"))
        for m in re.finditer(r"\belse\b", body):
            before = body[:m.start()].rstrip()
            self.assertTrue(before.endswith("}") or before.endswith(";"), body[m.start() - 80:m.start()])
            line = before.splitlines()[-1].strip() if before.endswith(";") else ""
            # a braced macro (UE_LOG, check, ...) plus `;` before else orphans it
            self.assertFalse(re.match(r"(else\s+)?if\s*\(.*\)\s*[A-Z_]{3,}\(.*\);$", line), line)
            if before.endswith(";"):
                self.assertRegex(line, r"^(else\s+)?if\s*\(|^else\b", line)

    def test_menu_flow_statement_not_swallowed_by_comment(self):
        for line in (SRC / "Private/Tests/TTAUIMenuFlowTest.cpp").read_text(encoding="utf-8-sig").splitlines():
            code = line.split("//")[0].strip()
            if "//" in line and code and not code.endswith((";", "{", "}", ",", ")")) and "(" in code:
                self.assertTrue(code.endswith(";"), line)

    def test_build_cs_modules(self):
        cs = (SRC / "TakeTwoAfterlife.Build.cs").read_text(encoding="utf-8-sig")
        public = re.search(r"PublicDependencyModuleNames\.AddRange\(new string\[\]\s*\{([^}]*)\}", cs).group(1)
        for mod in MODULES:
            self.assertIn('"%s"' % mod, public, mod)

    def test_uproject_and_docs_only_55(self):
        self.assertIn('"EngineAssociation": "5.5"', (ROOT / "TakeTwoAfterlife.uproject").read_text())
        files = [ROOT / d for d in DOCS if (ROOT / d).exists()]
        files += [p for p in (ROOT / "Scripts").glob("*") if p.suffix in (".bat", ".ps1", ".sh", ".cmd")]
        pat = re.compile(r"(?:UE[ _]?|Unreal Engine |Engine |UE_)(5\.\d+|4\.\d+)", re.I)
        for f in files:
            for v in pat.findall(f.read_text(encoding="utf-8-sig", errors="replace")):
                self.assertEqual(v, "5.5", f.name)
        self.assertIn(r"UE_5.5\Engine\Build\BatchFiles\Build.bat", (ROOT / "DEVLOG.md").read_text(encoding="utf-8-sig"))


if __name__ == "__main__":
    unittest.main()
