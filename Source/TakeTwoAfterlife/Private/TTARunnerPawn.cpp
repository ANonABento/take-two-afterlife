#include "TTARunnerPawn.h"
#include "Engine/SkeletalMesh.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/VoiceConfig.h"
#include "Sound/SoundAttenuation.h"
#include "TTAAudioDirector.h"
#include "TTAMicRecorder.h"
#include "TTACosmetics.h"
#include "TTAPlayerState.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "tta/MenuFlow.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "TTARunnerAnimInstance.h"
#include "tta/PunchArm.h"

namespace
{
const FName FlashParam(TEXT("Flash")); // M_TTA_Toon emissive override (task 11b)
const FName FaceParam(TEXT("FaceTexture")); // runner material face texture slot
const FName TintParam(TEXT("Tint"));
}

ATTARunnerPawn::ATTARunnerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(22.f, 50.f); // 1 m angel
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -51.65f));
	Wings = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wings"));
	Wings->SetupAttachment(GetMesh());
	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	Halo->SetupAttachment(GetMesh());
	Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	Handheld = CreateDefaultSubobject<UCameraComponent>(TEXT("Handheld"));
	MicRecorder = CreateDefaultSubobject<UTTAMicRecorder>(TEXT("MicRecorder"));
	Handheld->SetupAttachment(RootComponent);
	Handheld->SetRelativeLocation(FVector(40.f, 20.f, 50.f)); // held at chest height, right hand
	Handheld->bUsePawnControlRotation = true;
	Handheld->FieldOfView = 70.f;
	Handheld->SetAutoActivate(false);
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true; // sweep from pawn to camera; shorten the arm when a mesh is in the way
	CameraBoom->ProbeSize = 12.f;
	CameraBoom->ProbeChannel = ECC_Camera;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	Hat->SetupAttachment(GetMesh());
	for (UStaticMeshComponent* Part : {Wings.Get(), Halo.Get(), Hat.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Gen 3 angel animation
	static ConstructorHelpers::FClassFinder<UAnimInstance> AngelABP(
		TEXT("/Game/Characters/Angel/Anims/ABP_Angel"));
	if (AngelABP.Succeeded())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(AngelABP.Class);
		// Gen 3 mesh faces -X; turn to face +X
		GetMesh()->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	}
}

void ATTARunnerPawn::BeginPlay()
{
	// CDO config can be missed on a pawn spawned before the class ini is applied; read DefaultGame.ini here.
	LoadConfig();
	if (!RunnerMesh.IsNull())
	{
		USkeletalMesh* Loaded = RunnerMesh.LoadSynchronous();
		if (!Loaded)
		{
			UE_LOG(LogTemp, Error, TEXT("ATTARunnerPawn: RunnerMesh %s failed to load"), *RunnerMesh.ToString());
		}
		GetMesh()->SetSkeletalMesh(Loaded);
	}
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	MeshRelative = GetMesh()->GetRelativeTransform();
	// Only make MIDs for slots the loaded SK_Angel really has; with no mesh (or an empty slot)
	// CreateAndSetMaterialInstanceDynamic logs "Material index N invalid".
	const int32 NumSlots = GetMesh()->GetSkeletalMeshAsset() ? GetMesh()->GetNumMaterials() : 0;
	if (NumSlots == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATTARunnerPawn: %s has no skeletal mesh/materials; flash disabled"), *GetMesh()->GetName());
	}
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		if (!GetMesh()->GetMaterial(Slot)) continue;
		if (UMaterialInstanceDynamic* Mid = GetMesh()->CreateAndSetMaterialInstanceDynamic(Slot))
		{
			FlashMaterials.Add(Mid);
		}
	}
	SetForm(Form);
	if (const ATTAPlayerState* PS = GetPlayerState<ATTAPlayerState>())
	{
		const tta::PlayerLook Look = PS->GetLook();
		ApplyLook(Look.hat, Look.face);
	}
}

