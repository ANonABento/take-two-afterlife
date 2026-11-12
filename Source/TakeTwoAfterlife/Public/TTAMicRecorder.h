#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/UniquePtr.h"
#include "tta/MicTrack.h"
#include "TTAMicRecorder.generated.h"

class IVoiceCapture;
enum class ETTAPhase : uint8;

// Records the owning runner's mic during Run to a WAV on the replay timeline
// (plan req 10): sample 0 = replay time 0. Starts on entering Run and saves to
// FilePath(Round, PlayerId) on leaving it. Only the locally controlled runner
// records; with no mic it records nothing and playback stays silent.
UCLASS(ClassGroup = TTA, meta = (BlueprintSpawnableComponent))
class TAKETWOAFTERLIFE_API UTTAMicRecorder : public UActorComponent
{
	GENERATED_BODY()

public:
	UTTAMicRecorder();

	// Saved/Mic/Round<R>_Player<P>.wav
	static FString FilePath(int32 Round, int32 PlayerId);

	// Device buffering not seen by the capture API, subtracted from each chunk's stamp.
	UPROPERTY(Config, EditAnywhere, Category = "TTA") float MicLatencyMs = 0.f;

	// Adds a captured chunk whose last sample was captured at ReplayMs.
	void AppendCaptured(TConstArrayView<int16> Pcm, double ReplayMs);
	// Writes the track as WAV; false (logged) when the write fails.
	bool Save(const FString& Path) const;
	void Reset();
	const tta::MicTrack& GetTrack() const { return Track; }
	bool IsRecording() const { return Capture.IsValid(); }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void OnPhaseChanged(ETTAPhase Phase, int32 Round);
	void StartCapture(int32 Round);
	void StopAndSave();
	void Poll();
	double ReplayMs() const;

	tta::MicTrack Track;
	TSharedPtr<IVoiceCapture> Capture;
	TArray<uint8> Buffer;
	int32 RecordingRound = INDEX_NONE;
	FDelegateHandle PhaseHandle;
};
