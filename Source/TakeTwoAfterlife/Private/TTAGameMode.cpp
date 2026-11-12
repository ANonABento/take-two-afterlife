#include "TTAGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TTABuildHUD.h"
#include "TTABuildPawn.h"

ATTAGameMode::ATTAGameMode()
{
	GameStateClass = ATTAGameState::StaticClass();
	HUDClass = ATTABuildHUD::StaticClass();
	BuildPawnClass = ATTABuildPawn::StaticClass();
}

bool ATTAGameMode::RequestPhase(ETTAPhase To)
{
	if (!Match.transition(ToCore(To)))
	{
		return false;
	}
	GetGameState<ATTAGameState>()->SetPhase(ToUE(Match.phase()), Match.round());
	if (To == ETTAPhase::Build) EnterBuild();
	if (To == ETTAPhase::Run) RevealPlacements();
	return true;
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
