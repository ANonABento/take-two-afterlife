#include "TTATimelineWidget.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "tta/Replay.h"

namespace
{
const FLinearColor kStrip(0.12f, 0.12f, 0.16f);
const FLinearColor kClip(0.30f, 0.62f, 0.95f);
const FLinearColor kClipSelected(1.0f, 0.82f, 0.25f);
const FLinearColor kOverlay(1.f, 1.f, 1.f);
const FLinearColor kPlayhead(1.f, 0.25f, 0.35f);

FLinearColor MarkerColor(uint8 Type)
{
	switch (static_cast<tta::ReplayEventType>(Type))
	{
	case tta::ReplayEventType::Hit: return FLinearColor(1.f, 0.6f, 0.1f);
	case tta::ReplayEventType::Wipeout: return FLinearColor(1.f, 0.15f, 0.15f);
	case tta::ReplayEventType::Smite: return FLinearColor(1.f, 1.f, 0.3f);
	}
	return FLinearColor::White;
}

void Box(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, float X, float Y, float W, float H, const FLinearColor& C)
{
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2D(FMath::Max(W, 1.f), H), FSlateLayoutTransform(FVector2D(X, Y))),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, C);
}
}

void UTTATimelineWidget::SetData(const tta::EditList* InCut, uint32 InRunMs, TArray<TPair<uint32, uint8>> InMarkers)
{
	Cut = InCut;
	RunMs = FMath::Max<uint32>(InRunMs, 1);
	Markers = MoveTemp(InMarkers);
}

int32 UTTATimelineWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, G, Culling, Out, Layer, Style, bParentEnabled);
	const float W = G.GetLocalSize().X;
	const float CutY = StripHeight + 8.f;
	Box(Out, ++Layer, G, 0, 0, W, StripHeight, kStrip);
	Box(Out, Layer, G, 0, CutY, W, StripHeight, kStrip);
	++Layer;
	for (const TPair<uint32, uint8>& M : Markers)
	{
		Box(Out, Layer, G, W * M.Key / RunMs - 1.f, 0, 3.f, StripHeight, MarkerColor(M.Value));
	}
	if (!Cut) return Layer;
	// The cut strip spans the 30 s cap, so the space left is visible.
	const float PxPerMs = W / tta::kMaxCutMs;
	float X = 0;
	for (int32 i = 0; i < static_cast<int32>(Cut->clips().size()); ++i)
	{
		const float ClipW = Cut->clips()[i].durationMs() * PxPerMs;
		Box(Out, Layer, G, X + 1.f, CutY + 2.f, ClipW - 2.f, StripHeight - 4.f, i == Selected ? kClipSelected : kClip);
		X += ClipW;
	}
	++Layer;
	for (const tta::Overlay& O : Cut->overlays())
	{
		Box(Out, Layer, G, O.atMs * PxPerMs, CutY + StripHeight - 8.f, FMath::Max(O.durationMs * PxPerMs, 2.f), 6.f, kOverlay);
	}
	Box(Out, Layer, G, Playhead * PxPerMs - 1.f, CutY - 4.f, 2.f, StripHeight + 8.f, kPlayhead);
	return Layer;
}

FReply UTTATimelineWidget::NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{
	const FVector2D Local = G.AbsoluteToLocal(Event.GetScreenSpacePosition());
	const float W = FMath::Max(G.GetLocalSize().X, 1.f);
	if (Local.Y < StripHeight)
	{
		OnReplayClicked.ExecuteIfBound(static_cast<uint32>(FMath::Clamp(Local.X / W, 0.f, 1.f) * RunMs));
		return FReply::Handled();
	}
	if (!Cut) return FReply::Unhandled();
	const uint32 CutMs = static_cast<uint32>(FMath::Clamp(Local.X / W, 0.f, 1.f) * tta::kMaxCutMs);
	uint32 Start = 0;
	for (int32 i = 0; i < static_cast<int32>(Cut->clips().size()); ++i)
	{
		const uint32 Len = Cut->clips()[i].durationMs();
		if (CutMs < Start + Len)
		{
			OnCutClicked.ExecuteIfBound(i, CutMs);
			return FReply::Handled();
		}
		Start += Len;
	}
	return FReply::Handled();
}
