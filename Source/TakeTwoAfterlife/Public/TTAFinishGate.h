#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTAFinishGate.generated.h"

class ATTARunnerPawn;
class UBoxComponent;
class UStaticMeshComponent;

// The course's finish: a runner entering the trigger during Run is marked finished
// with the world's ATTARunDirector (server only). The arch is visual only.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAFinishGate : public AActor
{
	GENERATED_BODY()

public:
	ATTAFinishGate();

	// Returns true when the runner was marked finished: authority, a running director, a runner in it.
	bool HandleRunnerEntered(ATTARunnerPawn* Runner);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Arch;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION() void OnTriggerOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
};
