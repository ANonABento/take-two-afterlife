#include "TTAGameState.h"

#include "Net/UnrealNetwork.h"

static_assert(static_cast<int>(ETTAPhase::Podium) == static_cast<int>(tta::Phase::Podium), "ETTAPhase must mirror tta::Phase");

void ATTAGameState::SetPhase(ETTAPhase NewPhase, int32 NewRound)
{
	check(HasAuthority());
	Phase = NewPhase;
	Round = NewRound;
	NotifyPhaseChanged();
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
}
