#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "tta/Placement.h"
#include "TTAPlacementMarker.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

// Server answer to a build request. The first five mirror tta::PlaceResult.
UENUM(BlueprintType)
enum class ETTAPlaceOutcome : uint8 { Ok, OutOfBounds, OverBudget, CameraCapReached, BadSetting, WrongPhase, NotOwner, BadItem };

inline ETTAPlaceOutcome ToUE(tta::PlaceResult Result) { return static_cast<ETTAPlaceOutcome>(Result); }

// Build menu items: the Core trap catalogue in order, then the camera types.
namespace TTABuildItem
{
	TAKETWOAFTERLIFE_API int32 Count();
	TAKETWOAFTERLIFE_API bool IsCamera(int32 Item);
	TAKETWOAFTERLIFE_API tta::TrapType Trap(int32 Item);
	TAKETWOAFTERLIFE_API tta::CameraType Camera(int32 Item);
	TAKETWOAFTERLIFE_API FString Label(int32 Item);
}

// Replicated copy of one Core placement.
USTRUCT()
struct TAKETWOAFTERLIFE_API FTTAPlacementRep
{
	GENERATED_BODY()

	UPROPERTY() int32 Id = 0;
	UPROPERTY() int32 Owner = 0;
	UPROPERTY() int32 Item = 0;
	UPROPERTY() int32 CellX = 0;
	UPROPERTY() int32 CellY = 0;
	UPROPERTY() int32 Rotation = 0;
	UPROPERTY() int32 Setting0 = 0;
	UPROPERTY() int32 Setting1 = 0;
	UPROPERTY() bool bRevealed = false;

	static FTTAPlacementRep FromCore(const tta::Placement& Placement);
	tta::Placement ToCore() const;
};

// One placement in the world during Build (plan req 2). Until Run it is net-relevant
// only to its owner's connection and drawn only on the owner's machine, so other
// players never receive it; after Run starts everyone does.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API ATTAPlacementMarker : public AActor
{
	GENERATED_BODY()

public:
	ATTAPlacementMarker();

	// Server only.
	void SetData(const FTTAPlacementRep& InData);
	const FTTAPlacementRep& GetData() const { return Data; }

	// World transform of a placement: grid cell in cm from the corridor origin, 45 degree yaw,
	// cameras lifted to CameraHeightCm.
	static FTransform WorldTransform(const FTTAPlacementRep& Rep);
	// PlayerState id of a controller or pawn; INDEX_NONE without a PlayerState.
	static int32 PlayerIdOf(const AActor* ControllerOrPawn);
	// Configured mesh for a build item; logs an error and returns null when missing.
	UStaticMesh* MeshForItem(int32 Item) const;

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;

	// One mesh per build item (TTABuildItem order). Placeholder engine shapes in DefaultGame.ini.
	UPROPERTY(Config, EditAnywhere, Category = "TTA") TArray<TSoftObjectPtr<UStaticMesh>> ItemMeshes;
	UPROPERTY(Config, EditAnywhere, Category = "TTA") float CameraHeightCm = 250.f;

private:
	UFUNCTION() void OnRep_Data();
	void Refresh();

	UPROPERTY(ReplicatedUsing = OnRep_Data) FTTAPlacementRep Data;
};
