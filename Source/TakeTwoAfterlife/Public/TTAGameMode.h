#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TTAGameState.h"
#include "tta/MatchStateMachine.h"
#include "TTAGameMode.generated.h"

// Server-side owner of the match state machine; mirrors each accepted phase
// into ATTAGameState so it replicates to every client.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTAGameMode();

	// Returns false (and changes nothing) for an illegal transition.
	UFUNCTION(BlueprintCallable) bool RequestPhase(ETTAPhase To);

private:
	tta::MatchStateMachine Match{tta::MatchSettings{}};
};
