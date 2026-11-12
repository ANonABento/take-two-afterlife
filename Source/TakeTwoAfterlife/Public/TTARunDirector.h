#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/UniquePtr.h"
#include "tta/RunRecorder.h"
#include "TTARunDirector.generated.h"

class ACameraActor;
class ATTARunnerPawn;

// Server-side Run recorder: samples every runner's root + 15 bones into the
// Core replay at 20 Hz, records trap hits, and on timeout smites each runner
// who hasn't reached the gate while a Heaven Cam films it.
// Player id in the replay = index in the Runners array passed to StartRun.
UCLASS()
class TAKETWOAFTERLIFE_API ATTARunDirector : public AActor
{
	GENERATED_BODY()

public:
	ATTARunDirector();

	// Server only. Trap owners must be among the runners' ids.
	void StartRun(const TArray<ATTARunnerPawn*>& InRunners, float RunSeconds);
	void AdvanceRun(float DeltaSeconds);
	void MarkFinished(ATTARunnerPawn* Runner);
	void RecordHit(const tta::Placement& Trap, ATTARunnerPawn* Victim);

	bool IsRunning() const { return Recorder.IsValid(); }
	// Valid only after StartRun.
	const tta::Replay& GetReplay() const { return Recorder->replay(); }
	int32 PlayerId(const ATTARunnerPawn* Runner) const { return Runners.IndexOfByKey(Runner); }
	ACameraActor* GetHeavenCam() const { return HeavenCam; }

	// Heaven Cam offset from the victim, in the victim's yaw frame (cm).
	UPROPERTY(EditAnywhere, Category = "TTA") FVector HeavenCamOffset = FVector(-400.f, 0.f, 600.f);

	virtual void Tick(float DeltaSeconds) override;

private:
	void CaptureFrame();
	void Smite(ATTARunnerPawn* Victim);

	TUniquePtr<tta::RunRecorder> Recorder;
	UPROPERTY() TArray<TObjectPtr<ATTARunnerPawn>> Runners;
	UPROPERTY() TArray<TObjectPtr<ATTARunnerPawn>> Finished;
	UPROPERTY() TObjectPtr<ACameraActor> HeavenCam;
};
