import re, unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
def read(p): return (ROOT / p).read_text(encoding="utf-8-sig")
SRC = "Source/TakeTwoAfterlife/Private/"

# Plan 3 task 2: bots in the Play Solo round. UE can't run here, so these lock the source.
class BotsRound(unittest.TestCase):
    def test_bot_wants_player_state(self):
        ctor = re.search(r"ATTABotController::ATTABotController\(\)\s*\{(.*?)\}", read(SRC + "TTABotController.cpp"), re.S).group(1)
        self.assertIn("bWantsPlayerState = true", ctor)

    def test_mic_only_records_human_players(self):
        # AI pawns count as locally controlled on the server; bots must not open the mic.
        s = read(SRC + "TTAMicRecorder.cpp")
        on = re.search(r"void UTTAMicRecorder::OnPhaseChanged.*?\n\}", s, re.S).group(0)
        self.assertIn("IsPlayerControlled()", on)

    def test_mic_id_taken_when_capture_starts(self):
        # EnterRun unpossesses then destroys the lobby runner; its PlayerState is gone by EndPlay.
        s = read(SRC + "TTAMicRecorder.cpp")
        start = re.search(r"void UTTAMicRecorder::StartCapture.*?\n\}", s, re.S).group(0)
        stop = re.search(r"void UTTAMicRecorder::StopAndSave.*?\n\}", s, re.S).group(0)
        self.assertIn("RecordingPlayerId", start)
        self.assertNotIn("GetPlayerState", stop)

    def test_straight_line_steering_no_navmesh(self):
        s = read(SRC + "TTABotController.cpp")
        self.assertNotRegex(s, r"MoveTo|NavigationSystem|FindPathTo")
        self.assertIn("AddMovementInput(FVector(Step.dirX, Step.dirY", s)

    def test_bot_steering_is_logged(self):
        self.assertIn("LogTTABots", read(SRC + "TTABotController.cpp"))

    def test_run_ends_into_edit_and_bots_get_cuts(self):
        s = read(SRC + "TTAGameMode.cpp")
        self.assertRegex(s, r"AllFinished\(\)\s*\|\|\s*TimedOutFor >= SmiteHoldSeconds\) EndRun\(\)")
        end = re.search(r"void ATTAGameMode::EndRun\(\).*?\n\}", s, re.S).group(0)
        self.assertIn("FillCut(", end)
        self.assertIn("StopRun()", end)
        self.assertIn("RequestPhase(ETTAPhase::Edit)", end)

    def test_devlog_lists_bots_command(self):
        self.assertIn("Automation RunTests TTA.Bots", read("DEVLOG.md"))
