#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTAKillPlane.generated.h"

class ATTARunnerPawn;
class UBoxComponent;

// Volume under the course: a runner who falls in is hit (ragdoll, then respawn at the start).
// Sits well above the engine KillZ so fallen pawns are respawned, never destroyed.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAKillPlane : public AActor
{
	GENERATED_BODY()

public:
	ATTAKillPlane();

	// Server only. Returns true when an alive runner was hit.
	bool HandleRunnerFell(ATTARunnerPawn* Runner);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Volume;

private:
	UFUNCTION() void OnVolumeOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
