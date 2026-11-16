#include "TTAHeavenWatch.h"

#include "Camera/CameraActor.h"
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

AActor* ATTAHeavenWatch::GetSeatCamera()
{
	return SeatCamera.Get();
}

ATTAHeavenWatch::ATTAHeavenWatch()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Cloud = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cloud"));
	RootComponent = Cloud;
	Cloud->SetStaticMesh(CylinderMesh.Object);
	Cloud->SetRelativeScale3D(FVector(30.f, 30.f, 0.4f));
	Cloud->SetCollisionProfileName(TEXT("BlockAll"));

	Couch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Couch"));
	Couch->SetupAttachment(Cloud);
	Couch->SetStaticMesh(CubeMesh.Object);
	Couch->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Couch->SetRelativeScale3D(FVector(1.2f, 4.f, 0.8f));
	Couch->SetCollisionProfileName(TEXT("NoCollision"));

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Cloud);
	Screen->SetStaticMesh(CubeMesh.Object);
	Screen->SetRelativeLocation(FVector(700.f, 0.f, 280.f));
	Screen->SetRelativeScale3D(FVector(0.1f, 9.6f, 5.4f));
	Screen->SetCollisionProfileName(TEXT("NoCollision"));
}

void ATTAHeavenWatch::Start(const TArray<FTransform>& InViews)
{
	Views = InViews;
	if (Views.IsEmpty())
	{
		Views.Add(FTransform(FRotator(-18.f, 0.f, 0.f), FVector(1500.f, -600.f, 700.f)));
	}

	if (!SeatCamera)
	{
		const FVector Seat = GetActorLocation() + FVector(-80.f, 0.f, 160.f);
		const FVector ScreenAt = GetActorLocation() + FVector(700.f, 0.f, 280.f);
		SeatCamera = GetWorld()->SpawnActor<ACameraActor>(Seat, (ScreenAt - Seat).Rotation());
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
		}
	}
	if (!ScreenWidget)
	{
		ScreenWidget = NewObject<UWidgetComponent>(this, TEXT("Feed"));
		ScreenWidget->SetupAttachment(Screen);
		ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
		ScreenWidget->SetDrawSize(FVector2D(960.f, 540.f));
		ScreenWidget->SetTwoSided(true);
		ScreenWidget->SetBlendMode(EWidgetBlendMode::Transparent);
		ScreenWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ScreenWidget->SetRelativeLocation(FVector(-12.f, 0.f, 0.f));
		ScreenWidget->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
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
	if (!FeedCam || Views.IsEmpty()) return;
	SwitchIn += DeltaSeconds;
	if (SwitchIn < 2.5f) return;
	SwitchIn = 0.f;
	ViewIndex = (ViewIndex + 1) % Views.Num();
	ShowView(ViewIndex);
}
