#include "TTATrapActor.h"
#include "TTAPlacementMarker.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"

#include <stdexcept>

namespace
{
constexpr float kTrapCmPerMeter = 100.f;
}

ATTATrapActor::ATTATrapActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Moving = CreateDefaultSubobject<USceneComponent>(TEXT("Moving"));
	Moving->SetupAttachment(RootComponent);
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Moving);
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Projectile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile"));
	Projectile->SetupAttachment(RootComponent);
	Projectile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(Moving);
	Hitbox->SetBoxExtent(FVector(50.f));
	Hitbox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Hitbox->OnComponentBeginOverlap.AddDynamic(this, &ATTATrapActor::OnHitboxOverlap);
}

void ATTATrapActor::InitFromPlacement(const tta::Placement& InPlacement)
{
	if (InPlacement.isCamera)
	{
		throw std::invalid_argument("ATTATrapActor needs a trap placement");
	}
	tta::pose(InPlacement.trap, InPlacement.settings, 0.0); // throws on out-of-range settings
	Placement = InPlacement;
	SetActorTransform(ATTAPlacementMarker::WorldTransform(FTTAPlacementRep::FromCore(Placement)));
	ApplyPoseAt(0.0);
}

FTransform ATTATrapActor::MovingTransform(tta::TrapType Type, const tta::TrapPose& Pose)
{
	const float Angle = static_cast<float>(Pose.angleDeg);
	const float Reach = static_cast<float>(Pose.extensionMeters) * kTrapCmPerMeter;
	switch (Type)
	{
	case tta::TrapType::Hammer: return FTransform(FRotator(0.f, 0.f, Angle));
	case tta::TrapType::Spinner:
		return FTransform(FRotator(0.f, Angle, 0.f), FVector::ZeroVector, FVector(FMath::Max(Reach, 1.f) / kTrapCmPerMeter, 1.f, 1.f));
	case tta::TrapType::PusherWall: return FTransform(FVector(Reach, 0.f, 0.f));
	case tta::TrapType::Trapdoor: return FTransform(FRotator(Angle, 0.f, 0.f));
	case tta::TrapType::Cannon:
	case tta::TrapType::Banana: return FTransform::Identity;
	}
	checkNoEntry();
	return FTransform::Identity;
}

FTransform ATTATrapActor::ProjectileTransform(tta::TrapType Type, const tta::TrapPose& Pose)
{
	if (Type == tta::TrapType::Cannon)
	{
		const float Scale = Pose.projectileActive ? static_cast<float>(Pose.projectileRadiusMeters) * 2.f : 0.f;
		return FTransform(FRotator::ZeroRotator,
			FVector(static_cast<float>(Pose.projectileDistanceMeters) * kTrapCmPerMeter, 0.f, 0.f), FVector(Scale));
	}
	if (Type == tta::TrapType::Banana)
	{
		return FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(static_cast<float>(Pose.count), 1.f, 1.f));
	}
	return FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector);
}

void ATTATrapActor::ApplyPoseAt(double T)
{
	const tta::TrapPose Pose = tta::pose(Placement.trap, Placement.settings, FMath::Max(T, 0.0));
	Moving->SetRelativeTransform(MovingTransform(Placement.trap, Pose));
	Projectile->SetRelativeTransform(ProjectileTransform(Placement.trap, Pose));
}

void ATTATrapActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (const AGameStateBase* State = GetWorld()->GetGameState())
	{
		ApplyPoseAt(State->GetServerWorldTimeSeconds() - RunStartTime);
	}
}

bool ATTATrapActor::TryHit(ATTARunnerPawn* Runner)
{
	if (!HasAuthority() || !Runner || Runner->GetLifeState() != tta::LifeState::Alive)
	{
		return false;
	}
	TActorIterator<ATTARunDirector> Director(GetWorld());
	if (!Director || !Director->IsRunning())
	{
		return false;
	}
	Runner->Hit();
	Director->RecordHit(Placement, Runner);
	return true;
}

void ATTATrapActor::OnHitboxOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	TryHit(Cast<ATTARunnerPawn>(Other));
}
