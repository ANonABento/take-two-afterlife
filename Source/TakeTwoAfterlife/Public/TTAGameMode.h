#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TTAGameState.h"
#include "TTAPlacementMarker.h"
#include "tta/MatchStateMachine.h"
#include "tta/Scoring.h"
#include "TTAGameMode.generated.h"

class ATTABuildPawn;
class ATTARunDirector;
class ATTARunnerPawn;
class ATTATrapActor;

// Server-side owner of the match state machine; mirrors each accepted phase
// into ATTAGameState so it replicates to every client.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTAGameMode();

	// Returns false (and changes nothing) for an illegal transition.
	// Entering Build gives every player an ATTABuildPawn. Entering Run reveals all placements,
	// gives every player a runner at the PlayerStart, spawns the traps and starts the recording.
	// Run ends (scored, then Edit) when everyone is through the gate or SmiteHoldSeconds after the timer.
	UFUNCTION(BlueprintCallable) bool RequestPhase(ETTAPhase To);

	// Build requests (server). Each needs the Build phase and a player with a PlayerState;
	// settings and removal only touch the caller's own placements (removal: this round's only).
	ETTAPlaceOutcome PlaceItem(const AController* By, int32 Item, const FVector& WorldCm, float YawDeg, int32* OutId = nullptr);
	ETTAPlaceOutcome SetTrapSetting(const AController* By, int32 Id, int32 Index, int32 Value);
	ETTAPlaceOutcome RemovePlacement(const AController* By, int32 Id);
	const tta::Layout& GetLayout() const { return Layout; }
	ATTAPlacementMarker* FindMarker(int32 Id) const;

	// Match totals by PlayerState id.
	const tta::MatchScore& GetScore() const { return Score; }
	ATTARunDirector* GetRunDirector() const { return Director; }

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category = "TTA") TSubclassOf<ATTABuildPawn> BuildPawnClass;
	UPROPERTY(EditDefaultsOnly, Category = "TTA") TSubclassOf<ATTARunnerPawn> RunnerPawnClass;
	// Time the Heaven Cam holds on the smite before Run ends.
	UPROPERTY(EditDefaultsOnly, Category = "TTA") float SmiteHoldSeconds = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "TTA") FTransform BuildCameraStart = FTransform(FRotator(-30.f, 0.f, 0.f), FVector(-500.f, 0.f, 800.f));

private:
	void EnterBuild();
	void EnterRun();
	void SpawnTraps();
	void EndRun();
	void RevealPlacements();
	// Ok when By owns placement Id; fills OutPlacement.
	ETTAPlaceOutcome CheckOwned(const AController* By, int32 Id, const tta::Placement*& OutPlacement) const;
	void SyncMarker(int32 Id);

	tta::Layout Layout; // persists across rounds, like the map
	UPROPERTY() TMap<int32, TObjectPtr<ATTAPlacementMarker>> Markers;
	tta::MatchStateMachine Match{tta::MatchSettings{}};
	tta::MatchScore Score;
	UPROPERTY() TObjectPtr<ATTARunDirector> Director;
	UPROPERTY() TMap<int32, TObjectPtr<ATTATrapActor>> TrapActors; // by placement id; persist like the map
	TArray<int32> RunnerPlayerIds; // replay id (index) -> PlayerState id, this Run
	float TimedOutFor = 0.f;
};
