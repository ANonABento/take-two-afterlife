#include "TTAHeavenWatch.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "TTACinema.h"
#include "TTAFinishGate.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TTABotController.h"
#include "TTARunnerPawn.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// ATTACinema space, cm: couch at the origin facing +X, screen centre at x = 1200, the lobby gate
// at the end of the carpet on +Y (Scripts/heaven_layout.json). The stand-in uses the same spots.
const FVector BodySpot(0.f, -45.f, 97.f); // standing on the second couch seat
const FVector CameraSpot(-320.f, 0.f, 230.f); // behind the couch, over the angel's head
const FVector ScreenCentre(1200.f, 0.f, 350.f);
const FVector WalkSpot(150.f, 120.f, 60.f); // between the couch and the carpet
const FVector GateSpot(150.f, 870.f, 0.f);
// SM_Heaven_Screen's dark face is 8 cm in front of its pivot; the stand-in cube's is 5 cm.
constexpr float ScreenFaceGapCm = 11.f;
constexpr float SeatCameraFov = 60.f;
}

void UTTACamFeedWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Image = WidgetTree->ConstructWidget<UImage>();
		WidgetTree->RootWidget = Image;
	}
}

void UTTACamFeedWidget::SetFeed(UTexture* Texture)
{
	if (!Image && WidgetTree && !WidgetTree->RootWidget)
	{
		Image = WidgetTree->ConstructWidget<UImage>();
		WidgetTree->RootWidget = Image;
	}
	if (!Image || !Texture) return;
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = FVector2D(640.f, 360.f);
	Image->SetBrush(Brush);
}

FVector ATTAHeavenWatch::Spot(const FVector& Local) const
{
	return FTransform(GetActorRotation(), GetActorLocation()).TransformPosition(Local);
}

AActor* ATTAHeavenWatch::GetSeatCamera()
{
	return SeatCamera.Get();
}

FTransform ATTAHeavenWatch::GetSeatTransform() const
{
	const FVector Seat = Spot(BodySpot);
	return FTransform((Spot(ScreenCentre) - Seat).GetSafeNormal2D().Rotation(), Seat);
}

void ATTAHeavenWatch::ShowTexture(UTexture* Texture)
{
	bLive = false;
	if (Feed) Feed->SetFeed(Texture);
}

FTransform ATTAHeavenWatch::GetWalkTransform() const
{
	const FVector At = Spot(WalkSpot);
	return FTransform((Spot(GateSpot) - At).GetSafeNormal2D().Rotation(), At);
}

void ATTAHeavenWatch::ClearContinueGate()
{
	if (ContinueGate)
	{
		ContinueGate->Destroy();
		ContinueGate = nullptr;
	}
}

