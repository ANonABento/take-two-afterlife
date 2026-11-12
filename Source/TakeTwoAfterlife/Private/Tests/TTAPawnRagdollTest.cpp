#include "Misc/AutomationTest.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TTARunnerPawn.h"

#if WITH_DEV_AUTOMATION_TESTS

// Req 8: hit -> frozen (victim only) -> simulating ragdoll -> respawned at start.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAPawnRagdollTest, "TTA.Pawn.Ragdoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAPawnRagdollTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	const FVector Start(0.f, 0.f, 200.f);
	ATTARunnerPawn* Pawn = World->SpawnActor<ATTARunnerPawn>(Start, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("pawn spawned"), Pawn))
	{
		USkeletalMeshComponent* Body = Pawn->GetMesh();
		TestNotNull(TEXT("RunnerMesh loaded (Config/DefaultGame.ini)"), Body->GetSkeletalMeshAsset());
		TestNotNull(TEXT("RunnerMesh has a physics asset"), Body->GetPhysicsAsset());
		TestFalse(TEXT("human form hides wings"), Pawn->Wings->IsVisible());
		TestFalse(TEXT("human form hides halo"), Pawn->Halo->IsVisible());

		Pawn->SetActorLocation(Start + FVector(800.f, 0.f, 0.f)); // ran down the course
		Pawn->Hit();
		TestTrue(TEXT("frozen"), Pawn->GetLifeState() == tta::LifeState::Frozen);
		TestEqual(TEXT("victim time stopped"), Pawn->CustomTimeDilation, 0.f);
		TestEqual(TEXT("world time not stopped"), World->GetWorldSettings()->TimeDilation, 1.f);
		TestEqual(TEXT("white flash on"), Pawn->GetFlash(), 1.f);

		Pawn->AdvanceLife(static_cast<float>(tta::kImpactFreezeSeconds) + 0.01f);
		TestTrue(TEXT("ragdoll state"), Pawn->GetLifeState() == tta::LifeState::Ragdoll);
		TestTrue(TEXT("mesh simulating physics"), Body->IsSimulatingPhysics());
		TestEqual(TEXT("flash off"), Pawn->GetFlash(), 0.f);

		Pawn->AdvanceLife(static_cast<float>(tta::kRagdollSeconds) + 0.01f);
		TestTrue(TEXT("alive again"), Pawn->GetLifeState() == tta::LifeState::Alive);
		TestFalse(TEXT("physics off"), Body->IsSimulatingPhysics());
		TestTrue(TEXT("respawned at start"), Pawn->GetActorLocation().Equals(Start, 1.f));
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
