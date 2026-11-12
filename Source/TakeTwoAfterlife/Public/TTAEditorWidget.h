#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "tta/EditList.h"
#include "TTAEditorWidget.generated.h"

class ATTAEditStage;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTTACutPlayer;
class UTTATimelineWidget;

DECLARE_DELEGATE_OneParam(FTTAIndexClicked, int32);

// A button that reports its index (camera thumbnails).
UCLASS()
class TAKETWOAFTERLIFE_API UTTAIndexButton : public UButton
{
	GENERATED_BODY()

public:
	int32 Index = 0;
	FTTAIndexClicked OnIndexClicked;
	void Bind() { OnClicked.AddDynamic(this, &UTTAIndexButton::Forward); }

private:
	UFUNCTION() void Forward() { OnIndexClicked.ExecuteIfBound(Index); }
};

// Edit-phase screen (plan req 6, task 21). Timeline with hit markers, camera
// thumbnails, live preview, trim / reorder / split + angle / slow-mo / zoom /
// sticker / caption / WASTED card, auto-edit, the phase timer, and Reels autosave
// after every change. When the timer runs out with an empty cut, auto-edit fills it.
// The widget tree is built in C++; task 24 skins it.
UCLASS()
class TAKETWOAFTERLIFE_API UTTAEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Cameras = the stage's views; HeavenCam indexes into them. ReelPath = autosave target.
	void Init(ATTAEditStage* InStage, int32 InHeavenCam, const FString& InReelPath);

	const tta::EditList& GetCut() const { return Cut; }
	UTTACutPlayer* GetPlayer() const { return Player; }

	// Edit ops on the selected clip. Each returns false (logged, cut unchanged) when Core rejects it.
	bool AutoEdit();
	bool AddClipAt(uint32 ReplayMs);
	bool TrimIn();   // selected clip starts at the playhead
	bool TrimOut();  // selected clip ends at the playhead
	bool SplitHere();
	bool MoveSelected(int32 By);
	bool DeleteSelected();
	bool SetCamera(int32 Camera);
	bool CycleSlowMo();
	bool CycleZoom();
	bool AddOverlay(uint8 Type, const FString& Content);

	// Default sticker id for the Sticker button (task 24 adds a picker).
	UPROPERTY(EditAnywhere, Category = "TTA") FString DefaultSticker = TEXT("skull");
	UPROPERTY(EditAnywhere, Category = "TTA") uint32 NewClipMs = 3000;
	UPROPERTY(EditAnywhere, Category = "TTA") uint32 OverlayMs = 1500;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	template <typename Fn> bool Apply(const TCHAR* What, Fn&& Op);
	void Changed();
	// Replay ms under the playhead in the selected clip; unset without a selection.
	TOptional<uint32> PlayheadReplayMs() const;
	uint32 ClipStartCutMs(int32 Clip) const;
	UButton* AddButton(UHorizontalBox* Row, const FString& Label);

	UFUNCTION() void OnAutoEdit() { AutoEdit(); }
	UFUNCTION() void OnPlay();
	UFUNCTION() void OnTrimIn() { TrimIn(); }
	UFUNCTION() void OnTrimOut() { TrimOut(); }
	UFUNCTION() void OnSplit() { SplitHere(); }
	UFUNCTION() void OnLeft() { MoveSelected(-1); }
	UFUNCTION() void OnRight() { MoveSelected(1); }
	UFUNCTION() void OnDelete() { DeleteSelected(); }
	UFUNCTION() void OnSlowMo() { CycleSlowMo(); }
	UFUNCTION() void OnZoom() { CycleZoom(); }
	UFUNCTION() void OnSticker();
	UFUNCTION() void OnCaption();
	UFUNCTION() void OnWasted();

	tta::EditList Cut;
	int32 Selected = INDEX_NONE;
	int32 HeavenCam = 0;
	FString ReelPath;
	bool bTimeoutHandled = false;

	UPROPERTY() TObjectPtr<ATTAEditStage> Stage;
	UPROPERTY() TObjectPtr<UTTACutPlayer> Player;
	UPROPERTY() TObjectPtr<UTTATimelineWidget> Timeline;
	UPROPERTY() TObjectPtr<UImage> Preview;
	UPROPERTY() TObjectPtr<UHorizontalBox> Thumbs;
	UPROPERTY() TObjectPtr<UTextBlock> TimerText;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UEditableTextBox> CaptionBox;
};
