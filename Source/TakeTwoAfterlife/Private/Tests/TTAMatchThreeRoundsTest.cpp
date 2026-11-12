#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TTAGameMode.h"
#include "TTAGameState.h"
#include "TTAPlacementMarker.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "TTATrapActor.h"

#if WITH_DEV_AUTOMATION_TESTS

// Full match on the server: host settings from JSON, three rounds of
// Build -> Run -> Edit -> Premiere, then Podium. Joining is open only between
// rounds; a player who leaves after round 1 keeps their hammer on the map.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAMatchThreeRoundsTest, "TTA.Match.ThreeRounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	APlayerController* AddPlayer(UWorld* World, int32 Id)
	{
		APlayerController* PC = World->SpawnActor<APlayerController>();
		check(PC);
		PC->InitPlayerState();
		check(PC->PlayerState);
		PC->PlayerState->SetPlayerId(Id);
		return PC;
	}

	int32 HammerItem()
	{
		for (int32 Item = 0; Item < TTABuildItem::Count(); ++Item)
		{
			if (!TTABuildItem::IsCamera(Item) && TTABuildItem::Trap(Item) == tta::TrapType::Hammer) return Item;
		}
		return INDEX_NONE;
	}

	bool JoinOpen(ATTAGameMode* Mode)
	{
		FString Error;
		Mode->PreLogin(TEXT(""), TEXT(""), FUniqueNetIdRepl(), Error);
		return Error.IsEmpty();
	}
}

bool FTTAMatchThreeRoundsTest::RunTest(const FString& Parameters)
{
	FString OldSettings;
	const bool bHadSettings = FFileHelper::LoadFileToString(OldSettings, *ATTAGameMode::SettingsPath());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->GetWorldSettings()->DefaultGameMode = ATTAGameMode::StaticClass();
	World->SetGameMode(FURL());
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	ATTAGameMode* Mode = Cast<ATTAGameMode>(World->GetAuthGameMode());
	ATTAGameState* State = World->GetGameState<ATTAGameState>();
	if (TestNotNull(TEXT("game mode"), Mode) && TestNotNull(TEXT("game state"), State))
	{
		TestTrue(TEXT("settings file written"), FPaths::FileExists(ATTAGameMode::SettingsPath()));
		TestFalse(TEXT("bad settings rejected"), Mode->ApplySettingsJson(TEXT("{\"rounds\": 0}")));
		TestTrue(TEXT("host settings applied"), Mode->ApplySettingsJson(TEXT("{\"rounds\": 3, \"buildSeconds\": 30, \"runSeconds\": 20, \"editSeconds\": 15}")));
		TestEqual(TEXT("rounds from JSON"), Mode->GetSettings().rounds, 3);
		TestEqual(TEXT("build timer from JSON"), Mode->GetSettings().buildSeconds, 30);
		TestEqual(TEXT("settings mirrored for clients"), State->GetSettingsJson(), UTF8_TO_TCHAR(tta::toJson(Mode->GetSettings()).c_str()));
		FString Saved;
		TestTrue(TEXT("settings saved on the host"), FFileHelper::LoadFileToString(Saved, *ATTAGameMode::SettingsPath()));
		TestEqual(TEXT("saved file matches"), Saved, State->GetSettingsJson());
		TestTrue(TEXT("join open in the lobby"), JoinOpen(Mode));

		APlayerController* Leaver = AddPlayer(World, 1);
		APlayerController* Stayer = AddPlayer(World, 2);
		const int32 Hammer = HammerItem();
		TestNotEqual(TEXT("hammer in the build menu"), Hammer, INDEX_NONE);

		TestTrue(TEXT("match starts"), Mode->RequestPhase(ETTAPhase::Build));
		for (int32 Round = 1; Round <= 3; ++Round)
		{
			TestEqual(TEXT("build"), State->GetPhase(), ETTAPhase::Build);
			TestEqual(TEXT("round number"), State->GetRound(), Round);
			TestFalse(TEXT("illegal skip rejected"), Mode->RequestPhase(ETTAPhase::Premiere));
			TestFalse(TEXT("no joining in build"), JoinOpen(Mode));
			if (Round == 1)
			{
				TestEqual(TEXT("leaver places a hammer"), Mode->PlaceItem(Leaver, Hammer, FVector(1000.f, 0.f, 0.f), 0.f), ETTAPlaceOutcome::Ok);
			}

			TestTrue(TEXT("run"), Mode->RequestPhase(ETTAPhase::Run));
			TestFalse(TEXT("no joining in run"), JoinOpen(Mode));
			ATTARunDirector* Director = Mode->GetRunDirector();
			ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(Stayer->GetPawn());
			if (!TestNotNull(TEXT("director"), Director) || !TestNotNull(TEXT("runner"), Runner)) break;
			if (Round > 1)
			{
				int32 Traps = 0;
				for (TActorIterator<ATTATrapActor> It(World); It; ++It)
				{
					++Traps;
					TestFalse(TEXT("leaver's trap still shown"), It->IsHidden());
					TestEqual(TEXT("leaver's trap keeps a replay id after the runner"), It->GetPlacement().owner, 1);
				}
				TestEqual(TEXT("leaver's hammer persists"), Traps, 1);
				TestEqual(TEXT("absent owner recorded"), static_cast<int32>(Director->GetReplay().players().size()), 2);
			}
			Director->MarkFinished(Runner);
			Mode->Tick(0.f); // everyone through the gate: scored, then Edit
			TestEqual(TEXT("edit after run"), State->GetPhase(), ETTAPhase::Edit);
			TestFalse(TEXT("no joining in edit"), JoinOpen(Mode));

			TestTrue(TEXT("premiere"), Mode->RequestPhase(ETTAPhase::Premiere));
			TestTrue(TEXT("join open between rounds"), JoinOpen(Mode));
			TestTrue(TEXT("premiere ends"), Mode->EndPremiere({{2, 1.0}}));
			if (Round == 1)
			{
				Mode->Logout(Leaver);
				Leaver->Destroy();
			}
		}
		TestEqual(TEXT("podium after three rounds"), State->GetPhase(), ETTAPhase::Podium);
		TestEqual(TEXT("three rounds scored"), Mode->GetScore().roundsPlayed(), 3);
		TestEqual(TEXT("stayer: gate each round + ratings"), Mode->GetScore().total(2), 3 * tta::kGatePoints + 3.0);
		TestTrue(TEXT("join open on the podium"), JoinOpen(Mode));
		TestTrue(TEXT("back to lobby"), Mode->RequestPhase(ETTAPhase::Lobby));
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	if (bHadSettings) FFileHelper::SaveStringToFile(OldSettings, *ATTAGameMode::SettingsPath());
	else IFileManager::Get().Delete(*ATTAGameMode::SettingsPath());
	return true;
}

#endif
