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

	// Server only. Notifies locally; clients notify in OnRep_Phase.
	void SetPhase(ETTAPhase NewPhase, int32 NewRound);

	// Blueprint/UI listeners.
	UPROPERTY(BlueprintAssignable) FTTAPhaseChangedSignature OnPhaseChanged;
	// C++ listeners.
	FTTAPhaseChangedNative OnPhaseChangedNative;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION() void OnRep_Phase();

	UPROPERTY(ReplicatedUsing = OnRep_Phase) ETTAPhase Phase = ETTAPhase::Lobby;
	UPROPERTY(ReplicatedUsing = OnRep_Phase) int32 Round = 0;

private:
	void NotifyPhaseChanged();
};

inline ETTAPhase ToUE(tta::Phase Phase) { return static_cast<ETTAPhase>(Phase); }
inline tta::Phase ToCore(ETTAPhase Phase) { return static_cast<tta::Phase>(Phase); }
