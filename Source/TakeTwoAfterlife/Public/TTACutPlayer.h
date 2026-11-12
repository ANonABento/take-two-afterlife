#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "tta/CutPlayback.h"
#include "TTACutPlayer.generated.h"

// Plays a Core cut on its own timeline (editor preview now, cinema in task 22).
// Engine-light: the owner calls Advance each tick and reads GetFrame to render.
UCLASS()
class TAKETWOAFTERLIFE_API UTTACutPlayer : public UObject
{
	GENERATED_BODY()

public:
	void SetCut(const tta::EditList& InCut);
	const tta::EditList& GetCut() const { return Cut; }

	void Play();
	void Stop() { bPlaying = false; }
	void Seek(uint32 CutMs);
	// Moves the playhead; stops at the cut's end. Returns true while still playing.
	bool Advance(float DeltaSeconds);

	bool IsPlaying() const { return bPlaying; }
	uint32 GetCutMs() const { return static_cast<uint32>(FMath::RoundToInt64(PlayheadMs)); }
	// Cut time actually played since the last Play() (the PlayCut check).
	double GetPlayedMs() const { return PlayedMs; }
	// Unset at or after the end of the cut.
	TOptional<tta::CutFrame> GetFrame() const;

private:
	tta::EditList Cut;
	double PlayheadMs = 0.0;
	double PlayedMs = 0.0;
	bool bPlaying = false;
};
