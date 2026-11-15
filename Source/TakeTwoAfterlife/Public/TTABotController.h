#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "tta/BotBrain.h"
#include "tta/BotRoute.h"
#include "tta/EditList.h"
#include "tta/PawnLife.h"

#include <memory>
#include <optional>
#include <vector>

#include "TTABotController.generated.h"

class ATTACinema;

// Play Solo bot (MVP req 3). Possesses an ATTARunnerPawn like a player and makes every
// choice through Core: tta::BotBrain builds and rates, tta::BotRoute steers marker to marker
// toward the ATTAFinishGate, auto-edit fills its cut. A stuck bot is hit, so it respawns at
// the start through the pawn's tta::PawnLife like anyone else.
UCLASS()
class TAKETWOAFTERLIFE_API ATTABotController : public AAIController
{
	GENERATED_BODY()

public:
	// Actor tag on the Backyard route markers (Scripts/backyard_layout.py ROUTE_TAG).
	static const FName RouteTag;
	// Bot player ids start here, so a bot keeps its id (and score) when the map travels.
	static constexpr int32 FirstBotId = 1000;
	// The Heaven Cam films the whole bot cut, as in the editor when no camera is placed.
	static constexpr int32 HeavenCam = 0;

	ATTABotController();

	// Needs the PlayerState (spawned with the controller); sets its id.
	void InitBrain(int32 PlayerId, uint64 Seed);
	// Places this round's traps and cameras once; later calls this round do nothing.
	void BuildTraps(tta::Layout& Layout);
	// Starts steering along RouteCm (world cm, start to gate) with a fresh run plan.
	void StartRun(const TArray<FVector>& RouteCm);
	void StopRun();
	// One steering step for the possessed runner; Tick calls this.
	void DriveRunner(float DeltaSeconds);
	// Auto-edit of this Run's replay becomes the bot's cut.
	void FillCut(const tta::Replay& Replay);
	// Votes on the cinema's current cut during its rating screen. False outside Rating or on own cuts.
	bool RateCurrent(ATTACinema* Cinema);

	// Route markers sorted along +X (the Backyard runs start to gate along X), then the finish gate.
	static TArray<FVector> FindRoute(UWorld* World);

	bool IsRunning() const { return Route != nullptr; }
	const tta::EditList* GetCut() const { return Cut ? &*Cut : nullptr; }
	const tta::RunnerPlan& GetPlan() const { return Plan; }

	virtual void Tick(float DeltaSeconds) override;

	// How far ahead of the runner the floor probe looks for a gap.
	UPROPERTY(EditAnywhere, Category = "TTA") float ProbeAheadCm = 120.f;

private:
	std::unique_ptr<tta::BotBrain> Brain;
	std::unique_ptr<tta::BotRoute> Route;
	std::optional<tta::EditList> Cut;
	tta::RunnerPlan Plan;
	bool bBuiltThisRound = false;
	bool bWasGroundAhead = true;
	bool bSkipThisGap = false;
	float LogClock = 0.f;
	tta::LifeState LastLife = tta::LifeState::Alive;
};
