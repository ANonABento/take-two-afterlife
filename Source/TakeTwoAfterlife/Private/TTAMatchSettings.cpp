#include "TTAMatchSettings.h"

tta::MatchSettings UTTAMatchSettings::ToCore() const
{
	const FString Json = FString::Printf(
		TEXT("{\"rounds\": %d, \"buildSeconds\": %d, \"runSeconds\": %d, \"editSeconds\": %d, \"introSeconds\": %d, \"ratingSeconds\": %d, \"revealSeconds\": %d}"),
		Rounds, BuildSeconds, RunSeconds, EditSeconds, IntroSeconds, RatingSeconds, RevealSeconds);
	return tta::parseMatchSettings(TCHAR_TO_UTF8(*Json));
}
