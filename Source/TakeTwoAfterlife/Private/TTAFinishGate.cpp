#include "TTAFinishGate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "TTAGameMode.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "Engine/HitResult.h"

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
	// Heaven lobby: walking or jumping through marks the player ready.
	if (ATTAGameMode* Mode = GetWorld()->GetAuthGameMode<ATTAGameMode>(); Mode && Mode->MarkReady(Runner->GetController()))
	{
		return true;
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
