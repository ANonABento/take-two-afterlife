#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "TTATitleGameMode.generated.h"

class UTTAMenuWidget;

// Title map only (World Settings override, set by Scripts/create_title_map.py): no pawn, plain HUD,
// each local player gets the title menu. The match mode (ATTAGameMode + TTABuildHUD) runs in Backyard.
UCLASS()
class TAKETWOAFTERLIFE_API ATTATitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTATitleGameMode();
};

// Shows the title menu (Host / Join code / Direct IP / Quit), the mouse cursor, UI-only input,
// and views the level's CameraActor tagged TitleCamera.
UCLASS()
class TAKETWOAFTERLIFE_API ATTATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UTTAMenuWidget* GetMenu() const { return Menu; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UTTAMenuWidget> Menu;
};
