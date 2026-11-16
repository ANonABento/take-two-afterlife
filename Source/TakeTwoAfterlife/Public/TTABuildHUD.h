#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TTABuildHUD.generated.h"

class UFont;
class UTTAPodiumWidget;
class UTTAResultsWidget;

// Match HUD, styled by UTTAUIStyle inside the 1920x1080 safe area. Top bar on every phase:
// phase + round (left), timer (centre), budget while building (right). Below it the build
// tiles + panel while the player flies an ATTABuildPawn. In Podium it adds the results + podium widgets.
UCLASS()
class TAKETWOAFTERLIFE_API ATTABuildHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// Canvas pixels per 1080p design unit.
	float UiScale() const;
	void DrawCard(float X, float Y, float W, float H, int32 Fill);
	// Draws at 1080p design coords; returns the text width in design units.
	float DrawStyledText(const FString& Text, int32 Colour, float X, float Y, int32 Size, bool bDisplay);
	float MeasureText(const FString& Text, int32 Size, bool bDisplay);
	// Equal square tiles, one per build item (selected = Yellow); returns the row's bottom Y.
	float DrawBuildTiles(float X, float Y, int32 Selected);
	// Trap tile art: the item mesh rendered once off-screen, cached per build item.
	UTexture* TrapThumbnail(int32 Item);
	UPROPERTY() TMap<int32, TObjectPtr<class UTextureRenderTarget2D>> TrapThumbnails;
	void ShowResults();
	// Kahoot-style bars during each cut's Reveal (ATTACinema::GetScoreBoard).
	void DrawScoreBoard();

	UPROPERTY() TObjectPtr<UFont> DisplayFont;
	UPROPERTY() TObjectPtr<UFont> BodyFont;
	UPROPERTY() TObjectPtr<UTTAResultsWidget> Results;
	UPROPERTY() TObjectPtr<UTTAPodiumWidget> PodiumWidget;
};