void ATTARunnerPawn::ApplyLook(int32 HatIndex, int32 FaceIndex)
{
	const UDataTable* Table = CosmeticsTable.LoadSynchronous();
	if (!Table)
	{
		UE_LOG(LogTemp, Error, TEXT("ATTARunnerPawn: CosmeticsTable %s failed to load"), *CosmeticsTable.ToString());
		return;
	}
	const TArray<FName> Rows = Table->GetRowNames();
	const FTTACosmeticRow* HatRow = Rows.IsValidIndex(HatIndex) ? Table->FindRow<FTTACosmeticRow>(Rows[HatIndex], TEXT("Hat")) : nullptr;
	const FTTACosmeticRow* FaceRow = Rows.IsValidIndex(FaceIndex) ? Table->FindRow<FTTACosmeticRow>(Rows[FaceIndex], TEXT("Face")) : nullptr;
	if (!HatRow || !FaceRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATTARunnerPawn: no cosmetic row for hat %d / face %d (%d rows)"), HatIndex, FaceIndex, Rows.Num());
		return;
	}
	Hat->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, HatSocket);
	Hat->SetStaticMesh(HatRow->HatMesh.LoadSynchronous());
	Hat->SetUsingAbsoluteScale(true);
    Hat->SetRelativeTransform(FTransform(HatRow->Rotation, HatRow->SocketOffset, HatRow->Scale));
    Hat->SetWorldScale3D(HatRow->Scale);
	if (tta::kPaletteHex.size() > static_cast<size_t>(HatRow->PaletteIndex) && HatRow->PaletteIndex >= 0)
	{
		if (UMaterialInstanceDynamic* HatMid = Hat->GetNumMaterials() > 0 && Hat->GetMaterial(0) ? Hat->CreateAndSetMaterialInstanceDynamic(0) : nullptr)
		{
			HatMid->SetVectorParameterValue(TintParam, FLinearColor(FColor::FromHex(UTF8_TO_TCHAR(tta::kPaletteHex[HatRow->PaletteIndex]))));
		}
	}
	UTexture2D* Face = FaceRow->FaceTexture.LoadSynchronous();
	for (UMaterialInstanceDynamic* Material : FlashMaterials)
	{
		if (Material && Face) Material->SetTextureParameterValue(FaceParam, Face);
	}
}

void ATTARunnerPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAxis(TEXT("MoveForward"), this, &ATTARunnerPawn::MoveForward);
	Input->BindAxis(TEXT("MoveRight"), this, &ATTARunnerPawn::MoveRight);
	Input->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	Input->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
	Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	Input->BindAction(TEXT("Dive"), IE_Pressed, this, &ATTARunnerPawn::Dive);
	Input->BindAction(TEXT("Punch"), IE_Pressed, this, &ATTARunnerPawn::Punch);
	Input->BindAction(TEXT("RaiseCamera"), IE_Pressed, this, &ATTARunnerPawn::RaiseCamera);
	Input->BindAction(TEXT("RaiseCamera"), IE_Released, this, &ATTARunnerPawn::LowerCamera);
	Input->BindAction(TEXT("ToggleVoiceMute"), IE_Pressed, this, &ATTARunnerPawn::ToggleVoiceMute);
}

void ATTARunnerPawn::ToggleVoiceMute()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !IsLocallyControlled()) return;
	bVoiceMuted = !bVoiceMuted;
	PC->ToggleSpeaking(!bVoiceMuted);
}

void ATTARunnerPawn::MoveForward(float Value)
{
	if (Value != 0.f && Life.state() == tta::LifeState::Alive)
	{
		AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X), Value);
	}
}

void ATTARunnerPawn::MoveRight(float Value)
{
	if (Value != 0.f && Life.state() == tta::LifeState::Alive)
	{
		AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), Value);
	}
}

void ATTARunnerPawn::Dive()
{
	if (Life.state() != tta::LifeState::Alive || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	LaunchCharacter(GetActorForwardVector() * DiveSpeed + FVector(0.f, 0.f, DiveLift), true, true);
}

void ATTARunnerPawn::Punch()
{
	ServerPunch();
}

void ATTARunnerPawn::ServerPunch_Implementation()
{
	if (PunchState.start(Life.state())) // refuses while not Alive, mid-punch or on cooldown
	{
		MulticastWindUp();
	}
}

void ATTARunnerPawn::MulticastWindUp_Implementation()
{
	GetWorld()->GetSubsystem<UTTAAudioDirector>()->PlayEvent(tta::AudioEvent::PunchWindUp);
}

void ATTARunnerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATTARunnerPawn, PunchPhaseRep);
}

