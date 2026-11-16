#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TTACosmetics.generated.h"

class UStaticMesh;
class UTexture2D;

// One row of /Game/Data/DT_Cosmetics (built by Scripts/build_cosmetics_table.py).
// Row i = hat i and face i of tta::kHats / tta::kFaces, so PlayerState indices pick rows.
USTRUCT(BlueprintType)
struct TAKETWOAFTERLIFE_API FTTACosmeticRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> HatMesh;
	// Relative to the runner mesh's head socket.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector SocketOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Scale = FVector::OneVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> FaceTexture;
	// Hat tint, index into tta::kPaletteHex.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PaletteIndex = 0;
};

// M_TTA_Angel FaceUVOffset.G. Positive moves the sample up the texture, which drops the
// painted face on the gen 3 head (the face island sits high).
static constexpr float TTAFaceDropV = 0.20f;
