#include "TTAKillPlane.h"

#include "Components/BoxComponent.h"
#include "TTARunnerPawn.h"

ATTAKillPlane::ATTAKillPlane()
{
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	RootComponent = Volume;
	Volume->SetBoxExtent(FVector(4000.f, 2000.f, 200.f));
	Volume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Volume->OnComponentBeginOverlap.AddDynamic(this, &ATTAKillPlane::OnVolumeOverlap);
}

bool ATTAKillPlane::HandleRunnerFell(ATTARunnerPawn* Runner)
{
	if (!Runner || !HasAuthority() || Runner->GetLifeState() != tta::LifeState::Alive)
	{
		return false;
	}
	Runner->Hit();
	return true;
}

void ATTAKillPlane::OnVolumeOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandleRunnerFell(Cast<ATTARunnerPawn>(Other));
}
