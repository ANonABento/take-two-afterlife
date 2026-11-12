#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "tta/MicTrack.h"
#include "TTAMicPlayer.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

// Plays a runner's recorded mic in sync with replay playback (editor, cinema).
// A missing or unreadable file leaves an empty track: playback is silent.
UCLASS(ClassGroup = TTA, meta = (BlueprintSpawnableComponent))
class TAKETWOAFTERLIFE_API UTTAMicPlayer : public UActorComponent
{
	GENERATED_BODY()

public:
	// True when the track loaded; false (logged) means silent playback.
	UFUNCTION(BlueprintCallable, Category = "TTA") bool Load(const FString& Path);
	// Starts (or seeks) audio at a replay time.
	UFUNCTION(BlueprintCallable, Category = "TTA") void PlayFrom(float ReplaySeconds);
	UFUNCTION(BlueprintCallable, Category = "TTA") void Stop();

	// Samples at a replay time, zero where nothing was recorded.
	TArray<int16> ReadAt(double ReplaySeconds, int32 Count) const;
	const tta::MicTrack& GetTrack() const { return Track; }

private:
	void Fill(USoundWaveProcedural* Wave, int32 SamplesRequired);

	tta::MicTrack Track;
	int64 Cursor = 0; // next sample to queue
	UPROPERTY() TObjectPtr<USoundWaveProcedural> Wave;
	UPROPERTY() TObjectPtr<UAudioComponent> Audio;
};
