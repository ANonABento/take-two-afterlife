#include "TTATitleGameMode.h"

#include "Animation/AnimInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"
#include "TTACosmetics.h"
#include "TTAGameMode.h"
#include "TTARunnerPawn.h"
#include "TTAMenuWidget.h"
#include "TTASessionSubsystem.h"
#include "tta/MenuFlow.h"

namespace
{
const FName FaceParam(TEXT("FaceTexture"));
const FName FaceOffsetParam(TEXT("FaceUVOffset"));
const FName HatSocket(TEXT("head"));

void DressAngel(ASkeletalMeshActor* Angel, const UDataTable* Table, int32 HatIndex, int32 FaceIndex)
{
	if (!Angel || !Table) return;
	const FTTACosmeticRow* HatRow = ATTARunnerPawn::FindCosmeticRow(Table, HatIndex);
	const FTTACosmeticRow* FaceRow = ATTARunnerPawn::FindCosmeticRow(Table, FaceIndex);
	UStaticMesh* HatMesh = ATTARunnerPawn::LoadHatMesh(HatIndex);
	if (!HatRow && !HatMesh) return;
	USkeletalMeshComponent* Body = Angel->GetSkeletalMeshComponent();
	if (!Body) return;

	UStaticMeshComponent* Hat = NewObject<UStaticMeshComponent>(Angel);
	Hat->SetMobility(EComponentMobility::Movable);
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const bool bSocket = Body->DoesSocketExist(HatSocket);
	if (!bSocket)
	{
		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			UE_LOG(LogTemp, Warning, TEXT("Title choir: SK_Angel has no 'head' socket; hats sit on the mesh origin"));
		}
	}
	Hat->RegisterComponent();
	const FTTACosmeticRow Fallback;
	ATTARunnerPawn::DressHat(Hat, Body, bSocket ? HatSocket : NAME_None, HatRow ? *HatRow : Fallback, HatMesh);

	UTexture2D* Face = nullptr;
	if (FaceIndex >= 0 && FaceIndex < static_cast<int32>(tta::kFaces.size()))
	{
		const FString FaceName = UTF8_TO_TCHAR(tta::kFaces[FaceIndex]);
		Face = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Characters/Faces/T_Face_%s.T_Face_%s"), *FaceName, *FaceName));
	}
	if (!Face && FaceRow) Face = FaceRow->FaceTexture.LoadSynchronous();
	const FLinearColor FaceDrop(0.f, TTAFaceDropV, 0.f, 1.f);
	const int32 NumSlots = Body->GetNumMaterials();
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		if (!Body->GetMaterial(Slot)) continue;
		if (UMaterialInstanceDynamic* Mid = Body->CreateAndSetMaterialInstanceDynamic(Slot))
		{
			Mid->SetVectorParameterValue(FaceOffsetParam, FaceDrop);
			if (Face) Mid->SetTextureParameterValue(FaceParam, Face);
		}
	}
}

void SpawnTitleChoir(UWorld* World)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Angel/SK_Angel.SK_Angel"));
	if (!Mesh || !World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Title choir: SK_Angel did not load"));
		return;
	}
	UClass* Anim = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Angel/Anims/ABP_Angel.ABP_Angel_C"));
	const UDataTable* Looks = LoadObject<UDataTable>(nullptr, TEXT("/Game/Data/DT_Cosmetics.DT_Cosmetics"));
	const int32 HatCount = static_cast<int32>(tta::kHats.size());
	if (!Looks || HatCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Title choir: DT_Cosmetics has no rows; angels spawn without hats or faces"));
	}
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
		if (Looks && HatCount > 0)
		{
			DressAngel(Angel, Looks, Rng.RandHelper(HatCount), Rng.RandHelper(static_cast<int32>(tta::kFaces.size())));
		}
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