void ATTAHeavenWatch::SpawnContinueGate()
{
	ClearContinueGate();
	if (Stage) return; // Heaven.umap has the pearly gate
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// The trigger is thin along the gate's X, so the gate is turned to face the walk up from the couch.
	const FRotator Facing(0.f, GetActorRotation().Yaw + 90.f, 0.f);
	ContinueGate = GetWorld()->SpawnActor<ATTAFinishGate>(Spot(GateSpot), Facing, Params);
	if (ContinueGate && Cube)
	{
		ContinueGate->Arch->SetStaticMesh(Cube);
		ContinueGate->Arch->SetWorldScale3D(FVector(0.3f, 2.2f, 2.6f));
		ContinueGate->Arch->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
		ContinueGate->Arch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

ATTAHeavenWatch::ATTAHeavenWatch()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Unscaled root. The parts used to hang off the scaled cloud, which stretched the couch and
	// pushed the screen 200 m away.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Cloud = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cloud"));
	Cloud->SetupAttachment(RootComponent);
	Cloud->SetStaticMesh(CylinderMesh.Object);
	Cloud->SetRelativeLocation(FVector(0.f, 0.f, -20.f)); // top at z = 0
	Cloud->SetRelativeScale3D(FVector(30.f, 30.f, 0.4f));
	Cloud->SetCollisionProfileName(TEXT("BlockAll"));

	Couch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Couch"));
	Couch->SetupAttachment(RootComponent);
	Couch->SetStaticMesh(CubeMesh.Object);
	Couch->SetRelativeLocation(FVector(0.f, 0.f, 22.5f));
	Couch->SetRelativeScale3D(FVector(0.9f, 3.6f, 0.45f));
	Couch->SetCollisionProfileName(TEXT("NoCollision"));

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(RootComponent);
	Screen->SetStaticMesh(CubeMesh.Object);
	Screen->SetRelativeLocation(ScreenCentre);
	Screen->SetRelativeScale3D(FVector(0.1f, 9.6f, 5.4f));
	Screen->SetCollisionProfileName(TEXT("NoCollision"));

	for (UStaticMeshComponent* Part : {Cloud.Get(), Couch.Get(), Screen.Get()})
	{
		Part->SetMobility(EComponentMobility::Movable);
	}
}

void ATTAHeavenWatch::UseStage(ATTACinema* InStage)
{
	Stage = InStage;
	if (!Stage) return;
	SetActorLocationAndRotation(Stage->GetActorLocation(), Stage->GetActorRotation());
	for (UStaticMeshComponent* Part : {Cloud.Get(), Couch.Get(), Screen.Get()})
	{
		Part->SetVisibility(false);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	ClearContinueGate();
	PlaceRig();
}

void ATTAHeavenWatch::PlaceRig()
{
	if (SeatCamera)
	{
		const FVector At = Spot(CameraSpot);
		SeatCamera->SetActorLocationAndRotation(At, (Spot(ScreenCentre) - At).Rotation());
	}
	if (ScreenWidget)
	{
		// The widget draws on its +X side, so it is turned to face the couch.
		ScreenWidget->SetWorldLocationAndRotation(Spot(ScreenCentre - FVector(ScreenFaceGapCm, 0.f, 0.f)),
			FRotator(0.f, GetActorRotation().Yaw + 180.f, 0.f));
	}
}

void ATTAHeavenWatch::Start(const TArray<FTransform>& InViews)
{
	bLive = true;
	Views = InViews;
	if (Views.IsEmpty())
	{
		Views.Add(FTransform(FRotator(-18.f, 0.f, 0.f), FVector(1500.f, -600.f, 700.f)));
	}

	if (!SeatCamera)
	{
		SeatCamera = GetWorld()->SpawnActor<ACameraActor>(GetActorLocation(), GetActorRotation());
		if (SeatCamera) SeatCamera->GetCameraComponent()->SetFieldOfView(SeatCameraFov);
	}

	if (!Target)
	{
		Target = NewObject<UTextureRenderTarget2D>(this);
		Target->InitAutoFormat(640, 360);
		Target->UpdateResourceImmediate(true);
	}
	if (!FeedCam)
	{
		FeedCam = GetWorld()->SpawnActor<ASceneCapture2D>(Views[0].GetLocation(), Views[0].Rotator());
		if (USceneCaptureComponent2D* Cap = FeedCam ? FeedCam->GetCaptureComponent2D() : nullptr)
		{
			Cap->TextureTarget = Target;
			Cap->CaptureSource = SCS_FinalColorLDR;
			Cap->bCaptureEveryFrame = true;
			Cap->FOVAngle = 75.f;
			// The outline pass inks every surface black from some camera spots (only the sky is
			// spared), so the TV feed goes without it.
			Cap->ShowFlags.SetPostProcessMaterial(false);
		}
	}
	if (!ScreenWidget)
	{
		ScreenWidget = NewObject<UWidgetComponent>(this, TEXT("Feed"));
		ScreenWidget->SetupAttachment(RootComponent);
		ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
		// 1 px = 1 cm: the screen face is 9.6 x 5.4 m.
		ScreenWidget->SetDrawSize(FVector2D(960.f, 540.f));
		// Opaque: a scene capture's alpha is 0, which a transparent widget draws as nothing.
		ScreenWidget->SetBlendMode(EWidgetBlendMode::Opaque);
		ScreenWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ScreenWidget->RegisterComponent();
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			Feed = CreateWidget<UTTACamFeedWidget>(PC, UTTACamFeedWidget::StaticClass());
			if (Feed)
			{
				Feed->SetFeed(Target);
				ScreenWidget->SetWidget(Feed);
			}
		}
	}
	if (Feed && Target) Feed->SetFeed(Target);
	PlaceRig();
	ShowView(0);
}

void ATTAHeavenWatch::ShowView(int32 Index)
{
	if (!FeedCam || Views.IsEmpty()) return;
	FTransform View = Views[Index % Views.Num()];
	// Every other cut follows whoever is still running, so the TV stays on the race.
	if (Index % 2 == 1)
	{
		const ATTARunnerPawn* Leader = nullptr;
		for (TActorIterator<ATTABotController> It(GetWorld()); It; ++It)
		{
			if (!It->IsRunning()) continue;
			const ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(It->GetPawn());
			if (!Runner) continue;
			if (!Leader || Runner->GetActorLocation().X > Leader->GetActorLocation().X)
			{
				Leader = Runner;
			}
		}
		if (Leader)
		{
			const FVector At = Leader->GetActorLocation();
			const FVector Cam = At + FVector(-450.f, 220.f, 280.f);
			View = FTransform((At - Cam).Rotation(), Cam);
		}
	}
	FeedCam->SetActorLocationAndRotation(View.GetLocation(), View.Rotator());
}

void ATTAHeavenWatch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLive || !FeedCam || Views.IsEmpty()) return;
	SwitchIn += DeltaSeconds;
	if (SwitchIn < 2.5f) return;
	SwitchIn = 0.f;
	ViewIndex = (ViewIndex + 1) % Views.Num();
	ShowView(ViewIndex);
}
