#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "TTAGameState.h"
#include "TTAPlacementMarker.h"
#include "tta/MatchFlow.h"
#include "tta/MatchStateMachine.h"
#include "tta/Scoring.h"

#include <map>
#include <vector>
#include "TTAGameMode.generated.h"

class ATTABuildPawn;
class ATTARunDirector;
class ATTARunnerPawn;
class ATTATrapActor;

// Match player controller. Title uses ATTATitlePlayerController instead.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAPlayerController : public APlayerController
{
	GENERATED_BODY()
};

// Server-side owner of the match state machine; mirrors each accepted phase
// into ATTAGameState so it replicates to every client.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API ATTAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTAGameMode();

	// Returns false (and changes nothing) for an illegal transition.
	// Premiere travels to HeavenMap and a later round's Build back to BackyardMap (seamless,
	// match state kept in UTTAMatchCarry). Lobby (after the Podium) resets scores, keeps the
	// players and travels to the session's lobby map with the same room.
	// Entering Build gives every player an ATTABuildPawn. Entering Run reveals all placements,
	// gives every player a runner at the PlayerStart, spawns the traps and starts the recording.
	// Run ends (scored, then Edit) when everyone is through the gate or SmiteHoldSeconds after the timer.
	// Build and Edit move on (to Run / Premiere) when their timer runs out.
	UFUNCTION(BlueprintCallable) bool RequestPhase(ETTAPhase To);

	// The cinema finished this round's Premiere: adds its rating points (PlayerState ids),
	// then starts the next round's Build, or the Podium after the last round. False outside Premiere.
	bool EndPremiere(const std::map<int, double>& RatingPoints);

	// Timings from DefaultGame.ini (UTTAMatchSettings); code defaults if the ini is invalid.
	static tta::MatchSettings IniSettings();
	// Host settings (plan: Network). Loaded from SettingsPath() in InitGame (written with defaults
	// when missing) and mirrored to ATTAGameState. Changes only in the Lobby; saved back to the file.
	// False (nothing changes) outside the Lobby or on JSON tta::parseMatchSettings rejects.
	UFUNCTION(BlueprintCallable) bool ApplySettingsJson(const FString& Json);
	static FString SettingsPath();
	const tta::MatchSettings& GetSettings() const { return Match.settings(); }

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	// After seamless travel: a Build that arrived in the Backyard brings back markers and build pawns.
	virtual void PostSeamlessTravel() override;
	// Joining is only allowed between rounds (Lobby, Premiere, Podium).
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

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
	// Maps from DefaultGame.ini [/Script/TakeTwoAfterlife.TTAGameMode].
	UPROPERTY(Config) FString BackyardMap;
	UPROPERTY(Config) FString HeavenMap;
	UPROPERTY(EditDefaultsOnly, Category = "TTA") FTransform BuildCameraStart = FTransform(FRotator(-30.f, 0.f, 0.f), FVector(-500.f, 0.f, 800.f));

private:
	// Saves the match to UTTAMatchCarry and server-travels. False (logged) when Map isn't a package.
	bool TravelTo(const FString& Map);
	bool ReturnToLobby();
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
	// Replay id (index) -> PlayerState id, this Run: runners, then absent trap owners (tta::runPlayers).
	std::vector<int> RunPlayerIds;
	int32 RunnerCount = 0;
	float TimedOutFor = 0.f;
};
