#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/Optional.h"
#include "tta/Lawnmower.h"
#include "tta/Placement.h"
#include "tta/TrapTimeline.h"
#include "Engine/HitResult.h"
#include "TTATrapActor.generated.h"

class ATTARunnerPawn;
class UBoxComponent;
class UStaticMeshComponent;

// One placed trap in the world (plan req 3). Every machine drives its moving
// part from tta::pose() at the replicated server time since Run started, so all
// clients agree without replicating transforms. Hits are decided on the server only.
// The lawnmower is trigger-driven instead: each tick steps its Core tta::Lawnmower
// with every runner's position, and Core decides its trigger, offset and hits.
UCLASS()
class TAKETWOAFTERLIFE_API ATTATrapActor : public AActor
{
	GENERATED_BODY()

public:
	ATTATrapActor();

	// Takes type, settings, id, owner and grid transform from a Core placement.
	// Throws std::invalid_argument for a camera placement or out-of-range settings.
	void InitFromPlacement(const tta::Placement& InPlacement);
	const tta::Placement& GetPlacement() const { return Placement; }

	// Run start in GameState server seconds; pose time is measured from here.
	void SetRunStartTime(double ServerSeconds) { RunStartTime = ServerSeconds; }
	// Moves the parts to the Core pose at Run time T seconds.
	void ApplyPoseAt(double T);

	// Server only: hits an alive runner and records it with the run director.
	// Returns false on clients, for a runner that isn't alive, or with no running director.
	bool TryHit(ATTARunnerPawn* Runner);

	// Relative transform of the moving part for a Core pose (cm, degrees).
	static FTransform MovingTransform(tta::TrapType Type, const tta::TrapPose& Pose);
	// Lawnmower only: steps the Core mower to Run time T with every runner in the world,
	// sends each hit Core reports through TryHit, then moves the deck and spins the blade.
	void UpdateMower(double T);
	// Core mower state (Idle when this isn't a lawnmower).
	tta::LawnmowerState GetMowerState() const { return Mower ? Mower->state() : tta::LawnmowerState::Idle; }
	// Lawnmower deck offset along +X (cm) and visual-only blade spin while Driving.
	static FTransform MowerTransform(double OffsetMeters);
	static FTransform BladeTransform(tta::LawnmowerState State, double T);
	// Relative transform of the cannon ball / banana peels (cm). Scale 0 when inactive.
	static FTransform ProjectileTransform(tta::TrapType Type, const tta::TrapPose& Pose);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Moving;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Projectile;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Hitbox;
	// Lawnmower blade disc: visual only, spins around Z while Driving.
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Blade;

	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION() void OnHitboxOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	tta::Placement Placement;
	TOptional<tta::Lawnmower> Mower;
	double RunStartTime = 0.0;
};
