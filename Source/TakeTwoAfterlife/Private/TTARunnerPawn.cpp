#include "TTARunnerPawn.h"
#include "Engine/SkeletalMesh.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/VoiceConfig.h"
#include "Sound/SoundAttenuation.h"
#include "TTAMicRecorder.h"

namespace
{
const FName FlashParam(TEXT("Flash")); // M_TTA_Toon emissive override (task 11b)
}

ATTARunnerPawn::ATTARunnerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
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
	Hat->SetupAttachment(GetMesh());
	for (UStaticMeshComponent* Part : {Wings.Get(), Halo.Get(), Hat.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATTARunnerPawn::BeginPlay()
{
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
	for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot)
	{
		FlashMaterials.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(Slot));
	}
	SetForm(Form);
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
	Input->BindAction(TEXT("RaiseCamera"), IE_Pressed, this, &ATTARunnerPawn::RaiseCamera);
	Input->BindAction(TEXT("RaiseCamera"), IE_Released, this, &ATTARunnerPawn::LowerCamera);
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

void ATTARunnerPawn::SetForm(ETTAForm NewForm)
{
	Form = NewForm;
	const tta::Form CoreForm = NewForm == ETTAForm::Angel ? tta::Form::Angel : tta::Form::Human;
	Wings->SetVisibility(tta::wingsVisible(CoreForm));
	Halo->SetVisibility(tta::haloVisible(CoreForm));
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
}

void ATTARunnerPawn::AdvanceLife(float WorldDeltaSeconds)
{
	switch (Life.advance(WorldDeltaSeconds))
	{
	case tta::LifeEvent::StartRagdoll: StartRagdoll(); break;
	case tta::LifeEvent::Respawn: Respawn(); break;
	case tta::LifeEvent::None: break;
	}
}

void ATTARunnerPawn::SetFlash(float Value)
{
	Flash = Value;
	for (UMaterialInstanceDynamic* Material : FlashMaterials)
	{
		Material->SetScalarParameterValue(FlashParam, Value);
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
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSimulatePhysics(false);
	Body->SetAllBodiesSimulatePhysics(false);
	Body->SetCollisionProfileName(TEXT("CharacterMesh"));
	Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Body->SetRelativeTransform(MeshRelative);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ATTARunnerPawn::SetFilming(bool bOn)
{
	Handheld->SetActive(bOn);
}

bool ATTARunnerPawn::IsFilming() const
{
	return Handheld->IsActive();
}

void ATTARunnerPawn::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	if (!NewPlayerState || Talker || IsLocallyControlled() || GetNetMode() == NM_DedicatedServer) return;
	VoiceAttenuation = NewObject<USoundAttenuation>(this);
	VoiceAttenuation->Attenuation.bAttenuate = true;
	VoiceAttenuation->Attenuation.bSpatialize = true;
	VoiceAttenuation->Attenuation.FalloffDistance = VoiceFalloffCm;
	Talker = UVOIPTalker::CreateTalkerForPlayer(NewPlayerState);
	Talker->Settings.ComponentToAttachTo = GetMesh();
	Talker->Settings.AttenuationSettings = VoiceAttenuation;
}
