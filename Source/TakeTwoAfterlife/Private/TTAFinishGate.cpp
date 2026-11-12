#include "TTAFinishGate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"

ATTAFinishGate::ATTAFinishGate()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Arch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arch"));
	Arch->SetupAttachment(RootComponent);
	Arch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(RootComponent);
	Trigger->SetBoxExtent(FVector(50.f, 400.f, 200.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATTAFinishGate::OnTriggerOverlap);
}

bool ATTAFinishGate::HandleRunnerEntered(ATTARunnerPawn* Runner)
{
	if (!Runner || !HasAuthority())
	{
		return false;
	}
	for (TActorIterator<ATTARunDirector> It(GetWorld()); It; ++It)
	{
		if (It->IsRunning() && It->PlayerId(Runner) != INDEX_NONE)
		{
			It->MarkFinished(Runner);
			return true;
		}
	}
	return false;
}

void ATTAFinishGate::OnTriggerOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandleRunnerEntered(Cast<ATTARunnerPawn>(Other));
}
