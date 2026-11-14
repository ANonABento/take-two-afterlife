#include "Misc/AutomationTest.h"

#include "TTAMenuWidget.h"
#include "TTAReels.h"
#include "TTAGameMode.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// Menus (task 24): Title -> How to play -> back; Join rejects a bad code and accepts a
// typed one into the Lobby; lobby look carousels, START gate; settings clamp and mic
// meter; Reels shows what TTAReels lists. The visible switcher slot tracks the screen.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTAUIMenuFlowTest, "TTA.UI.MenuFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTAUIMenuFlowTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddInfo(TEXT("no engine; skipping"));
		return true;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!World)
	{
		AddInfo(TEXT("no world; skipping"));
		return true;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UGameInstance* Instance = NewObject<UGameInstance>(World);
	World->SetGameInstance(Instance);
	World->GetWorldSettings()->DefaultGameMode = ATTAGameMode::StaticClass();
	if (!TestTrue(TEXT("game mode starts"), World->SetGameMode(FURL()))) return false;
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, GEngine->LocalPlayerClass);
	const AGameModeBase* Mode = World->GetAuthGameMode();
	APlayerController* PC = World->SpawnActor<APlayerController>(Mode ? Mode->PlayerControllerClass.Get() : ATTAPlayerController::StaticClass());
	if (!TestNotNull(TEXT("local controller"), PC) || !TestNotNull(TEXT("local player"), LocalPlayer)) return false;
	PC->SetPlayer(LocalPlayer);

	UTTAMenuWidget* Menu = NewObject<UTTAMenuWidget>(PC);
	Menu->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
	if (!TestNotNull(TEXT("widget created"), Menu)) return false;
	if (!TestTrue(TEXT("widget initialized"), Menu->Initialize())) return false;
	auto Shows = [&](tta::MenuScreen Screen, const TCHAR* What)
	{
		TestTrue(What, Menu->GetScreen() == Screen);
		TestEqual(FString(What) + TEXT(" (visible)"), Menu->GetVisibleIndex(), static_cast<int32>(Screen));
	};
	Shows(tta::MenuScreen::Title, TEXT("starts on Title"));

	TestTrue(TEXT("open how to play"), Menu->Open(tta::MenuScreen::HowToPlay));
	Shows(tta::MenuScreen::HowToPlay, TEXT("how to play"));
	TestTrue(TEXT("back"), Menu->Back());
	TestFalse(TEXT("no back from Title"), Menu->Back());

	FString Joined;
	Menu->OnJoinCode.BindLambda([&](const FString& Code) { Joined = Code; });
	TestTrue(TEXT("open join"), Menu->Open(tta::MenuScreen::Join));
	TestFalse(TEXT("bad code refused"), Menu->SubmitJoinCode(TEXT("IO")));
	Shows(tta::MenuScreen::Join, TEXT("still on Join"));
	TestTrue(TEXT("code accepted"), Menu->SubmitJoinCode(TEXT("abc-def")));
	TestEqual(TEXT("normalized code sent"), Joined, FString(TEXT("ABCDEF")));
	Shows(tta::MenuScreen::Lobby, TEXT("lobby"));

	Menu->SetRoomCode(TEXT("ABCDEF"), true);
	Menu->SetPlayers(1, 1);
	TestFalse(TEXT("can't start alone"), Menu->CanStart());
	Menu->SetPlayers(2, 2);
	TestTrue(TEXT("can start with 2 ready"), Menu->CanStart());
	const FLinearColor Before = Menu->GetLookColour();
	Menu->CycleLook(tta::LookPart::Colour, 1);
	TestNotEqual(TEXT("colour changed"), Menu->GetLookColour(), Before);
	Menu->CycleLook(tta::LookPart::Hat, -1);
	TestEqual(TEXT("hat wraps"), Menu->GetLook().hat, static_cast<int32>(tta::kHats.size()) - 1);

	TestTrue(TEXT("settings from lobby"), Menu->Open(tta::MenuScreen::Settings));
	tta::ClientSettings Loud;
	Loud.master = 400;
	Loud.sensitivity = 0;
	Menu->SetSettings(Loud);
	TestEqual(TEXT("volume clamped"), Menu->GetSettings().master, 100);
	TestEqual(TEXT("sensitivity clamped"), Menu->GetSettings().sensitivity, 10);
	TestFalse(TEXT("out-of-range resolution refused"), Menu->ApplyResolution(Menu->NumResolutionChoices()));
	const int16 Samples[] = {0, 16384, -8000};
	Menu->SetMicSamples(Samples, 3);
	TestTrue(TEXT("mic meter ~0.5"), FMath::IsNearlyEqual(Menu->GetMicLevel(), 0.5f, 0.01f));
	TestTrue(TEXT("back to lobby"), Menu->Back());
	TestTrue(TEXT("back to title"), Menu->Back());

	TestTrue(TEXT("open reels"), Menu->Open(tta::MenuScreen::Reels));
	TestEqual(TEXT("reels listed"), Menu->NumReels(), TTAReels::List().Num());
	return true;
}

#endif
