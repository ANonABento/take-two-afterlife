#include "TTATitleGameMode.h"

#include "Camera/CameraActor.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineSubsystem.h"
#include "TTAMenuWidget.h"
#include "TTASessionSubsystem.h"

ATTATitleGameMode::ATTATitleGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = AHUD::StaticClass();
	PlayerControllerClass = ATTATitlePlayerController::StaticClass();
}

void ATTATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TitleCamera")))
		{
			SetViewTarget(*It);
			break;
		}
	}

	Menu = CreateWidget<UTTAMenuWidget>(this, UTTAMenuWidget::StaticClass());
	UTTASessionSubsystem* Session = GetGameInstance()->GetSubsystem<UTTASessionSubsystem>();
	// EOS lobby when EOS is the platform service; the NULL subsystem (no Artifact IDs yet) hosts direct.
	const IOnlineSubsystem* Online = IOnlineSubsystem::Get();
	const bool bEos = Online && Online->GetSubsystemName() == TEXT("EOS");
	Menu->OnHost.BindLambda([Session, bEos]() { if (bEos) Session->HostLobby(); else Session->HostDirect(); });
	Menu->OnJoinCode.BindLambda([Session](const FString& Code) { Session->JoinByCode(Code); });
	Menu->OnJoinDirect.BindLambda([Session](const FString& Address) { Session->JoinDirect(Address); });
	Menu->OnQuit.BindLambda([this]() { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); });
	Menu->AddToPlayerScreen();

	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}
