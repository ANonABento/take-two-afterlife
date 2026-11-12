#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "TTACinema.h"
#include "tta/CutPlayback.h"

#include <set>

#if WITH_DEV_AUTOMATION_TESTS

// Heaven cinema (task 22): 4 couch seats face the screen; a final-round premiere of
// 4 cuts plays them in a seeded random order, each as intro -> playback -> 10 s rating
// -> reveal, rejects self-ratings, and ends on the podium.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTACinemaSequenceTest, "TTA.Cinema.Sequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTACinemaSequenceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ATTACinema* Cinema = World->SpawnActor<ATTACinema>();
	if (!TestNotNull(TEXT("cinema spawns"), Cinema)) return false;

	TestTrue(TEXT("cloud is walkable"), Cinema->Cloud->IsCollisionEnabled());
	std::set<int32> SeatYs;
	for (int32 Seat = 0; Seat < ATTACinema::NumSeats; ++Seat)
	{
		const FTransform T = Cinema->GetSeatTransform(Seat);
		SeatYs.insert(FMath::RoundToInt(T.GetLocation().Y));
		TestTrue(FString::Printf(TEXT("seat %d faces the screen"), Seat), T.GetRotation().GetForwardVector().X > 0.9f);
	}
	TestEqual(TEXT("4 distinct seats"), static_cast<int32>(SeatYs.size()), 4);

	tta::Replay Replay({0, 1});
	const std::vector<tta::ReplayPose> Poses(2);
	for (int f = 0; f <= 40 * tta::kReplayHz; ++f) Replay.addFrame(Poses);
	Replay.addEvent({tta::ReplayEventType::Hit, 8000, 1, 3, 0});
	Replay.addEvent({tta::ReplayEventType::Wipeout, 8600, 1, 3, 0});
	const tta::EditList Edit = tta::autoEdit(Replay, {0, 1}, 2);

	TArray<FTTACinemaCut> Cuts;
	for (int32 i = 0; i < 4; ++i) Cuts.Add({100 + i, tta::Cut{i, (i + 1) % 4}, (i + 2) % 4, Edit, FString()});
	TArray<tta::PremiereStep> Steps;
	Cinema->OnStep.AddLambda([&Steps](tta::PremiereStep S) { Steps.Add(S); });
	Cinema->StartPremiere(Cuts, 42, true, {2, 0, 3, 1});

	std::set<int> Seen;
	bool bScreenShown = false, bIntroOk = true, bSelfRejected = true;
	int32 RatingTicks = 0;
	for (int32 Tick = 0; Tick < 30 * 60 * 5 && Cinema->GetPremiere()->step() != tta::PremiereStep::Podium; ++Tick)
	{
		const tta::Premiere& P = *Cinema->GetPremiere();
		Seen.insert(P.current().cutId);
		if (P.step() == tta::PremiereStep::Intro)
			bIntroOk &= Cinema->GetIntroLine().ToString().Contains(FString::Printf(TEXT("Player %d"), P.current().cut.editor));
		if (P.step() == tta::PremiereStep::Playback) bScreenShown |= Cinema->GetScreenFrame().IsSet();
		if (P.step() == tta::PremiereStep::Rating)
		{
			if (RatingTicks++ % 300 == 0)
			{
				bSelfRejected &= !Cinema->Rate(P.current().cut.editor, 5, 5);
				const int32 Outsider = (P.current().cut.builder + 1) % 4;
				Cinema->Rate(Outsider, 3, 4);
			}
		}
		Cinema->Step(1.f / 30.f);
	}

	TestEqual(TEXT("ends on the podium"), Cinema->GetPremiere()->step(), tta::PremiereStep::Podium);
	TestEqual(TEXT("podium shown"), Cinema->GetPodium(), TArray<int32>({2, 0, 3, 1}));
	TestEqual(TEXT("every cut premiered once"), static_cast<int32>(Seen.size()), 4);
	TestTrue(TEXT("narrator names the director"), bIntroOk);
	TestTrue(TEXT("cut on the big screen"), bScreenShown);
	TestTrue(TEXT("self-rating rejected"), bSelfRejected);
	TestTrue(TEXT("rating screen ~10 s per cut"), FMath::Abs(RatingTicks - 4 * 300) <= 8);
	TestEqual(TEXT("steps: 4 x (intro, playback, rating, reveal) + podium"), Steps.Num(), 17);
	for (const tta::PremiereCut& C : Cinema->GetPremiere()->order())
		TestTrue(FString::Printf(TEXT("cut %d revealed with votes"), C.cutId), Cinema->GetPremiere()->ratings().averages(C.cutId).votes > 0);
	return true;
}

#endif
