#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TTARunnerAnimInstance.generated.h"

// Parent class for ABP_Angel. ATTARunnerPawn writes these every tick from the
// replicated punch phase (tta::punchArmPose); the AnimBP feeds them into two
// Transform (Modify) Bone nodes on UpperArmBone / ForearmBone, weighted by PunchArmAlpha.
UCLASS()
class TAKETWOAFTERLIFE_API UTTARunnerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TTA|Punch") FName UpperArmBone;
	UPROPERTY(BlueprintReadOnly, Category = "TTA|Punch") FName ForearmBone;
	UPROPERTY(BlueprintReadOnly, Category = "TTA|Punch") FRotator UpperArmRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "TTA|Punch") FRotator ForearmRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "TTA|Punch") float PunchArmAlpha = 0.f;
};
