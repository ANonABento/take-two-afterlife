#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "tta/MatchStateMachine.h"
#include "tta/Placement.h"
#include "tta/Scoring.h"

#include <optional>
#include "TTAMatchCarry.generated.h"

// Server match state that outlives the game mode across map travel (Backyard <-> Heaven,
// Heaven -> Lobby). ATTAGameMode saves it right before ServerTravel and takes it back in InitGame.
UCLASS()
class TAKETWOAFTERLIFE_API UTTAMatchCarry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Save(const tta::MatchStateMachine& Match, const tta::MatchScore& Score, const tta::Layout& Layout);
	// False (outputs untouched) when nothing was saved. Consumes the saved state.
	bool Take(tta::MatchStateMachine& OutMatch, tta::MatchScore& OutScore, tta::Layout& OutLayout);

private:
	std::optional<tta::MatchStateMachine> Match;
	tta::MatchScore Score;
	tta::Layout Layout;
};
