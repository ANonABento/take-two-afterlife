#include "TTAGameMode.h"
#include "GameFramework/PlayerState.h"
#pragma warning(disable: 4458) // local Owner shadows AActor::Owner

#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "TTAMatchCarry.h"
#include "TTAPlayerState.h"
#include "TTASessionSubsystem.h"
#include "TTAMatchSettings.h"
#include "GameFramework/PlayerController.h"
#include "TTABuildHUD.h"
#include "TTABuildPawn.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "TTATrapActor.h"

#include <algorithm>

ATTAGameMode::ATTAGameMode()
{
	GameStateClass = ATTAGameState::StaticClass();
	HUDClass = ATTABuildHUD::StaticClass();
	PlayerStateClass = ATTAPlayerState::StaticClass();
	bUseSeamlessTravel = true;
	BuildPawnClass = ATTABuildPawn::StaticClass();
	RunnerPawnClass = ATTARunnerPawn::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

bool ATTAGameMode::RequestPhase(ETTAPhase To)
{
	if (To == ETTAPhase::Lobby) return ReturnToLobby();
	if (!Match.transition(ToCore(To)))
	{
		return false;
	}
	GetGameState<ATTAGameState>()->SetPhase(ToUE(Match.phase()), Match.round(), Match.phaseSeconds());
	// Round 1 is already in the Backyard; on a failed travel the round still goes on here.
	if (To == ETTAPhase::Build && (Match.round() == 1 || !TravelTo(BackyardMap))) EnterBuild();
	if (To == ETTAPhase::Run) EnterRun();
	if (To == ETTAPhase::Premiere) TravelTo(HeavenMap);
	return true;
}

bool ATTAGameMode::ReturnToLobby()
{
	std::vector<int> Players;
	for (const APlayerState* PS : GetGameState<ATTAGameState>()->PlayerArray)
	{
		if (PS) Players.push_back(PS->GetPlayerId());
	}
	if (!tta::returnToLobby(Match, Score, Players)) return false;
	Layout = tta::Layout{};
	GetGameState<ATTAGameState>()->SetPhase(ETTAPhase::Lobby, Match.round());
	// Seamless travel keeps every connection and the session, so the room code stays.
	TravelTo(GetGameInstance()->GetSubsystem<UTTASessionSubsystem>()->GetLobbyMap());
	return true;
}

bool ATTAGameMode::TravelTo(const FString& Map)
{
	if (Map.IsEmpty() || !FPackageName::DoesPackageExist(Map))
	{
		UE_LOG(LogTemp, Error, TEXT("ATTAGameMode: can't travel to '%s' (set the map in DefaultGame.ini [/Script/TakeTwoAfterlife.TTAGameMode])"), *Map);
		return false;
	}
	GetGameInstance()->GetSubsystem<UTTAMatchCarry>()->Save(Match, Score, Layout);
	return GetWorld()->ServerTravel(Map + TEXT("?listen"));
}

void ATTAGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	if (Match.phase() != tta::Phase::Build) return;
	for (const tta::Placement& P : Layout.all()) SyncMarker(P.id);
	EnterBuild();
}

tta::MatchSettings ATTAGameMode::IniSettings()
{
	try
	{
		return GetDefault<UTTAMatchSettings>()->ToCore();
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTemp, Error, TEXT("ATTAGameMode: [/Script/TakeTwoAfterlife.TTAMatchSettings] is invalid (%s); using code defaults"), UTF8_TO_TCHAR(Error.what()));
		return tta::MatchSettings{};
	}
}

FString ATTAGameMode::SettingsPath()
{
	return FPaths::ProjectSavedDir() / TEXT("TTA/MatchSettings.json");
}

void ATTAGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const tta::MatchSettings Base = IniSettings();
	Match = tta::MatchStateMachine(Base);
	const FString Path = SettingsPath();
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		Json = UTF8_TO_TCHAR(tta::toJson(Base).c_str());
		if (!FFileHelper::SaveStringToFile(Json, *Path))
		{
			UE_LOG(LogTemp, Error, TEXT("ATTAGameMode: can't write default settings to %s"), *Path);
		}
	}
	try
	{
		Match = tta::MatchStateMachine(tta::parseMatchSettings(TCHAR_TO_UTF8(*Json), Base));
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTemp, Error, TEXT("ATTAGameMode: %s is invalid (%s); using DefaultGame.ini settings"), *Path, UTF8_TO_TCHAR(Error.what()));
	}
	// Arrived by travel mid-match: the carried state wins over the settings file.
	GetGameInstance()->GetSubsystem<UTTAMatchCarry>()->Take(Match, Score, Layout);
}

void ATTAGameMode::InitGameState()
{
	Super::InitGameState();
	ATTAGameState* State = GetGameState<ATTAGameState>();
	State->SetSettingsJson(UTF8_TO_TCHAR(tta::toJson(Match.settings()).c_str()));
	if (Match.phase() != tta::Phase::Lobby) State->SetPhase(ToUE(Match.phase()), Match.round(), Match.phaseSeconds());
}

