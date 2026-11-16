#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "tta/BotBrain.h"
#include "tta/BotRace.h"
#include "tta/EditList.h"
#include "tta/PawnLife.h"

#include <memory>
#include <optional>

#include "TTABotController.generated.h"

class ATTACinema;

// Play Solo bot (MVP req 3). Possesses an ATTARunnerPawn like a player and makes every
// choice through Core: tta::BotBrain builds and rates, tta::BotRace runs for the finish
// gate (dodging traps, punching rivals), auto-edit fills its cut. A stuck bot is hit, so
// it respawns at the start through the pawn's tta::PawnLife like anyone else.
UCLASS()
class TAKETWOAFTERLIFE_API ATTABotController : public AAIController
{
	GENERATED_BODY()

public:
	// Bot player ids start here, so a bot keeps its id (and score) when the map travels.
	static constexpr int32 FirstBotId = 1000;
	// The Heaven Cam films the whole bot cut, as in the editor when no camera is placed.
	static constexpr int32 HeavenCam = 0;

	ATTABotController();

	// Needs the PlayerState (spawned with the controller); sets its id.
	void InitBrain(int32 PlayerId, uint64 Seed);
	// Places this round's traps and cameras once; later calls this round do nothing.
	void BuildTraps(tta::Layout& Layout);
	// Starts racing toward GateCm (world cm) with a fresh run plan.
	void StartRun(const FVector& GateCm);
	void StopRun();
	// One steering step for the possessed runner; Tick calls this.
	void DriveRunner(float DeltaSeconds);
	// Auto-edit of this Run's replay becomes the bot's cut.
	void FillCut(const tta::Replay& Replay);
	// Votes on the cinema's current cut during its rating screen. False outside Rating or on own cuts.
	bool RateCurrent(ATTACinema* Cinema);

	bool IsRunning() const { return bRacing; }
	const tta::EditList* GetCut() const { return Cut ? &*Cut : nullptr; }
	const tta::RunnerPlan& GetPlan() const { return Plan; }

	virtual void Tick(float DeltaSeconds) override;

	// Build phase only: how close counts as at a spot, and how long the bot stands there.
	UPROPERTY(EditAnywhere, Category = "TTA") float BuildArriveCm = 80.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float BuildPauseSeconds = 0.6f;

	// How far ahead of the runner the floor probe looks for a gap.
	UPROPERTY(EditAnywhere, Category = "TTA") float ProbeAheadCm = 120.f;

private:
	std::unique_ptr<tta::BotBrain> Brain;
	std::unique_ptr<tta::BotRace> Race;
	FVector GateCm = FVector::ZeroVector;
	bool bRacing = false;
	std::optional<tta::EditList> Cut;
	tta::RunnerPlan Plan;
	// Walks the runner to each of its own placements in turn so building reads on screen.
	void WalkBuildStops(float DeltaSeconds);
	TArray<FVector> BuildStops;
	int32 BuildStop = 0;
	float BuildPause = 0.f;
	bool bBuiltThisRound = false;
	bool bWasGroundAhead = true;
	bool bSkipThisGap = false;
	float LogClock = 0.f;
	tta::LifeState LastLife = tta::LifeState::Alive;
};
