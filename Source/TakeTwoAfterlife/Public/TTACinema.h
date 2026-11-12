#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "tta/CutPlayback.h"
#include "tta/Premiere.h"

#include <map>
#include <memory>

#include "TTACinema.generated.h"

class UMaterialParameterCollection;
class UStaticMeshComponent;
class UTTACutPlayer;
class UTTAMicPlayer;

// One cut to premiere: who made it, who it stars, the edit and the runner's mic file
// (empty or unreadable = silent playback).
struct FTTACinemaCut
{
	int32 CutId = 0;
	tta::Cut Cut;
	int32 Victim = 0;
	tta::EditList Edit;
	FString MicPath;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTTAOnPremiereStep, tta::PremiereStep);

// Heaven cinema (plan: Cinema): a walkable cloud, a 4-seat cloud couch facing a big
// screen, and the premiere sequence from Core tta::Premiere. The owner feeds it the
// round's cuts; it plays each on the screen with the runner's mic, opens the 10 s
// rating screen, reveals the averages and, after the final round, shows the podium.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API ATTACinema : public AActor
{
	GENERATED_BODY()

public:
	static constexpr int32 NumSeats = 4;

	ATTACinema();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Seat 0-3 on the couch, world space, facing the screen.
	FTransform GetSeatTransform(int32 Seat) const;

	// Starts a premiere in seeded random order. Podium = MatchScore::podium() after the
	// final round's scores; ignored when bFinalRound is false. Throws like tta::Premiere.
	void StartPremiere(const TArray<FTTACinemaCut>& Cuts, uint32 Seed, bool bFinalRound, const TArray<int32>& Podium);
	// A 1-5 Funny / Evil vote during the rating screen; false (logged) when rejected.
	bool Rate(int32 Rater, int32 Funny, int32 Evil);
	// Advances the sequence; Tick calls this, tests call it directly.
	void Step(float DeltaSeconds);

	const tta::Premiere* GetPremiere() const { return Premiere.get(); }
	// What the big screen shows now; unset outside playback.
	TOptional<tta::CutFrame> GetScreenFrame() const;
	// Narrator intro of the current cut's director and victim.
	FText GetIntroLine() const;
	// Players by total, set once the final premiere reaches the podium.
	const TArray<int32>& GetPodium() const { return ShownPodium; }

	FTTAOnPremiereStep OnStep;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cloud;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Couch;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Screen;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTTAMicPlayer> Mic;
	UPROPERTY(Config) TSoftObjectPtr<UMaterialParameterCollection> WorldCollection;

private:
	void EnterStep();

	std::unique_ptr<tta::Premiere> Premiere;
	std::map<int, FTTACinemaCut> CutsById;
	TArray<int32> FinalPodium;
	TArray<int32> ShownPodium;
	UPROPERTY() TObjectPtr<UTTACutPlayer> Player;
	int32 LastClip = -1;
	bool bMicLoaded = false;
};