bool ATTAGameMode::ApplySettingsJson(const FString& Json)
{
	if (Match.phase() != tta::Phase::Lobby) return false;
	tta::MatchSettings Settings;
	try
	{
		Settings = tta::parseMatchSettings(TCHAR_TO_UTF8(*Json), IniSettings());
	}
	catch (const std::invalid_argument& Error)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATTAGameMode: settings rejected: %s"), UTF8_TO_TCHAR(Error.what()));
		return false;
	}
	Match = tta::MatchStateMachine(Settings);
	const FString Saved = UTF8_TO_TCHAR(tta::toJson(Settings).c_str());
	if (!FFileHelper::SaveStringToFile(Saved, *SettingsPath()))
	{
		UE_LOG(LogTemp, Error, TEXT("ATTAGameMode: can't save settings to %s"), *SettingsPath());
	}
	GetGameState<ATTAGameState>()->SetSettingsJson(Saved);
	return true;
}

void ATTAGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && !tta::canJoin(Match.phase()))
	{
		ErrorMessage = TEXT("Round in progress: join when this round's premiere starts.");
	}
}

bool ATTAGameMode::EndPremiere(const std::map<int, double>& RatingPoints)
{
	if (Match.phase() != tta::Phase::Premiere) return false;
	Score.addRatings(RatingPoints);
	return RequestPhase(Match.canTransition(tta::Phase::Build) ? ETTAPhase::Build : ETTAPhase::Podium);
}

void ATTAGameMode::EnterRun()
{
	RevealPlacements();
	TArray<ATTARunnerPawn*> Runners;
	std::vector<int> RunnerIds;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->PlayerState) continue;
		const AActor* Start = FindPlayerStart(PC);
		const FTransform StartTransform = Start ? Start->GetActorTransform() : FTransform::Identity;
		FActorSpawnParameters Params;
		Params.Owner = PC;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ATTARunnerPawn* Runner = GetWorld()->SpawnActor<ATTARunnerPawn>(RunnerPawnClass, StartTransform, Params);
		check(Runner);
		Runner->SetStartLocation(Runner->GetActorLocation()); // everyone respawns where they started
		Runner->SetForm(ETTAForm::Human);
		APawn* Old = PC->GetPawn();
		PC->Possess(Runner);
		if (Old) Old->Destroy();
		Runners.Add(Runner);
		RunnerIds.push_back(PC->PlayerState->GetPlayerId());
	}
	RunPlayerIds = tta::runPlayers(RunnerIds, Layout);
	RunnerCount = Runners.Num();
	SpawnTraps();
	if (!Director) Director = GetWorld()->SpawnActor<ATTARunDirector>();
	check(Director);
	TimedOutFor = 0.f;
	Director->StartRun(Runners, static_cast<float>(Match.phaseSeconds()), static_cast<int32>(RunPlayerIds.size()) - RunnerCount);
}

void ATTAGameMode::SpawnTraps()
{
	for (const tta::Placement& P : Layout.all())
	{
		if (P.isCamera) continue;
		// The replay knows players by index; owners who left are in RunPlayerIds too, so their traps stay live.
		const auto Owner = std::find(RunPlayerIds.begin(), RunPlayerIds.end(), P.owner);
		check(Owner != RunPlayerIds.end());
		const int32 OwnerIndex = static_cast<int32>(Owner - RunPlayerIds.begin());
		TObjectPtr<ATTATrapActor>& Trap = TrapActors.FindOrAdd(P.id);
		if (!Trap) Trap = GetWorld()->SpawnActor<ATTATrapActor>();
		check(Trap);
		tta::Placement InRun = P;
		InRun.owner = OwnerIndex;
		Trap->InitFromPlacement(InRun);
		Trap->SetActorHiddenInGame(false);
		Trap->SetActorEnableCollision(true);
	}
}

void ATTAGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const ATTAGameState* State = GetGameState<ATTAGameState>();
	const bool bTimerDone = Match.phaseSeconds() > 0 && State->GetSecondsLeft() == 0;
	if (State->GetPhase() == ETTAPhase::Build && bTimerDone) verify(RequestPhase(ETTAPhase::Run));
	if (State->GetPhase() == ETTAPhase::Edit && bTimerDone) verify(RequestPhase(ETTAPhase::Premiere));
	if (State->GetPhase() != ETTAPhase::Run || !Director || !Director->IsRunning()) return;
	if (Director->IsTimedOut()) TimedOutFor += DeltaSeconds;
	if (Director->AllFinished() || TimedOutFor >= SmiteHoldSeconds) EndRun();
}

