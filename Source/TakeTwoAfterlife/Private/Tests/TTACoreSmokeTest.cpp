#include "Misc/AutomationTest.h"

#include "tta/MatchStateMachine.h"

#if WITH_DEV_AUTOMATION_TESTS

// Proves the UE build links Core: one full round through the state machine.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTACoreSmokeTest, "TTA.Core.Smoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTTACoreSmokeTest::RunTest(const FString& Parameters)
{
	tta::MatchSettings Settings;
	Settings.rounds = 1;
	tta::MatchStateMachine Match(Settings);

	TestFalse(TEXT("Lobby -> Run is rejected"), Match.transition(tta::Phase::Run));
	TestTrue(TEXT("Lobby -> Build"), Match.transition(tta::Phase::Build));
	TestEqual(TEXT("round 1"), Match.round(), 1);
	TestTrue(TEXT("Build -> Run"), Match.transition(tta::Phase::Run));
	TestTrue(TEXT("Run -> Edit"), Match.transition(tta::Phase::Edit));
	TestTrue(TEXT("Edit -> Premiere"), Match.transition(tta::Phase::Premiere));
	TestTrue(TEXT("last round Premiere -> Podium"), Match.transition(tta::Phase::Podium));
	TestEqual(TEXT("phase name"), FString(tta::phaseName(Match.phase())), FString(TEXT("Podium")));
	return true;
}

#endif
