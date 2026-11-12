#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TTABuildHUD.generated.h"

// Build panel while the player flies an ATTABuildPawn: budget, camera counts by type,
// selected item, ghost/server result and the selected trap's 2 settings. Skinned in task 24.
UCLASS()
class TAKETWOAFTERLIFE_API ATTABuildHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
