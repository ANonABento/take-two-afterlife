#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TTAWorldLook.generated.h"

class UMaterialParameterCollection;

UENUM(BlueprintType)
enum class ETTAWorld : uint8
{
	Heaven,
	Course
};

// One MPC_TTA_World preset; field names match the collection's scalar parameters.
USTRUCT(BlueprintType)
struct FTTAWorldLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Saturation = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Contrast = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShadowFloor = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Bands = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RimStrength = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Warmth = 0.f;
};

// Values live in Config/DefaultGame.ini; Scripts/toon_spec.py reads the same section.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "TTA World Look"))
class TAKETWOAFTERLIFE_API UTTAWorldLookSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere) FTTAWorldLook Heaven;
	UPROPERTY(Config, EditAnywhere) FTTAWorldLook Course;
};

UCLASS()
class TAKETWOAFTERLIFE_API UTTAWorldLookLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Writes the heaven or course preset into MPC_TTA_World for this world.
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"))
	static void ApplyWorldLook(const UObject* WorldContext, UMaterialParameterCollection* Collection, ETTAWorld World);
};
