#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/FileHelper.h"
#include "TTABotController.h"
#include "TTACinema.h"
#include "TTAFinishGate.h"
#include "TTAGameMode.h"
#include "TTAGameState.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// Play Solo on the server: one player + 3 bots on a flat route. Bots get fixed ids, build
// within budget, run the TTARoute markers toward the gate, get an auto-edited cut when Run
// ends, and vote on someone else's cut in the cinema's rating screen.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTABotsSoloRoundTest, "TTA.Bots.SoloRound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTABotsSoloRoundTest::RunTest(const FString& Parameters)
{
	FString OldSettings;
	const bool bHadSettings = FFileHelper::LoadFileToString(OldSettings, *ATTAGameMode::SettingsPath());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->SetGameInstance(NewObject<UGameInstance>());
	World->GetWorldSettings()->DefaultGameMode = ATTAGameMode::StaticClass();
	World->SetGameMode(FURL());
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	ATTAGameMode* Mode = Cast<ATTAGameMode>(World->GetAuthGameMode());
	ATTAGameState* State = World->GetGameState<ATTAGameState>();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (TestNotNull(TEXT("game mode"), Mode) && TestNotNull(TEXT("game state"), State) && TestNotNull(TEXT("cube"), Cube))
	{
		// 40 m x 8 m floor, start at x=0, markers every 4 m, gate at 16 m.
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(1500.f, 0.f, -50.f), FRotator::ZeroRotator);
		Floor->GetStaticMeshComponent()->SetStaticMesh(Cube);
		Floor->SetActorScale3D(FVector(40.f, 8.f, 1.f));
		World->SpawnActor<APlayerStart>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
		for (float X : {400.f, 800.f, 1200.f})
		{
			AStaticMeshActor* Marker = World->SpawnActor<AStaticMeshActor>(FVector(X, 0.f, 0.f), FRotator::ZeroRotator);
			Marker->Tags.Add(ATTABotController::RouteTag);
		}
		World->SpawnActor<ATTAFinishGate>(FVector(1600.f, 0.f, 0.f), FRotator::ZeroRotator);
		TestEqual(TEXT("route = 3 markers + gate"), ATTABotController::FindRoute(World).Num(), 4);

		APlayerController* Human = World->SpawnActor<APlayerController>();
		Human->InitPlayerState();
		Human->PlayerState->SetPlayerId(1);
		TestTrue(TEXT("settings"), Mode->ApplySettingsJson(TEXT("{\"rounds\": 1, \"buildSeconds\": 30, \"runSeconds\": 20, \"editSeconds\": 15}")));

		Mode->AddBots(5);
		TArray<ATTABotController*> Bots;
		for (TActorIterator<ATTABotController> It(World); It; ++It) Bots.Add(*It);
		TestEqual(TEXT("capped at 3 bots"), Bots.Num(), ATTAGameMode::MaxBots);
		for (ATTABotController* Bot : Bots)
		{
			TestTrue(TEXT("bot id fixed"), Bot->PlayerState->GetPlayerId() >= ATTABotController::FirstBotId);
			TestNotNull(TEXT("bot has a lobby runner"), Cast<ATTARunnerPawn>(Bot->GetPawn()));
		}

		TestTrue(TEXT("build"), Mode->RequestPhase(ETTAPhase::Build));
		for (ATTABotController* Bot : Bots)
		{
			TestTrue(TEXT("bot spent build budget"), Mode->GetLayout().spent(Bot->PlayerState->GetPlayerId()) > 0);
		}
		TestTrue(TEXT("run"), Mode->RequestPhase(ETTAPhase::Run));
		TMap<ATTABotController*, float> StartX;
		for (ATTABotController* Bot : Bots)
		{
			TestTrue(TEXT("bot is steering"), Bot->IsRunning());
			StartX.Add(Bot, Bot->GetPawn()->GetActorLocation().X);
		}
		for (int32 i = 0; i < 90; ++i) World->Tick(LEVELTICK_All, 1.f / 30.f); // 3 s
		for (ATTABotController* Bot : Bots)
		{
			ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(Bot->GetPawn());
			const bool bMoved = Runner && Runner->GetActorLocation().X > StartX[Bot] + 200.f;
			TestTrue(TEXT("bot moved toward the gate or finished"), bMoved || Mode->GetRunDirector()->HasFinished(Runner));
		}
		for (int32 i = 0; i < 30 * 25 && State->GetPhase() == ETTAPhase::Run; ++i) World->Tick(LEVELTICK_All, 1.f / 30.f);
		TestTrue(TEXT("run ended into Edit"), State->GetPhase() == ETTAPhase::Edit);
		for (ATTABotController* Bot : Bots)
		{
			TestFalse(TEXT("bot stopped steering"), Bot->IsRunning());
			TestNotNull(TEXT("auto-edit filled the bot cut"), Bot->GetCut());
		}

		ATTACinema* Cinema = World->SpawnActor<ATTACinema>();
		if (TestNotNull(TEXT("cinema"), Cinema) && Bots.Num() > 0 && Bots[0]->GetCut())
		{
			FTTACinemaCut Cut;
			Cut.CutId = 1;
			Cut.Cut = tta::Cut{1, 1}; // the human's cut and trap
			Cut.Victim = 1;
			Cut.Edit = *Bots[0]->GetCut();
			Cinema->StartPremiere({Cut}, 7, false, {});
			for (int32 i = 0; i < 30 * 120 && Cinema->GetPremiere()->step() != tta::PremiereStep::Rating; ++i) Cinema->Step(1.f / 30.f);
			TestTrue(TEXT("rating screen"), Cinema->GetPremiere()->step() == tta::PremiereStep::Rating);
			for (ATTABotController* Bot : Bots) TestTrue(TEXT("bot rated the human's cut"), Bot->RateCurrent(Cinema));
			TestFalse(TEXT("no second vote"), Bots[0]->RateCurrent(Cinema));
		}
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	if (bHadSettings) FFileHelper::SaveStringToFile(OldSettings, *ATTAGameMode::SettingsPath());
	return true;
}

#endif
