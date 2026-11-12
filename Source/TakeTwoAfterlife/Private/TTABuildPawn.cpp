#include "TTABuildPawn.h"

#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TTAGameMode.h"

ATTABuildPawn::ATTABuildPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(ADefaultPawn::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bOnlyRelevantToOwner = true;
	SetReplicatingMovement(false); // the owner flies locally; the server never needs the camera
	GetCollisionComponent()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Ghost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ghost"));
	Ghost->SetupAttachment(RootComponent);
	Ghost->SetUsingAbsoluteLocation(true);
	Ghost->SetUsingAbsoluteRotation(true);
	Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ghost->SetVisibility(false);
}

void ATTABuildPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	Input->BindAction("BuildPlace", IE_Pressed, this, &ATTABuildPawn::Place);
	Input->BindAction("BuildRemove", IE_Pressed, this, &ATTABuildPawn::RemoveUnderCursor);
	Input->BindAction("BuildSelect", IE_Pressed, this, &ATTABuildPawn::SelectUnderCursor);
	Input->BindAction("BuildRotate", IE_Pressed, this, &ATTABuildPawn::Rotate);
	Input->BindAction("BuildNextItem", IE_Pressed, this, &ATTABuildPawn::NextItem);
	Input->BindAction("BuildPrevItem", IE_Pressed, this, &ATTABuildPawn::PrevItem);
	Input->BindAction("BuildSetting1Down", IE_Pressed, this, &ATTABuildPawn::Setting1Down);
	Input->BindAction("BuildSetting1Up", IE_Pressed, this, &ATTABuildPawn::Setting1Up);
	Input->BindAction("BuildSetting2Down", IE_Pressed, this, &ATTABuildPawn::Setting2Down);
	Input->BindAction("BuildSetting2Up", IE_Pressed, this, &ATTABuildPawn::Setting2Up);
}

tta::Layout ATTABuildPawn::OwnLayout() const
{
	tta::Layout Layout;
	const int32 Me = ATTAPlacementMarker::PlayerIdOf(this);
	for (TActorIterator<ATTAPlacementMarker> It(GetWorld()); It; ++It)
	{
		const tta::Placement P = It->GetData().ToCore();
		if (P.owner != Me) continue;
		const double X = tta::cellToMeters(P.cellX), Y = tta::cellToMeters(P.cellY), Deg = 45.0 * P.rotation;
		const tta::PlaceResult Result = P.isCamera ? Layout.placeCamera(Me, P.camera, X, Y, Deg) : Layout.placeTrap(Me, P.trap, X, Y, Deg);
		ensureMsgf(Result == tta::PlaceResult::Ok, TEXT("own placement %d does not rebuild"), P.id);
	}
	return Layout;
}

ATTAPlacementMarker* ATTABuildPawn::FindMarker(int32 Id) const
{
	for (TActorIterator<ATTAPlacementMarker> It(GetWorld()); It; ++It)
	{
		if (It->GetData().Id == Id) return *It;
	}
	return nullptr;
}

bool ATTABuildPawn::TraceCursor(FHitResult& Hit) const
{
	const AController* PC = GetController();
	if (!PC) return false;
	FVector Eye;
	FRotator View;
	PC->GetPlayerViewPoint(Eye, View);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TTABuildTrace), false, this);
	return GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + View.Vector() * TraceDistanceCm, ECC_Visibility, Params);
}

ATTAPlacementMarker* ATTABuildPawn::OwnMarkerUnderCursor() const
{
	FHitResult Hit;
	ATTAPlacementMarker* Marker = TraceCursor(Hit) ? Cast<ATTAPlacementMarker>(Hit.GetActor()) : nullptr;
	return Marker && Marker->GetData().Owner == ATTAPlacementMarker::PlayerIdOf(this) ? Marker : nullptr;
}

void ATTABuildPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsLocallyControlled()) return;
	FHitResult Hit;
	bGhostOnFloor = TraceCursor(Hit);
	Ghost->SetVisibility(bGhostOnFloor);
	if (!bGhostOnFloor) return;

	GhostCm = Hit.ImpactPoint;
	const tta::Placement Preview = [&]
	{
		tta::Placement P;
		P.isCamera = TTABuildItem::IsCamera(SelectedItem);
		if (P.isCamera) P.camera = TTABuildItem::Camera(SelectedItem);
		else P.trap = TTABuildItem::Trap(SelectedItem);
		P.cellX = tta::snapToCell(GhostCm.X / 100.0);
		P.cellY = tta::snapToCell(GhostCm.Y / 100.0);
		P.rotation = RotationStep;
		return P;
	}();
	Ghost->SetWorldTransform(ATTAPlacementMarker::WorldTransform(FTTAPlacementRep::FromCore(Preview)));
	Ghost->SetStaticMesh(GetDefault<ATTAPlacementMarker>()->MeshForItem(SelectedItem));
	GhostOutcome = ToUE(OwnLayout().canPlace(ATTAPlacementMarker::PlayerIdOf(this), Preview.isCamera, Preview.trap,
		GhostCm.X / 100.0, GhostCm.Y / 100.0));
}

void ATTABuildPawn::Place()
{
	if (bGhostOnFloor) ServerPlace(SelectedItem, GhostCm, 45.f * RotationStep);
}

void ATTABuildPawn::RemoveUnderCursor()
{
	if (const ATTAPlacementMarker* Marker = OwnMarkerUnderCursor()) ServerRemove(Marker->GetData().Id);
}

void ATTABuildPawn::SelectUnderCursor()
{
	if (const ATTAPlacementMarker* Marker = OwnMarkerUnderCursor()) SelectedPlacement = Marker->GetData().Id;
}

void ATTABuildPawn::Rotate()
{
	RotationStep = (RotationStep + 1) % tta::kRotationSteps;
}

void ATTABuildPawn::NextItem()
{
	SelectedItem = (SelectedItem + 1) % TTABuildItem::Count();
}

void ATTABuildPawn::PrevItem()
{
	SelectedItem = (SelectedItem + TTABuildItem::Count() - 1) % TTABuildItem::Count();
}

void ATTABuildPawn::AdjustSetting(int32 Index, int32 Delta)
{
	const ATTAPlacementMarker* Marker = FindMarker(SelectedPlacement);
	if (!Marker) return;
	const FTTAPlacementRep& Data = Marker->GetData();
	ServerSetSetting(Data.Id, Index, (Index == 0 ? Data.Setting0 : Data.Setting1) + Delta);
}

void ATTABuildPawn::ServerPlace_Implementation(int32 Item, FVector_NetQuantize WorldCm, float YawDeg)
{
	int32 Id = 0;
	const ETTAPlaceOutcome Outcome = GetWorld()->GetAuthGameMode<ATTAGameMode>()->PlaceItem(GetController(), Item, WorldCm, YawDeg, &Id);
	ClientResult(Outcome, Id);
}

void ATTABuildPawn::ServerRemove_Implementation(int32 Id)
{
	ClientResult(GetWorld()->GetAuthGameMode<ATTAGameMode>()->RemovePlacement(GetController(), Id), 0);
}

void ATTABuildPawn::ServerSetSetting_Implementation(int32 Id, int32 Index, int32 Value)
{
	ClientResult(GetWorld()->GetAuthGameMode<ATTAGameMode>()->SetTrapSetting(GetController(), Id, Index, Value), 0);
}

void ATTABuildPawn::ClientResult_Implementation(ETTAPlaceOutcome Outcome, int32 Id)
{
	LastServerOutcome = Outcome;
	if (Outcome == ETTAPlaceOutcome::Ok && Id != 0) SelectedPlacement = Id;
}
