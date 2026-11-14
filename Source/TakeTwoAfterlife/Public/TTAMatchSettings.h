#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "tta/MatchSettings.h"
#include "TTAMatchSettings.generated.h"

// Phase timings from Config/DefaultGame.ini [/Script/TakeTwoAfterlife.TTAMatchSettings].
// Defaults mirror tta::MatchSettings; a host's saved MatchSettings.json still overrides these.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "TTA Match Settings"))
class TAKETWOAFTERLIFE_API UTTAMatchSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere) int32 Rounds = tta::MatchSettings{}.rounds;
	UPROPERTY(Config, EditAnywhere) int32 BuildSeconds = tta::MatchSettings{}.buildSeconds;
	UPROPERTY(Config, EditAnywhere) int32 RunSeconds = tta::MatchSettings{}.runSeconds;
	UPROPERTY(Config, EditAnywhere) int32 EditSeconds = tta::MatchSettings{}.editSeconds;
	UPROPERTY(Config, EditAnywhere) int32 IntroSeconds = tta::MatchSettings{}.introSeconds;
	UPROPERTY(Config, EditAnywhere) int32 RatingSeconds = tta::MatchSettings{}.ratingSeconds;
	UPROPERTY(Config, EditAnywhere) int32 RevealSeconds = tta::MatchSettings{}.revealSeconds;

	// Checked with tta::parseMatchSettings' ranges; throws std::invalid_argument on a bad value.
	tta::MatchSettings ToCore() const;
};
