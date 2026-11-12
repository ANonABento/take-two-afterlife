#include "TTAGameMode.h"

ATTAGameMode::ATTAGameMode()
{
	GameStateClass = ATTAGameState::StaticClass();
}

bool ATTAGameMode::RequestPhase(ETTAPhase To)
{
	if (!Match.transition(ToCore(To)))
	{
		return false;
	}
	GetGameState<ATTAGameState>()->SetPhase(ToUE(Match.phase()), Match.round());
	return true;
}