void ATTARunnerPawn::UpdatePunchArm(float WorldDeltaSeconds)
{
	const tta::PunchPhase Phase = Life.state() == tta::LifeState::Alive
		? static_cast<tta::PunchPhase>(PunchPhaseRep) : tta::PunchPhase::Idle;
	if (Phase != ArmPhase)
	{
		ArmPhase = Phase;
		ArmPhaseTime = 0.f;
	}
	else
	{
		ArmPhaseTime += WorldDeltaSeconds;
	}
	UTTARunnerAnimInstance* Anim = Cast<UTTARunnerAnimInstance>(GetMesh()->GetAnimInstance());
	if (!Anim)
	{
		if (!bWarnedAnimClass)
		{
			bWarnedAnimClass = true;
			UE_LOG(LogTemp, Warning, TEXT("ATTARunnerPawn: %s AnimBP is not a UTTARunnerAnimInstance; punch arm pose disabled"), *GetName());
		}
		return;
	}
	const tta::PunchArmConfig Config;
	const double Length = tta::punchPhaseSeconds(Phase);
	const tta::PunchArmPose Pose = tta::punchArmPose(Phase, Length > 0.0 ? ArmPhaseTime / Length : 0.0, Config);
	Anim->UpperArmBone = FName(Config.upperArmBone);
	Anim->ForearmBone = FName(Config.forearmBone);
	Anim->UpperArmRotation = FRotator(Pose.upperArm.pitch, Pose.upperArm.yaw, Pose.upperArm.roll);
	Anim->ForearmRotation = FRotator(Pose.forearm.pitch, Pose.forearm.yaw, Pose.forearm.roll);
	Anim->PunchArmAlpha = static_cast<float>(Pose.alpha);
}

void ATTARunnerPawn::AdvancePunch(float WorldDeltaSeconds)
{
	// Same slicing as AdvanceLife: one Punch transition per advance call.
	constexpr float Step = 0.05f;
	for (float Left = WorldDeltaSeconds; Left > 0.f; Left -= Step)
	{
		if (PunchState.phase() == tta::PunchPhase::Idle && PunchState.cooldownLeft() <= 0.0) return;
		if (PunchState.advance(FMath::Min(Left, Step)) == tta::PunchEvent::OpenHitWindow)
		{
			PunchHitCheck();
		}
	}
}

void ATTARunnerPawn::PunchHitCheck()
{
	const FVector Forward = GetActorForwardVector();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TTAPunch), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation() + Forward * PunchReach, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(PunchRadius), Params);
	ATTARunnerPawn* Victim = nullptr;
	float Best = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(Overlap.GetActor());
		if (!Runner || Runner == this || Runner->GetLifeState() != tta::LifeState::Alive || Runner->IsPunchImmune()) continue;
		const float Dist = FVector::DistSquared(GetActorLocation(), Runner->GetActorLocation());
		if (Dist < Best)
		{
			Best = Dist;
			Victim = Runner;
		}
	}
	if (Victim)
	{
		Victim->MulticastPunched(Forward);
	}
}

void ATTARunnerPawn::MulticastPunched_Implementation(FVector_NetQuantizeNormal Direction)
{
	if (!Life.stun(tta::kStunSeconds)) return;
	StartRagdoll();
	GetMesh()->SetAllPhysicsLinearVelocity(Direction * PunchSpeed + FVector(0.f, 0.f, PunchLift), true);
	GetWorld()->GetSubsystem<UTTAAudioDirector>()->PlayEvent(tta::AudioEvent::PunchHit);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (IsLocallyControlled() && PC && ImpactShake)
	{
		PC->ClientStartCameraShake(ImpactShake);
	}
}

void ATTARunnerPawn::SetForm(ETTAForm NewForm)
{
	Form = NewForm;
	const tta::Form CoreForm = NewForm == ETTAForm::Angel ? tta::Form::Angel : tta::Form::Human;
	// Gen 3 angel: wings are built into SK_Angel and there is no halo.
	Wings->SetVisibility(false);
	Halo->SetVisibility(false);
	Hat->SetVisibility(CoreForm == tta::Form::Human);
}

void ATTARunnerPawn::Hit()
{
	if (!Life.hit())
	{
		return;
	}
	CustomTimeDilation = 0.f; // victim-only freeze; the world keeps running
	SetFlash(1.f);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && ImpactShake)
	{
		PC->ClientStartCameraShake(ImpactShake);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	OnImpact.Broadcast(this);
}

void ATTARunnerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// DeltaSeconds is scaled by CustomTimeDilation (0 while frozen); timers need world time.
	AdvanceLife(GetWorld()->GetDeltaSeconds());
	if (HasAuthority())
	{
		AdvancePunch(GetWorld()->GetDeltaSeconds());
		PunchPhaseRep = static_cast<uint8>(PunchState.phase());
	}
	UpdatePunchArm(GetWorld()->GetDeltaSeconds());
}

void ATTARunnerPawn::AdvanceLife(float WorldDeltaSeconds)
{
	// PawnLife applies one transition and drops the rest of dt. Feed it short slices so a
	// hitch (or a test that passes the whole recovery) still walks Frozen -> Ragdoll -> Alive.
	// Punch immunity also ticks down while Alive.
	constexpr float Step = 0.05f;
	for (float Left = WorldDeltaSeconds; Left > 0.f && (Life.state() != tta::LifeState::Alive || Life.punchImmune()); Left -= Step)
	{
		switch (Life.advance(FMath::Min(Left, Step)))
		{
		case tta::LifeEvent::StartRagdoll: StartRagdoll(); break;
		case tta::LifeEvent::Respawn: Respawn(); break;
		case tta::LifeEvent::GetUp: StopStun(); break;
		case tta::LifeEvent::None: break;
		}
	}
}