void ATTAGameMode::EndRun()
{
	// Replay ids are indices into RunPlayerIds; totals are kept by PlayerState id.
	Score.addRound(tta::toPlayerIds(Director->GetRoundInput(), RunPlayerIds, static_cast<std::size_t>(RunnerCount)));
	verify(RequestPhase(ETTAPhase::Edit));
}

void ATTAGameMode::EnterBuild()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;
		FActorSpawnParameters Params;
		Params.Owner = PC;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATTABuildPawn* Pawn = GetWorld()->SpawnActor<ATTABuildPawn>(BuildPawnClass, BuildCameraStart, Params);
		check(Pawn);
		PC->Possess(Pawn);
	}
}

void ATTAGameMode::RevealPlacements()
{
	Layout.revealAll();
	for (const TPair<int32, TObjectPtr<ATTAPlacementMarker>>& Pair : Markers) SyncMarker(Pair.Key);
}

ATTAPlacementMarker* ATTAGameMode::FindMarker(int32 Id) const
{
	const TObjectPtr<ATTAPlacementMarker>* Found = Markers.Find(Id);
	return Found ? Found->Get() : nullptr;
}

void ATTAGameMode::SyncMarker(int32 Id)
{
	for (const tta::Placement& P : Layout.all())
	{
		if (P.id != Id) continue;
		ATTAPlacementMarker* Marker = FindMarker(Id);
		if (!Marker)
		{
			Marker = GetWorld()->SpawnActor<ATTAPlacementMarker>(ATTAPlacementMarker::StaticClass(), FTransform::Identity);
			check(Marker);
			Markers.Add(Id, Marker);
		}
		Marker->SetData(FTTAPlacementRep::FromCore(P));
		return;
	}
	checkf(false, TEXT("no placement %d"), Id);
}

ETTAPlaceOutcome ATTAGameMode::PlaceItem(const AController* By, int32 Item, const FVector& WorldCm, float YawDeg, int32* OutId)
{
	if (GetGameState<ATTAGameState>()->GetPhase() != ETTAPhase::Build) return ETTAPlaceOutcome::WrongPhase;
	if (Item < 0 || Item >= TTABuildItem::Count()) return ETTAPlaceOutcome::BadItem;
	const int32 Owner = ATTAPlacementMarker::PlayerIdOf(By);
	if (Owner == INDEX_NONE) return ETTAPlaceOutcome::NotOwner;
	const double X = WorldCm.X / 100.0, Y = WorldCm.Y / 100.0;
	int Id = 0;
	const tta::PlaceResult Result = TTABuildItem::IsCamera(Item)
		? Layout.placeCamera(Owner, TTABuildItem::Camera(Item), X, Y, YawDeg, &Id)
		: Layout.placeTrap(Owner, TTABuildItem::Trap(Item), X, Y, YawDeg, &Id);
	if (Result != tta::PlaceResult::Ok) return ToUE(Result);
	SyncMarker(Id);
	if (OutId) *OutId = Id;
	return ETTAPlaceOutcome::Ok;
}

ETTAPlaceOutcome ATTAGameMode::CheckOwned(const AController* By, int32 Id, const tta::Placement*& OutPlacement) const
{
	if (GetGameState<ATTAGameState>()->GetPhase() != ETTAPhase::Build) return ETTAPlaceOutcome::WrongPhase;
	for (const tta::Placement& P : Layout.all())
	{
		if (P.id != Id) continue;
		if (P.owner != ATTAPlacementMarker::PlayerIdOf(By)) return ETTAPlaceOutcome::NotOwner;
		OutPlacement = &P;
		return ETTAPlaceOutcome::Ok;
	}
	return ETTAPlaceOutcome::NotOwner;
}

ETTAPlaceOutcome ATTAGameMode::SetTrapSetting(const AController* By, int32 Id, int32 Index, int32 Value)
{
	const tta::Placement* P = nullptr;
	const ETTAPlaceOutcome Owned = CheckOwned(By, Id, P);
	if (Owned != ETTAPlaceOutcome::Ok) return Owned;
	const tta::PlaceResult Result = Layout.setTrapSetting(Id, Index, Value);
	if (Result == tta::PlaceResult::Ok) SyncMarker(Id);
	return ToUE(Result);
}

ETTAPlaceOutcome ATTAGameMode::RemovePlacement(const AController* By, int32 Id)
{
	const tta::Placement* P = nullptr;
	const ETTAPlaceOutcome Owned = CheckOwned(By, Id, P);
	if (Owned != ETTAPlaceOutcome::Ok) return Owned;
	if (!P->hidden) return ETTAPlaceOutcome::WrongPhase; // revealed in an earlier round: part of the map now
	Layout.remove(Id);
	TObjectPtr<ATTAPlacementMarker> Marker;
	if (Markers.RemoveAndCopyValue(Id, Marker) && Marker) Marker->Destroy();
	return ETTAPlaceOutcome::Ok;
}
