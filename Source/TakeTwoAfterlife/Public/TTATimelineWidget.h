#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "tta/EditList.h"
#include "TTATimelineWidget.generated.h"

DECLARE_DELEGATE_OneParam(FTTATimelineReplayClick, uint32 /*ReplayMs*/);
DECLARE_DELEGATE_TwoParams(FTTATimelineCutClick, int32 /*Clip*/, uint32 /*CutMs*/);

// Two strips, painted directly:
//  top    = the whole Run (source footage) with hit markers (hit / wipeout / smite);
//           click = add a clip around that moment.
//  bottom = the cut, clips sized by their cut length, overlays as ticks, playhead;
//           click = select that clip and seek there.
UCLASS()
class TAKETWOAFTERLIFE_API UTTATimelineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetData(const tta::EditList* InCut, uint32 InRunMs, TArray<TPair<uint32, uint8>> InMarkers);
	void SetSelection(int32 Clip, uint32 PlayheadMs) { Selected = Clip; Playhead = PlayheadMs; }

	FTTATimelineReplayClick OnReplayClicked;
	FTTATimelineCutClick OnCutClicked;

	UPROPERTY(EditAnywhere, Category = "TTA") float StripHeight = 40.f;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	const tta::EditList* Cut = nullptr;
	uint32 RunMs = 1;
	TArray<TPair<uint32, uint8>> Markers; // replay ms, tta::ReplayEventType
	int32 Selected = INDEX_NONE;
	uint32 Playhead = 0;
};
