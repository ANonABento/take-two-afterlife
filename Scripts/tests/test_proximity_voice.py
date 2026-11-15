import configparser
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def ini(name):
    p = configparser.ConfigParser(strict=False, interpolation=None)
    p.optionxform = str
    p.read(ROOT / "Config" / name, encoding="utf-8")
    return p


class ProximityVoiceTest(unittest.TestCase):
    def test_engine_voice_keys(self):
        e = ini("DefaultEngine.ini")
        self.assertEqual(e["OnlineSubsystem"]["bHasVoiceEnabled"].lower(), "true")
        self.assertEqual(e["OnlineSubsystem"]["DefaultPlatformService"].lower(), "null")
        self.assertEqual(e["Voice"]["bEnabled"].lower(), "true")

    def test_open_mic(self):
        g = ini("DefaultGame.ini")
        self.assertEqual(g["/Script/Engine.GameSession"]["bRequiresPushToTalk"].lower(), "false")

    def test_attenuation_radii(self):
        pawn = ini("DefaultGame.ini")["/Script/TakeTwoAfterlife.TTARunnerPawn"]
        self.assertEqual(float(pawn["VoiceFullVolumeCm"]), 500.0)
        self.assertEqual(float(pawn["VoiceSilentCm"]), 2500.0)
        src = (ROOT / "Source/TakeTwoAfterlife/Private/TTARunnerPawn.cpp").read_text(encoding="utf-8")
        self.assertIn("AttenuationShapeExtents = FVector(VoiceFullVolumeCm", src)
        self.assertIn("FalloffDistance = VoiceSilentCm - VoiceFullVolumeCm", src)

    def test_mute_key_unique_and_leaves_mic_recorder_alone(self):
        text = (ROOT / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="ToggleVoiceMute"', text)
        keys = re.findall(r'\+ActionMappings=\(ActionName="(\w+)".*?Key=(\w+)\)', text)
        mute_key = dict(keys)["ToggleVoiceMute"]
        self.assertEqual([a for a, k in keys if k == mute_key], ["ToggleVoiceMute"])
        src = (ROOT / "Source/TakeTwoAfterlife/Private/TTARunnerPawn.cpp").read_text(encoding="utf-8")
        body = src.split("void ATTARunnerPawn::ToggleVoiceMute()")[1].split("\n}")[0]
        self.assertIn("ToggleSpeaking", body)
        self.assertNotIn("MicRecorder", body)


if __name__ == "__main__":
    unittest.main()