void ATTARunnerPawn::SetFlash(float Value)
{
	Flash = Value;
	for (UMaterialInstanceDynamic* Material : FlashMaterials)
	{
		if (Material) Material->SetScalarParameterValue(FlashParam, Value);
	}
}

void ATTARunnerPawn::StartRagdoll()
{
	CustomTimeDilation = 1.f;
	SetFlash(0.f);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	Body->SetAllBodiesSimulatePhysics(true);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();
}

void ATTARunnerPawn::Respawn()
{
	RestoreBody();
	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void ATTARunnerPawn::StopStun()
{
	// Every machine reaches GetUp on its own timer, but each ragdoll lands somewhere slightly
	// different. Only the server picks the spot; everyone else stays ragdolled until it arrives.
	if (!HasAuthority()) return;
	const FVector Pelvis = GetMesh()->GetBoneLocation(PelvisBone);
	FHitResult Floor;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TTAGetUp), false, this);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const bool bFloor = GetWorld()->LineTraceSingleByChannel(Floor, Pelvis + FVector(0.f, 0.f, HalfHeight),
		Pelvis - FVector(0.f, 0.f, GetUpTraceDepth), ECC_Visibility, Params);
	const FVector Ground = bFloor ? Floor.ImpactPoint : Pelvis - FVector(0.f, 0.f, HalfHeight);
	MulticastGetUp(Ground + FVector(0.f, 0.f, HalfHeight));
}

void ATTARunnerPawn::MulticastGetUp_Implementation(FVector_NetQuantize Landing)
{
	// Mesh back on the capsule, then move the capsule to the server's landing spot (teleport, no
	// sweep: a sweep from the pre-ragdoll capsule gets blocked and leaves the runner where they stood).
	RestoreBody();
	SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics);
}

void ATTARunnerPawn::RestoreBody()
{
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSimulatePhysics(false);
	Body->SetAllBodiesSimulatePhysics(false);
	Body->SetCollisionProfileName(TEXT("CharacterMesh"));
	Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Body->SetRelativeTransform(MeshRelative);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ATTARunnerPawn::SetFilming(bool bOn)
{
	Handheld->SetActive(bOn);
	FollowCamera->SetActive(!bOn); // one active camera, so the view uses the handheld while filming
}

bool ATTARunnerPawn::IsFilming() const
{
	return Handheld->IsActive();
}

void ATTARunnerPawn::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	if (const ATTAPlayerState* PS = Cast<ATTAPlayerState>(NewPlayerState); PS && HasActorBegunPlay())
	{
		const tta::PlayerLook Look = PS->GetLook();
		ApplyLook(Look.hat, Look.face);
	}
	if (!NewPlayerState || Talker || IsLocallyControlled() || GetNetMode() == NM_DedicatedServer) return;
	VoiceAttenuation = NewObject<USoundAttenuation>(this);
	VoiceAttenuation->Attenuation.bAttenuate = true;
	VoiceAttenuation->Attenuation.bSpatialize = true;
	VoiceAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	VoiceAttenuation->Attenuation.AttenuationShapeExtents = FVector(VoiceFullVolumeCm, 0.f, 0.f);
	VoiceAttenuation->Attenuation.FalloffDistance = VoiceSilentCm - VoiceFullVolumeCm;
	Talker = UVOIPTalker::CreateTalkerForPlayer(NewPlayerState);
	Talker->Settings.ComponentToAttachTo = GetMesh();
	Talker->Settings.AttenuationSettings = VoiceAttenuation;
}

void ATTARunnerPawn::FreezeAsStatue(int32 HatIndex, int32 FaceIndex, int32 Colour)
{
	SetForm(ETTAForm::Angel);
	ApplyLook(HatIndex, FaceIndex);
	const FLinearColor Body(FColor::FromHex(UTF8_TO_TCHAR(tta::kPaletteHex[Colour])));
	for (UMaterialInstanceDynamic* Material : FlashMaterials)
	{
		if (Material) Material->SetVectorParameterValue(TintParam, Body);
	}
	GetCharacterMovement()->DisableMovement();
	GetMesh()->bPauseAnims = true;
	SetActorTickEnabled(false);
	Tags.AddUnique(StatueTag);
}
