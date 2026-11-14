#include "TTAMatchCarry.h"

void UTTAMatchCarry::Save(const tta::MatchStateMachine& InMatch, const tta::MatchScore& InScore, const tta::Layout& InLayout)
{
	Match = InMatch;
	Score = InScore;
	Layout = InLayout;
}

bool UTTAMatchCarry::Take(tta::MatchStateMachine& OutMatch, tta::MatchScore& OutScore, tta::Layout& OutLayout)
{
	if (!Match) return false;
	OutMatch = *Match;
	OutScore = Score;
	OutLayout = Layout;
	Match.reset();
	Score = tta::MatchScore{};
	Layout = tta::Layout{};
	return true;
}
