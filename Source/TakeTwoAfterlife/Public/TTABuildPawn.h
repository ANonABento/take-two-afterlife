#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "TTAPlacementMarker.h"
#include "TTABuildPawn.generated.h"

// Free-fly Build camera (DefaultPawn WASD/E/Q + mouse look) with a ghost preview of the
// selected item snapped to the grid under the crosshair. All changes go through server RPCs
// to ATTAGameMode, which applies the Core rules.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API ATTABuildPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	ATTABuildPawn(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	int32 GetSelectedItem() const { return SelectedItem; }
	int32 GetSelectedPlacement() const { return SelectedPlacement; }
	ETTAPlaceOutcome GetGhostOutcome() const { return GhostOutcome; }
	ETTAPlaceOutcome GetLastServerOutcome() const { return LastServerOutcome; }
	// The local player's own placements as a Core layout, rebuilt from replicated markers.
	tta::Layout OwnLayout() const;
	ATTAPlacementMarker* FindMarker(int32 Id) const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ghost;
	UPROPERTY(Config, EditAnywhere, Category = "TTA") float TraceDistanceCm = 10000.f;

private:
	bool TraceCursor(FHitResult& Hit) const;
	ATTAPlacementMarker* OwnMarkerUnderCursor() const;
	void Place();
	void RemoveUnderCursor();
	void SelectUnderCursor();
	void Rotate();
	void NextItem();
	void PrevItem();
	void AdjustSetting(int32 Index, int32 Delta);
	void Setting1Down() { AdjustSetting(0, -1); }
	void Setting1Up() { AdjustSetting(0, 1); }
	void Setting2Down() { AdjustSetting(1, -1); }
	void Setting2Up() { AdjustSetting(1, 1); }

	UFUNCTION(Server, Reliable) void ServerPlace(int32 Item, FVector_NetQuantize WorldCm, float YawDeg);
	UFUNCTION(Server, Reliable) void ServerRemove(int32 Id);
	UFUNCTION(Server, Reliable) void ServerSetSetting(int32 Id, int32 Index, int32 Value);
	UFUNCTION(Client, Reliable) void ClientResult(ETTAPlaceOutcome Outcome, int32 Id);

	int32 SelectedItem = 0;
	int32 RotationStep = 0;
	int32 SelectedPlacement = 0;
	bool bGhostOnFloor = false;
	FVector GhostCm = FVector::ZeroVector;
	ETTAPlaceOutcome GhostOutcome = ETTAPlaceOutcome::Ok;
	ETTAPlaceOutcome LastServerOutcome = ETTAPlaceOutcome::Ok;
};
