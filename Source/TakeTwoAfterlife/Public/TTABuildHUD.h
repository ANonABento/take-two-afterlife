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

// Build + Run overlay, built only from UTTAUIStyle builders (mockups ui/v2/build.png, run.png):
// phase/round panel (left), timer banner (centre), budget panel (right); ready count in the
// lobby, film hint in Run, build panel with camera-type icons while flying an ATTABuildPawn.
// Premiere (ui/v2/premiere.png): now-showing card with "CUT x OF y", then score bars on Reveal.
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

	// Adds score rows until there are Count.
	void EnsureScoreRows(int32 Count);

	UPROPERTY() TObjectPtr<UVerticalBox> PremiereBox;
	UPROPERTY() TObjectPtr<UBorder> NowShowing;
	UPROPERTY() TObjectPtr<UTextBlock> IntroText;
	UPROPERTY() TObjectPtr<UTextBlock> CutText;
	UPROPERTY() TObjectPtr<UBorder> ScorePanel;
	UPROPERTY() TObjectPtr<UVerticalBox> ScoreList;
	// One entry per score row, by board slot.
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ScoreNames;
	UPROPERTY() TArray<TObjectPtr<USizeBox>> ScoreBars;
	UPROPERTY() TArray<TObjectPtr<UBorder>> ScoreFills;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ScoreValues;
};

// Match HUD: shows UTTABuildHUDWidget in every phase but Podium (with its premiere card and
// score board in Premiere). In Podium it adds the results + podium widgets.
UCLASS()
class TAKETWOAFTERLIFE_API ATTABuildHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void ShowResults();
	// Creates Overlay on first use and fills it from the game state and pawn.
	void UpdateOverlay(const ATTAGameState& State);
	// Now-showing card, and Kahoot-style bars during each cut's Reveal (ATTACinema::GetScoreBoard).
	void UpdatePremiere(bool bPremiere);

	UPROPERTY() TObjectPtr<UTTABuildHUDWidget> Overlay;
	UPROPERTY() TObjectPtr<UTTAResultsWidget> Results;
	UPROPERTY() TObjectPtr<UTTAPodiumWidget> PodiumWidget;
};
