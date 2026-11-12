#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "TTATrapActor.h"
#include "tta/Placement.h"
#include "tta/Scoring.h"

#if WITH_DEV_AUTOMATION_TESTS

// Run phase end to end on the server: both runners race at once, a hammer hit
// respawns the victim at the start, the builder reaches the gate, the timer
// smites the victim, and the round scores through Core.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTARunScoreRoundTest, "TTA.Run.ScoreRound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTARunScoreRoundTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	const FVector Start(0.f, 0.f, 200.f);
	ATTARunnerPawn* Victim = World->SpawnActor<ATTARunnerPawn>(Start, FRotator::ZeroRotator);
	ATTARunnerPawn* Builder = World->SpawnActor<ATTARunnerPawn>(Start + FVector(0.f, 300.f, 0.f), FRotator::ZeroRotator);
	ATTARunDirector* Director = World->SpawnActor<ATTARunDirector>();
	ATTATrapActor* Trap = World->SpawnActor<ATTATrapActor>();
	if (TestNotNull(TEXT("victim"), Victim) && TestNotNull(TEXT("builder"), Builder)
		&& TestNotNull(TEXT("director"), Director) && TestNotNull(TEXT("trap"), Trap))
	{
		Victim->SetStartLocation(Start);
		tta::Placement Hammer;
		Hammer.id = 4;
		Hammer.owner = 1; // Builder's replay id
		Hammer.trap = tta::TrapType::Hammer;
		Hammer.settings = {tta::trapSpec(Hammer.trap).settings[0].min, tta::trapSpec(Hammer.trap).settings[1].min};
		Trap->InitFromPlacement(Hammer);

		Director->StartRun({Victim, Builder}, 3.f);
		Director->AdvanceRun(0.5f);
		TestTrue(TEXT("hammer hits"), Trap->TryHit(Victim));
		Victim->SetActorLocation(Start + FVector(1500.f, 0.f, 0.f));
		Victim->AdvanceLife(10.f);
		TestTrue(TEXT("victim respawned at the start"), Victim->GetActorLocation().Equals(Start, 1.f));
		TestTrue(TEXT("victim alive again"), Victim->GetLifeState() == tta::LifeState::Alive);

		Director->MarkFinished(Builder);
		TestFalse(TEXT("not everyone finished"), Director->AllFinished());
		TestFalse(TEXT("timer still running"), Director->IsTimedOut());
		Director->AdvanceRun(3.f);
		TestTrue(TEXT("timer ran out"), Director->IsTimedOut());

		const tta::RoundInput Round = Director->GetRoundInput();
		TestEqual(TEXT("one wipeout"), static_cast<int32>(Round.wipeouts.size()), 1);
		TestEqual(TEXT("one finisher"), static_cast<int32>(Round.finishers.size()), 1);
		std::map<int, double> Points = tta::scoreRound(Round);
		const double HammerPoints = FMath::Max(tta::kMinTrapPoints, static_cast<double>(tta::trapSpec(tta::TrapType::Hammer).cost));
		TestEqual(TEXT("builder: gate + hammer"), Points[1], tta::kGatePoints + HammerPoints);
		TestEqual(TEXT("victim: nothing"), Points[0], 0.0);

		TestFalse(TEXT("handheld down by default"), Builder->IsFilming());
		Builder->SetFilming(true);
		TestTrue(TEXT("handheld raised"), Builder->IsFilming());
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
