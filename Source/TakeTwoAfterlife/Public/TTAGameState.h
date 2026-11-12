#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "tta/MatchStateMachine.h"
#include "TTAGameState.generated.h"

// Mirrors tta::Phase for Blueprint/UI and replication. Order must match.
UENUM(BlueprintType)
enum class ETTAPhase : uint8
{
	Lobby,
	Build,
	Run,
	Edit,
	Premiere,
	Podium
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTTAPhaseChangedSignature, ETTAPhase, Phase, int32, Round);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTTAPhaseChangedNative, ETTAPhase, int32);

// Replicated match phase and round (plan req 9). Only the server writes them,
// through ATTAGameMode; clients get the change from the RepNotify.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure) ETTAPhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure) int32 GetRound() const { return Round; }
	// Whole seconds left in a timed phase (Build/Run/Edit), 0 when untimed or over.
	UFUNCTION(BlueprintPure) int32 GetSecondsLeft() const;
	// Seconds since the current phase started, on the server clock. In Run this is replay time.
	UFUNCTION(BlueprintPure) double GetPhaseElapsedSeconds() const { return GetServerWorldTimeSeconds() - PhaseStartedAt; }

	// Server only. Notifies locally; clients notify in OnRep_Phase.
	// PhaseSeconds 0 = untimed.
	void SetPhase(ETTAPhase NewPhase, int32 NewRound, int32 PhaseSeconds = 0);

	// Blueprint/UI listeners.
	UPROPERTY(BlueprintAssignable) FTTAPhaseChangedSignature OnPhaseChanged;
	// C++ listeners.
	FTTAPhaseChangedNative OnPhaseChangedNative;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION() void OnRep_Phase();

	UPROPERTY(ReplicatedUsing = OnRep_Phase) ETTAPhase Phase = ETTAPhase::Lobby;
	UPROPERTY(ReplicatedUsing = OnRep_Phase) int32 Round = 0;
	// Server world time when the current phase's timer runs out; < 0 when untimed.
	UPROPERTY(Replicated) double PhaseEndsAt = -1.0;
	// Server world time when the current phase started.
	UPROPERTY(Replicated) double PhaseStartedAt = 0.0;

private:
	void NotifyPhaseChanged();
};

inline ETTAPhase ToUE(tta::Phase Phase) { return static_cast<ETTAPhase>(Phase); }
inline tta::Phase ToCore(ETTAPhase Phase) { return static_cast<tta::Phase>(Phase); }
