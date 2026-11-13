#include "Misc/AutomationTest.h"

#include "Misc/Paths.h"
#include "TTACutPlayer.h"
#include "TTAReels.h"
#include "tta/CutPlayback.h"

#if WITH_DEV_AUTOMATION_TESTS

// Editor playback (task 21): the auto-edit of a fixture Run, played through the
// editor's cut player at 60 fps, lasts exactly as long as the Core edit list says,
// visits every clip's camera in order, and survives a Reels autosave round trip.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAEditorPlayCutTest, "TTA.Editor.PlayCut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAEditorPlayCutTest::RunTest(const FString& Parameters)
{
	tta::Replay Replay({0, 1});
	const std::vector<tta::ReplayPose> Poses(2);
	for (int f = 0; f <= 90 * tta::kReplayHz; ++f) Replay.addFrame(Poses);
	Replay.addEvent({tta::ReplayEventType::Hit, 8000, 1, 3, 0});
	Replay.addEvent({tta::ReplayEventType::Wipeout, 8600, 1, 3, 0});
	Replay.addEvent({tta::ReplayEventType::Hit, 30000, 0, 5, 1});
	Replay.addEvent({tta::ReplayEventType::Smite, 89000, 0, tta::kNoTrap, tta::kNoTrap});
	const tta::EditList Cut = tta::autoEdit(Replay, {0, 1}, 2);
	if (!TestTrue(TEXT("auto-edit made clips"), Cut.clips().size() > 1)) return false;

	UTTACutPlayer* Player = NewObject<UTTACutPlayer>();
	Player->SetCut(Cut);
	Player->Play();
	TArray<int32> CamerasSeen;
	int32 Ticks = 0;
	do
	{
		const TOptional<tta::CutFrame> Frame = Player->GetFrame();
		if (Frame && (CamerasSeen.IsEmpty() || CamerasSeen.Num() <= Frame->clip)) CamerasSeen.Add(Frame->camera);
		++Ticks;
	} while (Player->Advance(1.f / 60.f) && Ticks < 60 * 60);

	TestFalse(TEXT("stopped at the end"), Player->IsPlaying());
	TestEqual<int64>(TEXT("played length = Core edit list length (ms)"), FMath::RoundToInt(Player->GetPlayedMs()), static_cast<int32>(Cut.durationMs()));
	TestTrue(TEXT("within the 30 s cap"), Cut.durationMs() <= tta::kMaxCutMs);
	TestEqual(TEXT("every clip shown"), CamerasSeen.Num(), static_cast<int32>(Cut.clips().size()));
	for (int32 i = 0; i < CamerasSeen.Num() && i < static_cast<int32>(Cut.clips().size()); ++i)
	{
		TestEqual(FString::Printf(TEXT("clip %d camera"), i), CamerasSeen[i], Cut.clips()[i].camera);
	}
	TestFalse(TEXT("no frame past the end"), Player->GetFrame().IsSet());

	const FString Path = FPaths::AutomationTransientDir() / TEXT("PlayCut.ttacut");
	if (!TestTrue(TEXT("reel saved"), TTAReels::Save(Cut, Path))) return false;
	const TOptional<tta::EditList> Back = TTAReels::Load(Path);
	if (!TestTrue(TEXT("reel loaded"), Back.IsSet())) return false;
	TestEqual(TEXT("reel length"), static_cast<int32>(Back->durationMs()), static_cast<int32>(Cut.durationMs()));
	return true;
}

#endif
