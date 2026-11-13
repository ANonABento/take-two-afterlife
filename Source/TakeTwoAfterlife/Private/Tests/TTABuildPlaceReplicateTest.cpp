#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/ScopeExit.h"
#include "TTABuildPawn.h"
#include "TTAGameMode.h"
#include "TTAPlacementMarker.h"

#if WITH_DEV_AUTOMATION_TESTS

// Plan req 2 in the world: Build gives each player a free-fly build pawn; placements snap,
// rotate, respect budget/camera cap/bounds and settings ranges; a placement replicates only
// to its owner until Run, then to everyone. The live 2-PC hop is a manual check.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTABuildPlaceReplicateTest, "TTA.Build.PlaceReplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTTABuildPlaceReplicateTest::RunTest(const FString& Parameters)
{
	const FProperty* DataProp = ATTAPlacementMarker::StaticClass()->FindPropertyByName(TEXT("Data"));
	if (!TestNotNull(TEXT("marker Data exists"), DataProp)) return false;
	TestTrue(TEXT("Data replicates"), DataProp->HasAnyPropertyFlags(CPF_Net));
	TestTrue(TEXT("Data has RepNotify"), DataProp->HasAnyPropertyFlags(CPF_RepNotify));
	TestEqual(TEXT("10 build items (6 traps + 4 cameras)"), TTABuildItem::Count(), 10);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UGameInstance* Instance = NewObject<UGameInstance>();
	World->SetGameInstance(Instance);
	World->GetWorldSettings()->DefaultGameMode = ATTAGameMode::StaticClass();
	if (!TestTrue(TEXT("game mode starts"), World->SetGameMode(FURL()))) return false;
	World->InitializeActorsForPlay(FURL());
	ATTAGameMode* Mode = Cast<ATTAGameMode>(World->GetAuthGameMode());
	if (!TestNotNull(TEXT("game mode"), Mode)) return false;
	APlayerController* A = World->SpawnActor<APlayerController>();
	APlayerController* B = World->SpawnActor<APlayerController>();
	for (APlayerController* PC : {A, B})
	{
		if (!PC->PlayerState) PC->InitPlayerState();
		if (!TestNotNull(TEXT("player state"), PC->PlayerState.Get())) return false;
	}
	A->PlayerState->SetPlayerId(1);
	B->PlayerState->SetPlayerId(2);

	const int32 Hammer = 0, Cannon = 1, FirstCamera = 6;
	TestEqual(TEXT("no placing in Lobby"), Mode->PlaceItem(A, Hammer, FVector(1000, 0, 0), 0.f), ETTAPlaceOutcome::WrongPhase);
	TestTrue(TEXT("Lobby -> Build"), Mode->RequestPhase(ETTAPhase::Build));
	TestNotNull(TEXT("A flies a build pawn"), Cast<ATTABuildPawn>(A->GetPawn()));
	TestNotNull(TEXT("B flies a build pawn"), Cast<ATTABuildPawn>(B->GetPawn()));

	// Snap 0.5 m + rotate 45 degrees.
	int32 Id = 0;
	TestEqual(TEXT("A places a hammer"), Mode->PlaceItem(A, Hammer, FVector(1037, 110, 0), 100.f, &Id), ETTAPlaceOutcome::Ok);
	ATTAPlacementMarker* Marker = Mode->FindMarker(Id);
	if (!TestNotNull(TEXT("marker spawned"), Marker)) return false;
	TestTrue(TEXT("marker replicates"), Marker->GetIsReplicated());
	TestTrue(TEXT("snapped location"), Marker->GetActorLocation().Equals(FVector(1050, 100, 0), 0.01));
	TestEqual(TEXT("snapped yaw"), Marker->GetActorRotation().Yaw, 90.0, 0.01);
	TestEqual(TEXT("budget spent"), Mode->GetLayout().remaining(1), tta::kBudget - tta::trapSpec(tta::TrapType::Hammer).cost);

	// Hidden from others until Run.
	TestTrue(TEXT("relevant to owner"), Marker->IsNetRelevantFor(A, A, FVector::ZeroVector));
	TestFalse(TEXT("hidden from B"), Marker->IsNetRelevantFor(B, B, FVector::ZeroVector));

	// 2-setting panel.
	TestEqual(TEXT("setting 1 -> 5"), Mode->SetTrapSetting(A, Id, 0, 5), ETTAPlaceOutcome::Ok);
	TestEqual(TEXT("setting 2 -> 1"), Mode->SetTrapSetting(A, Id, 1, 1), ETTAPlaceOutcome::Ok);
	TestEqual(TEXT("out of range"), Mode->SetTrapSetting(A, Id, 0, 9), ETTAPlaceOutcome::BadSetting);
	TestEqual(TEXT("B can't tune A's trap"), Mode->SetTrapSetting(B, Id, 0, 1), ETTAPlaceOutcome::NotOwner);
	TestEqual(TEXT("B can't remove A's trap"), Mode->RemovePlacement(B, Id), ETTAPlaceOutcome::NotOwner);
	TestEqual(TEXT("marker carries setting 1"), Marker->GetData().Setting0, 5);
	TestEqual(TEXT("marker carries setting 2"), Marker->GetData().Setting1, 1);

	// Bounds, budget, items.
	TestEqual(TEXT("out of corridor"), Mode->PlaceItem(A, Hammer, FVector(-100, 0, 0), 0.f), ETTAPlaceOutcome::OutOfBounds);
	TestEqual(TEXT("bad item"), Mode->PlaceItem(A, 99, FVector(500, 0, 0), 0.f), ETTAPlaceOutcome::BadItem);
	for (int32 i = 0; i < 4; ++i) TestEqual(TEXT("B cannon"), Mode->PlaceItem(B, Cannon, FVector(500.f * i, 0, 0), 0.f), ETTAPlaceOutcome::Ok);
	TestEqual(TEXT("B over budget"), Mode->PlaceItem(B, Hammer, FVector(3000, 0, 0), 0.f), ETTAPlaceOutcome::OverBudget);

	// Cameras by type, capped at 12 per type, lifted off the floor.
	int32 CamId = 0;
	for (int32 i = 0; i < tta::kCameraCap; ++i)
	{
		TestEqual(TEXT("A camera"), Mode->PlaceItem(A, FirstCamera + i % 4, FVector(100.f * i, -300, 0), 0.f, &CamId), ETTAPlaceOutcome::Ok);
	}
	const int32 MoreFixed = tta::kCameraCap - tta::kCameraCap / 4;
	for (int32 i = 0; i < MoreFixed; ++i)
	{
		TestEqual(TEXT("A fixed camera"), Mode->PlaceItem(A, FirstCamera, FVector(100.f * i, 300, 0), 0.f), ETTAPlaceOutcome::Ok);
	}
	TestEqual(TEXT("other types still open"), Mode->GetLayout().cameraCount(1, tta::CameraType::Pan), tta::kCameraCap / 4);
	TestEqual(TEXT("camera cap"), Mode->PlaceItem(A, FirstCamera, FVector(2000, 0, 0), 0.f), ETTAPlaceOutcome::CameraCapReached);
	TestEqual(TEXT("3 trap-cams"), Mode->GetLayout().cameraCount(1, tta::CameraType::TrapCam), 3);
	TestEqual(TEXT("last is a trap-cam"), Mode->FindMarker(CamId)->GetData().Item, FirstCamera + 3);
	TestEqual(TEXT("camera height"), Mode->FindMarker(CamId)->GetActorLocation().Z, (double)GetDefault<ATTAPlacementMarker>()->CameraHeightCm, 0.01);

	// Client copy: write the replicated value the way the net driver does, then RepNotify.
	ATTAPlacementMarker* Client = World->SpawnActor<ATTAPlacementMarker>();
	DataProp->CopyCompleteValue_InContainer(Client, Marker);
	Client->ProcessEvent(Client->FindFunctionChecked(TEXT("OnRep_Data")), nullptr);
	TestTrue(TEXT("client marker at the placement"), Client->GetActorTransform().Equals(Marker->GetActorTransform(), 0.01));
	TestEqual(TEXT("client sees setting"), Client->GetData().Setting0, 5);

	// Run reveals everything to everyone; no more building.
	TestTrue(TEXT("Build -> Run"), Mode->RequestPhase(ETTAPhase::Run));
	TestTrue(TEXT("revealed to B"), Marker->IsNetRelevantFor(B, B, FVector::ZeroVector));
	TestTrue(TEXT("marker data revealed"), Marker->GetData().bRevealed);
	TestEqual(TEXT("B sees all placements"), static_cast<int32>(Mode->GetLayout().visibleTo(2).size()), 1 + 4 + tta::kCameraCap + MoreFixed);
	TestEqual(TEXT("no placing in Run"), Mode->PlaceItem(A, Hammer, FVector(1000, 0, 0), 0.f), ETTAPlaceOutcome::WrongPhase);
	return true;
}

#endif
