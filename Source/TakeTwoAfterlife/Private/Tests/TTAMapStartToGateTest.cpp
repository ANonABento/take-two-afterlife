#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "TTAFinishGate.h"
#include "TTAKillPlane.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "tta/Placement.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
constexpr double kCm = 100.0;

template <typename T>
TArray<T*> ActorsOf(const UWorld* World)
{
	TArray<T*> Found;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (T* Typed = Cast<T>(Actor))
		{
			Found.Add(Typed);
		}
	}
	return Found;
}
}

// Task 17: the Backyard map (built by Scripts/build_backyard_map.py) has one start, one gate
// at the far end of the 60 x 8 m corridor and a kill plane under it; the gate finishes a
// runner and the kill plane sends a faller back to the start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAMapStartToGateTest, "TTA.Map.StartToGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAMapStartToGateTest::RunTest(const FString& Parameters)
{
	const double Length = tta::kCorridorLengthMeters * kCm;
	const double HalfWidth = tta::kCorridorWidthMeters * kCm / 2.0;

	if (const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Backyard.Backyard")); TestNotNull(TEXT("Backyard loads"), Map))
	{
		const TArray<APlayerStart*> Starts = ActorsOf<APlayerStart>(Map);
		const TArray<ATTAFinishGate*> Gates = ActorsOf<ATTAFinishGate>(Map);
		const TArray<ATTAKillPlane*> Kills = ActorsOf<ATTAKillPlane>(Map);
		if (TestEqual(TEXT("one start"), Starts.Num(), 1) && TestEqual(TEXT("one gate"), Gates.Num(), 1)
			&& TestEqual(TEXT("one kill plane"), Kills.Num(), 1))
		{
			const FVector Start = Starts[0]->GetActorLocation();
			const FVector Gate = Gates[0]->GetActorLocation();
			TestTrue(TEXT("start at the near end"), Start.X >= 0.0 && Start.X <= 2.0 * kCm && FMath::Abs(Start.Y) <= HalfWidth);
			TestTrue(TEXT("gate at the far end"), Gate.X >= Length - 2.0 * kCm && Gate.X <= Length && FMath::Abs(Gate.Y) <= HalfWidth);
			TestTrue(TEXT("route covers 90% of the corridor"), Gate.X - Start.X >= 0.9 * Length);

			const FBox Kill = Kills[0]->Volume->Bounds.GetBox();
			TestTrue(TEXT("kill plane below the floor"), Kill.Max.Z < Start.Z - 300.0);
			TestTrue(TEXT("kill plane covers the corridor"), Kill.Min.X < 0.0 && Kill.Max.X > Length
				&& Kill.Min.Y < -HalfWidth && Kill.Max.Y > HalfWidth);
			TestTrue(TEXT("engine KillZ below the kill plane"), Map->GetWorldSettings()->KillZ < Kill.Min.Z);
		}
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	const FVector StartAt(100.f, 0.f, 200.f);
	ATTARunnerPawn* Runner = World->SpawnActor<ATTARunnerPawn>(StartAt, FRotator::ZeroRotator);
	ATTARunnerPawn* Faller = World->SpawnActor<ATTARunnerPawn>(StartAt + FVector(0.f, 200.f, 0.f), FRotator::ZeroRotator);
	ATTARunDirector* Director = World->SpawnActor<ATTARunDirector>();
	ATTAFinishGate* Gate = World->SpawnActor<ATTAFinishGate>(FVector(Length - 100.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATTAKillPlane* Kill = World->SpawnActor<ATTAKillPlane>(FVector(Length / 2.0, 0.0, -1000.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("runner"), Runner) && TestNotNull(TEXT("faller"), Faller) && TestNotNull(TEXT("director"), Director)
		&& TestNotNull(TEXT("gate"), Gate) && TestNotNull(TEXT("kill plane"), Kill))
	{
		TestFalse(TEXT("no finish before Run"), Gate->HandleRunnerEntered(Runner));
		Director->StartRun({Runner, Faller}, 60.f);
		Runner->SetActorLocation(Gate->GetActorLocation() + FVector(0.f, 0.f, 100.f));
		TestTrue(TEXT("gate finishes the runner"), Gate->HandleRunnerEntered(Runner));
		TestTrue(TEXT("director knows"), Director->HasFinished(Runner));
		TestFalse(TEXT("faller not finished"), Director->HasFinished(Faller));

		Faller->SetActorLocation(Kill->GetActorLocation());
		TestTrue(TEXT("kill plane hits the faller"), Kill->HandleRunnerFell(Faller));
		TestFalse(TEXT("not hit twice"), Kill->HandleRunnerFell(Faller));
		Faller->AdvanceLife(10.f);
		TestTrue(TEXT("faller alive again"), Faller->GetLifeState() == tta::LifeState::Alive);
		TestTrue(TEXT("faller back at the start"), Faller->GetActorLocation().Equals(Faller->GetStartLocation(), 1.f));
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
