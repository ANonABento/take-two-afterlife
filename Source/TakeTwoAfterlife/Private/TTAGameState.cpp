#include "TTAGameState.h"

#include "Net/UnrealNetwork.h"

static_assert(static_cast<int>(ETTAPhase::Podium) == static_cast<int>(tta::Phase::Podium), "ETTAPhase must mirror tta::Phase");

void ATTAGameState::SetPhase(ETTAPhase NewPhase, int32 NewRound, int32 PhaseSeconds)
{
	check(HasAuthority());
	Phase = NewPhase;
	Round = NewRound;
	PhaseStartedAt = GetServerWorldTimeSeconds();
	PhaseEndsAt = PhaseSeconds > 0 ? GetServerWorldTimeSeconds() + PhaseSeconds : -1.0;
	NotifyPhaseChanged();
}

void ATTAGameState::SetSettingsJson(const FString& Json)
{
	check(HasAuthority());
	SettingsJson = Json;
}

int32 ATTAGameState::GetSecondsLeft() const
{
	return PhaseEndsAt < 0.0 ? 0 : FMath::Max(0, FMath::CeilToInt32(PhaseEndsAt - GetServerWorldTimeSeconds()));
}

void ATTAGameState::OnRep_Phase()
{
	NotifyPhaseChanged();
}

void ATTAGameState::NotifyPhaseChanged()
{
	OnPhaseChanged.Broadcast(Phase, Round);
	OnPhaseChangedNative.Broadcast(Phase, Round);
}

void ATTAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATTAGameState, Phase);
	DOREPLIFETIME(ATTAGameState, Round);
	DOREPLIFETIME(ATTAGameState, PhaseEndsAt);
	DOREPLIFETIME(ATTAGameState, PhaseStartedAt);
	DOREPLIFETIME(ATTAGameState, SettingsJson);
}
