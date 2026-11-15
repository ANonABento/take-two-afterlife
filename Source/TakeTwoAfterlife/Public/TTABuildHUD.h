#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "TTABuildHUD.generated.h"

class UBorder;
class USizeBox;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class ATTAGameState;
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
	void ShowResults();
	// Creates Overlay on first use and fills it from the game state and pawn.
	void UpdateOverlay(const ATTAGameState& State);
	// Now-showing card, and Kahoot-style bars during each cut's Reveal (ATTACinema::GetScoreBoard).
	void UpdatePremiere(bool bPremiere);

	UPROPERTY() TObjectPtr<UTTABuildHUDWidget> Overlay;
	UPROPERTY() TObjectPtr<UTTAResultsWidget> Results;
	UPROPERTY() TObjectPtr<UTTAPodiumWidget> PodiumWidget;
};
