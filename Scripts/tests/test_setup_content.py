import configparser, re, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PS1 = (ROOT / "Scripts" / "setup_content.ps1").read_text()
ARTIFACT_KEYS = ("ClientId", "ClientSecret", "ProductId", "SandboxId", "DeploymentId")


def markers():
    return dict(re.findall(r'"(\w+\.py)"\s*=\s*\'([^\']+)\'', PS1))


class SetupContentTests(unittest.TestCase):
    def test_exit_code_alone_does_not_fail(self):
        # UnrealEditor-Cmd exits 1 on any LogX: Error (e.g. EOS with no Dev Portal IDs)
        self.assertNotRegex(PS1, r"\$code -ne 0\)\s*\{\s*throw")
        self.assertIn("LogPython: Error", PS1)
        self.assertIn("Traceback", PS1)

    def test_every_script_prints_its_success_marker(self):
        m = markers()
        self.assertEqual(sorted(m), sorted(["create_toon_assets.py", "build_backyard_map.py",
                                            "swap_to_toon.py", "create_title_map.py",
                                            "create_placeholder_audio.py"]))
        for script, pattern in m.items():
            src = (ROOT / "Scripts" / script).read_text()
            logged = [re.sub(r"\{[^}]*\}", "1", s)
                      for s in re.findall(r'unreal\.log\(f?"([^"]*)"\)', src)]
            self.assertTrue(any(re.search(pattern, "LogPython: " + s) for s in logged),
                            f"{script} never logs a line matching {pattern!r}")
        self.assertFalse(re.search(markers()["create_toon_assets.py"],
                                   "LogOnline: Error: EOS: UEOSSettings::GetSelectedArtifactSettings"))

    def test_eos_not_default_without_artifact_ids(self):
        ini = (ROOT / "Config" / "DefaultEngine.ini").read_text()
        svc = re.search(r"^DefaultPlatformService=(\w+)", ini, re.M).group(1)
        ids = {k: v.strip('" ') for k, v in re.findall(r"(%s)=([^,)\n]*)" % "|".join(ARTIFACT_KEYS), ini)}
        if not all(ids.get(k) for k in ARTIFACT_KEYS):
            self.assertNotEqual(svc, "EOS", "EOS selected but its Artifact IDs are empty")


if __name__ == "__main__":
    unittest.main()
