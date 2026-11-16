#include "TTATitleGameMode.h"

#include "Animation/AnimInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "TTAGameMode.h"
#include "TTAMenuWidget.h"
#include "TTASessionSubsystem.h"

namespace
{
void SpawnTitleChoir(UWorld* World)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Angel/SK_Angel.SK_Angel"));
	if (!Mesh || !World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Title choir: SK_Angel did not load"));
		return;
	}
	UClass* Anim = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Angel/Anims/ABP_Angel.ABP_Angel_C"));
	FRandomStream Rng(2026);
	for (int32 i = 0; i < 40; ++i)
	{
		const float Angle = Rng.FRandRange(-55.f, 55.f);
		const float Dist = Rng.FRandRange(220.f, 1600.f);
		const float Rad = FMath::DegreesToRadians(Angle);
		const FVector At(FMath::Cos(Rad) * Dist - 80.f, FMath::Sin(Rad) * Dist, Rng.FRand() < 0.45f ? Rng.FRandRange(30.f, 560.f) : 0.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASkeletalMeshActor* Angel = World->SpawnActor<ASkeletalMeshActor>(At, FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params);
		if (!Angel) continue;
		USkeletalMeshComponent* Body = Angel->GetSkeletalMeshComponent();
		Body->SetSkeletalMesh(Mesh);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		Body->SetRelativeScale3D(FVector(Rng.FRandRange(0.85f, 1.35f)));
		if (Anim) Body->SetAnimInstanceClass(Anim);
	}
}
}

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
	Menu->OnPlaySolo.BindLambda([Session]() { Session->HostSolo(ATTAGameMode::MaxBots); });
	Menu->OnJoinCode.BindLambda([Session](const FString& Code) { Session->JoinByCode(Code); });
	Menu->OnJoinDirect.BindLambda([Session](const FString& Address) { Session->JoinDirect(Address); });
	Menu->OnQuit.BindLambda([this]() { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); });
	Menu->AddToPlayerScreen();
	SpawnTitleChoir(GetWorld());
	if (FParse::Param(FCommandLine::Get(), TEXT("autodemo")))
	{
		FTimerHandle Auto;
		GetWorldTimerManager().SetTimer(Auto, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (Menu) Menu->PlaySolo();
		}), 2.f, false);
	}

	bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Menu->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}
