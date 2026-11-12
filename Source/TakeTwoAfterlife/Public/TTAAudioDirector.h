#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TTAGameState.h"
#include "tta/AudioCues.h"
#include "TTAAudioDirector.generated.h"

class UAudioComponent;
class USoundBase;

// Plays the tta::audioCues() table: phase music (crossfaded), narrator lines (one at a
// time, round-robin per event) and SFX. Follows ATTAGameState phase changes by itself;
// gameplay code calls PlayEvent for the rest.
UCLASS()
class TAKETWOAFTERLIFE_API UTTAAudioDirector : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Loads a cue's asset; logs an error and returns nullptr when it is missing.
	static USoundBase* LoadCue(const tta::AudioCue& Cue);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Plays the event's SFX and its next narrator line (and music for phase events).
	void PlayEvent(tta::AudioEvent Event);
	void PlayPhase(ETTAPhase Phase);

	static constexpr float MusicFadeSeconds = 1.0f;

private:
	void OnPhaseChanged(ETTAPhase Phase, int32 Round);

	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	UPROPERTY() TObjectPtr<UAudioComponent> Narrator;
	TMap<int32, int32> LinesPlayed;
};
