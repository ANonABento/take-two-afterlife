#include "TTACutPlayer.h"

void UTTACutPlayer::SetCut(const tta::EditList& InCut)
{
	Cut = InCut;
	PlayheadMs = FMath::Min<double>(PlayheadMs, Cut.durationMs());
}

void UTTACutPlayer::Play()
{
	if (PlayheadMs >= Cut.durationMs()) PlayheadMs = 0.0;
	PlayedMs = 0.0;
	bPlaying = Cut.durationMs() > 0;
}

void UTTACutPlayer::Seek(uint32 CutMs)
{
	PlayheadMs = FMath::Min<double>(CutMs, Cut.durationMs());
}

bool UTTACutPlayer::Advance(float DeltaSeconds)
{
	if (!bPlaying) return false;
	const double End = Cut.durationMs();
	const double Step = FMath::Min(DeltaSeconds * 1000.0, End - PlayheadMs);
	PlayheadMs += Step;
	PlayedMs += Step;
	bPlaying = PlayheadMs < End;
	return bPlaying;
}

TOptional<tta::CutFrame> UTTACutPlayer::GetFrame() const
{
	const std::optional<tta::CutFrame> Frame = tta::cutFrameAt(Cut, GetCutMs());
	return Frame ? TOptional<tta::CutFrame>(*Frame) : TOptional<tta::CutFrame>();
}
