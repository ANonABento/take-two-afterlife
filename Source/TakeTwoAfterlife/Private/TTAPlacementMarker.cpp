#include "TTAPlacementMarker.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

static_assert(static_cast<int>(ETTAPlaceOutcome::BadSetting) == static_cast<int>(tta::PlaceResult::BadSetting),
	"ETTAPlaceOutcome must mirror tta::PlaceResult");

namespace
{
constexpr double kCmPerMeter = 100.0;
constexpr int32 kCameraTypeCount = static_cast<int32>(tta::CameraType::TrapCam) + 1;
int32 TrapCount() { return static_cast<int32>(tta::trapCatalogue().size()); }
}

int32 TTABuildItem::Count() { return TrapCount() + kCameraTypeCount; }
bool TTABuildItem::IsCamera(int32 Item) { return Item >= TrapCount(); }
tta::TrapType TTABuildItem::Trap(int32 Item) { return tta::trapCatalogue().at(static_cast<size_t>(Item)).type; }
tta::CameraType TTABuildItem::Camera(int32 Item) { return static_cast<tta::CameraType>(Item - TrapCount()); }

FString TTABuildItem::Label(int32 Item)
{
	if (IsCamera(Item))
	{
		return FString::Printf(TEXT("Camera: %s (free)"), UTF8_TO_TCHAR(tta::cameraName(Camera(Item))));
	}
	const tta::TrapSpec& Spec = tta::trapSpec(Trap(Item));
	return FString::Printf(TEXT("Trap: %s (%d)"), UTF8_TO_TCHAR(Spec.name), Spec.cost);
}

FTTAPlacementRep FTTAPlacementRep::FromCore(const tta::Placement& P)
{
	FTTAPlacementRep Rep;
	Rep.Id = P.id;
	Rep.Owner = P.owner;
	Rep.Item = P.isCamera ? TrapCount() + static_cast<int32>(P.camera) : static_cast<int32>(P.trap);
	Rep.CellX = P.cellX;
	Rep.CellY = P.cellY;
	Rep.Rotation = P.rotation;
	Rep.Setting0 = P.settings[0];
	Rep.Setting1 = P.settings[1];
	Rep.bRevealed = !P.hidden;
	return Rep;
}

tta::Placement FTTAPlacementRep::ToCore() const
{
	tta::Placement P;
	P.id = Id;
	P.owner = Owner;
	P.isCamera = TTABuildItem::IsCamera(Item);
	if (P.isCamera) P.camera = TTABuildItem::Camera(Item);
	else P.trap = TTABuildItem::Trap(Item);
	P.cellX = CellX;
	P.cellY = CellY;
	P.rotation = Rotation;
	P.settings = {Setting0, Setting1};
	P.hidden = !bRevealed;
	return P;
}

ATTAPlacementMarker::ATTAPlacementMarker()
{
	bReplicates = true;
	SetReplicatingMovement(false); // the transform comes from Data
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Mesh;
}

void ATTAPlacementMarker::SetData(const FTTAPlacementRep& InData)
{
	check(HasAuthority());
	Data = InData;
	Refresh();
}

void ATTAPlacementMarker::OnRep_Data()
{
	Refresh();
}

FTransform ATTAPlacementMarker::WorldTransform(const FTTAPlacementRep& Rep)
{
	const double Z = TTABuildItem::IsCamera(Rep.Item) ? GetDefault<ATTAPlacementMarker>()->CameraHeightCm : 0.0;
	return FTransform(FRotator(0.f, 45.f * Rep.Rotation, 0.f),
		FVector(tta::cellToMeters(Rep.CellX) * kCmPerMeter, tta::cellToMeters(Rep.CellY) * kCmPerMeter, Z));
}

int32 ATTAPlacementMarker::PlayerIdOf(const AActor* ControllerOrPawn)
{
	const APlayerState* State = nullptr;
	if (const AController* Controller = Cast<AController>(ControllerOrPawn)) State = Controller->PlayerState;
	else if (const APawn* Pawn = Cast<APawn>(ControllerOrPawn)) State = Pawn->GetPlayerState();
	return State ? State->GetPlayerId() : INDEX_NONE;
}

UStaticMesh* ATTAPlacementMarker::MeshForItem(int32 Item) const
{
	if (!ItemMeshes.IsValidIndex(Item))
	{
		UE_LOG(LogTemp, Error, TEXT("ATTAPlacementMarker: no ItemMeshes entry for build item %d (DefaultGame.ini)"), Item);
		return nullptr;
	}
	return ItemMeshes[Item].LoadSynchronous();
}

void ATTAPlacementMarker::Refresh()
{
	SetActorTransform(WorldTransform(Data));
	Mesh->SetStaticMesh(MeshForItem(Data.Item));
	bool bShow = Data.bRevealed;
	if (!bShow)
	{
		// Listen-server host holds every marker; draw only the local player's own.
		const APlayerController* Local = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		bShow = Local && Local->IsLocalController() && PlayerIdOf(Local) == Data.Owner;
	}
	Mesh->SetVisibility(bShow);
	Mesh->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool ATTAPlacementMarker::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return Data.bRevealed || PlayerIdOf(RealViewer) == Data.Owner;
}

void ATTAPlacementMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATTAPlacementMarker, Data);
}
