#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/HUD.h"
#include "TTABuildHUD.generated.h"

class UBorder;
class UFont;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class ATTAGameState;
class UTTAPodiumWidget;
class UTTAResultsWidget;

// Build + Run overlay, built only from UTTAUIStyle builders (mockups ui/v2/build.png, run.png):
// phase/round panel (left), timer banner (centre), budget panel (right); ready count in the
// lobby, film hint in Run, build panel with camera-type icons while flying an ATTABuildPawn.
// ATTABuildHUD fills it every frame.
UCLASS()
class TAKETWOAFTERLIFE_API UTTABuildHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	friend class ATTABuildHUD;

	UPROPERTY() TObjectPtr<UTextBlock> PhaseText;
	UPROPERTY() TObjectPtr<UTextBlock> RoundText;
	UPROPERTY() TObjectPtr<UTextBlock> TimerText;
	UPROPERTY() TObjectPtr<UHorizontalBox> ReadyRow;
	UPROPERTY() TObjectPtr<UTextBlock> ReadyText;
	UPROPERTY() TObjectPtr<UHorizontalBox> FilmHint;
	UPROPERTY() TObjectPtr<UBorder> BudgetPanel;
	UPROPERTY() TObjectPtr<UTextBlock> BudgetText;
	UPROPERTY() TObjectPtr<UBorder> BuildPanel;
	UPROPERTY() TObjectPtr<UTextBlock> CameraTotal;
	// Fixed, Pan, Follow, TrapCam (tta::CameraType order).
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CameraCounts;
	UPROPERTY() TObjectPtr<UTextBlock> ItemText;
	UPROPERTY() TObjectPtr<UTextBlock> GhostText;
	UPROPERTY() TObjectPtr<UTextBlock> RejectedText;
	UPROPERTY() TObjectPtr<UVerticalBox> SelectedBox;
	// Selected trap name, then its two settings.
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SelectedLines;
};

// Match HUD: shows UTTABuildHUDWidget in every phase but Podium, and draws the premiere
// score board on the canvas. In Podium it adds the results + podium widgets.
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
	void ShowResults();
	// Creates Overlay on first use and fills it from the game state and pawn.
	void UpdateOverlay(const ATTAGameState& State);
	// Kahoot-style bars during each cut's Reveal (ATTACinema::GetScoreBoard).
	void DrawScoreBoard();

	UPROPERTY() TObjectPtr<UFont> DisplayFont;
	UPROPERTY() TObjectPtr<UFont> BodyFont;
	UPROPERTY() TObjectPtr<UTTABuildHUDWidget> Overlay;
	UPROPERTY() TObjectPtr<UTTAResultsWidget> Results;
	UPROPERTY() TObjectPtr<UTTAPodiumWidget> PodiumWidget;
};
