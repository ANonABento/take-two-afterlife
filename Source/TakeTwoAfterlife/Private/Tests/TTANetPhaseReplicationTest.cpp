#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "TTAGameMode.h"
#include "TTAGameState.h"

#if WITH_DEV_AUTOMATION_TESTS

// Plan req 9 (phase part): the server's phase lives in replicated RepNotify
// properties of ATTAGameState, only legal transitions reach it, and a client
// that receives the values is notified. The live 2-PC code join is a manual check.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTANetPhaseReplicationTest, "TTA.Net.PhaseReplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTTANetPhaseReplicationTest::RunTest(const FString& Parameters)
{
	const TCHAR* RepNames[] = {TEXT("Phase"), TEXT("Round")};
	for (const TCHAR* Name : RepNames)
	{
		const FProperty* Prop = ATTAGameState::StaticClass()->FindPropertyByName(Name);
		if (!TestNotNull(FString::Printf(TEXT("%s exists"), Name), Prop)) return false;
		TestTrue(FString::Printf(TEXT("%s replicates"), Name), Prop->HasAnyPropertyFlags(CPF_Net));
		TestTrue(FString::Printf(TEXT("%s has RepNotify"), Name), Prop->HasAnyPropertyFlags(CPF_RepNotify));
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	ATTAGameMode* Mode = World->SpawnActor<ATTAGameMode>();
	if (!TestNotNull(TEXT("game mode"), Mode)) return false;
	ATTAGameState* Server = Mode->GetGameState<ATTAGameState>();
	if (!TestNotNull(TEXT("game mode spawned ATTAGameState"), Server)) return false;
	TestTrue(TEXT("game state replicates"), Server->GetIsReplicated());

	TestFalse(TEXT("Lobby -> Run rejected"), Mode->RequestPhase(ETTAPhase::Run));
	TestEqual(TEXT("rejected leaves Lobby"), Server->GetPhase(), ETTAPhase::Lobby);
	TestTrue(TEXT("Lobby -> Build"), Mode->RequestPhase(ETTAPhase::Build));
	TestEqual(TEXT("server phase Build"), Server->GetPhase(), ETTAPhase::Build);
	TestEqual(TEXT("server round 1"), Server->GetRound(), 1);

	// Client copy: write the replicated values the way the net driver does, then fire the RepNotify.
	ATTAGameState* Client = World->SpawnActor<ATTAGameState>();
	ETTAPhase SeenPhase = ETTAPhase::Lobby;
	int32 SeenRound = -1;
	int32 Calls = 0;
	Client->OnPhaseChangedNative.AddLambda([&](ETTAPhase Phase, int32 Round)
	{
		SeenPhase = Phase;
		SeenRound = Round;
		++Calls;
	});
	for (const TCHAR* Name : RepNames)
	{
		ATTAGameState::StaticClass()->FindPropertyByName(Name)->CopyCompleteValue_InContainer(Client, Server);
	}
	Client->ProcessEvent(Client->FindFunctionChecked(TEXT("OnRep_Phase")), nullptr);
	TestEqual(TEXT("client sees Build"), SeenPhase, ETTAPhase::Build);
	TestEqual(TEXT("client sees round 1"), SeenRound, 1);
	TestEqual(TEXT("client notified once"), Calls, 1);
	return true;
}

#endif
