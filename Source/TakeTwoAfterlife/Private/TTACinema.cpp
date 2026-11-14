#include "TTACinema.h"
#include "Engine/StaticMesh.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "TTACutPlayer.h"
#include "TTAGameMode.h"
#include "TTAMicPlayer.h"
#include "TTAWorldLook.h"
#include "UObject/ConstructorHelpers.h"

#include <stdexcept>
#include <vector>

namespace
{
// Layout in cm, actor space: couch at the origin facing +X, screen ahead of it.
constexpr float SeatSpacing = 90.f;
constexpr float SeatHeight = 45.f;
const FVector ScreenLocation(1200.f, 0.f, 350.f);
}

ATTACinema::ATTACinema()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Cloud = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cloud"));
	RootComponent = Cloud;
	Cloud->SetStaticMesh(CylinderMesh.Object);
	Cloud->SetRelativeScale3D(FVector(30.f, 30.f, 0.4f)); // ~15 m radius, walkable top
	Cloud->SetCollisionProfileName(TEXT("BlockAll"));

	Couch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Couch"));
	Couch->SetupAttachment(Cloud);
	Couch->SetStaticMesh(CubeMesh.Object);
	Couch->SetAbsolute(false, false, true);
	Couch->SetRelativeScale3D(FVector(0.9f, NumSeats * SeatSpacing / 100.f, 0.9f));
	Couch->SetCollisionProfileName(TEXT("BlockAll"));

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Cloud);
	Screen->SetStaticMesh(CubeMesh.Object);
	Screen->SetAbsolute(false, false, true);
	Screen->SetRelativeScale3D(FVector(0.1f, 9.6f, 5.4f)); // 16:9
	Screen->SetCollisionProfileName(TEXT("NoCollision"));

	Mic = CreateDefaultSubobject<UTTAMicPlayer>(TEXT("Mic"));
}

void ATTACinema::BeginPlay()
{
	Super::BeginPlay();
	// Cloud scale is absolute-scaled for children, so place them in actor space now.
	Couch->SetWorldLocation(GetActorTransform().TransformPosition(FVector(0.f, 0.f, SeatHeight)));
	Screen->SetWorldLocation(GetActorTransform().TransformPosition(ScreenLocation));
	if (UMaterialParameterCollection* Collection = WorldCollection.LoadSynchronous())
	{
		UTTAWorldLookLibrary::ApplyWorldLook(this, Collection, ETTAWorld::Heaven);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATTACinema: WorldCollection '%s' did not load; heaven look not applied"), *WorldCollection.ToString());
	}
}

void ATTACinema::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

FTransform ATTACinema::GetSeatTransform(int32 Seat) const
{
	check(Seat >= 0 && Seat < NumSeats);
	const float Y = (Seat - (NumSeats - 1) / 2.f) * SeatSpacing;
	const FVector Local(0.f, Y, SeatHeight);
	const FRotator Facing = (ScreenLocation - Local).GetSafeNormal2D().Rotation();
	return FTransform(Facing, Local) * GetActorTransform();
}

void ATTACinema::StartPremiere(const TArray<FTTACinemaCut>& Cuts, uint32 Seed, bool bFinalRound, const TArray<int32>& Podium)
{
	std::vector<tta::PremiereCut> Core;
	CutsById.clear();
	for (const FTTACinemaCut& C : Cuts)
	{
		Core.push_back({C.CutId, C.Cut, C.Victim, C.Edit.durationMs()});
		CutsById.emplace(C.CutId, C);
	}
	// The match's timings on a TTA server; ini timings when no TTA game mode runs (tests, preview).
	const ATTAGameMode* Mode = GetWorld()->GetAuthGameMode<ATTAGameMode>();
	const tta::MatchSettings Timings = Mode ? Mode->GetSettings() : ATTAGameMode::IniSettings();
	Premiere = std::make_unique<tta::Premiere>(std::move(Core), Seed, bFinalRound, Timings);
	FinalPodium = Podium;
	ShownPodium.Reset();
	StepRemainder = 0.f;
	if (!Player) Player = NewObject<UTTACutPlayer>(this);
	EnterStep();
}

bool ATTACinema::Rate(int32 Rater, int32 Funny, int32 Evil)
{
	if (!Premiere) return false;
	try
	{
		Premiere->rate(Rater, Funny, Evil);
		return true;
	}
	catch (const std::exception& E)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATTACinema: rating from %d rejected: %hs"), Rater, E.what());
		return false;
	}
}

void ATTACinema::Step(float DeltaSeconds)
{
	if (!Premiere) return;
	const tta::PremiereStep Before = Premiere->step();
	const size_t BeforeIndex = Premiere->index();
	StepRemainder += DeltaSeconds;
	const int32 WholeMs = FMath::FloorToInt(StepRemainder * 1000.f);
	StepRemainder -= WholeMs * 0.001f;
	Premiere->advance(static_cast<uint32>(FMath::Max(0, WholeMs)));
	if (Premiere->step() != Before || Premiere->index() != BeforeIndex) EnterStep();

	if (Premiere->step() == tta::PremiereStep::Playback)
	{
		Player->Seek(Premiere->stepElapsedMs());
		const TOptional<tta::CutFrame> Frame = Player->GetFrame();
		// Re-sync the runner's mic at every cut to a new clip.
		if (Frame && Frame->clip != LastClip)
		{
			LastClip = Frame->clip;
			if (bMicLoaded) Mic->PlayFrom(static_cast<float>(Frame->replaySeconds));
		}
	}
}

void ATTACinema::EnterStep()
{
	if (bMicLoaded) Mic->Stop();
	bMicLoaded = false;
	LastClip = -1;
	switch (Premiere->step())
	{
	case tta::PremiereStep::Playback:
	{
		const FTTACinemaCut& C = CutsById.at(Premiere->current().cutId);
		Player->SetCut(C.Edit);
		Player->Play();
		bMicLoaded = !C.MicPath.IsEmpty() && Mic->Load(C.MicPath);
		break;
	}
	case tta::PremiereStep::Podium:
		ShownPodium = FinalPodium;
		break;
	default:
		Player->Stop();
		break;
	}
	OnStep.Broadcast(Premiere->step());
}

TOptional<tta::CutFrame> ATTACinema::GetScreenFrame() const
{
	if (!Premiere || Premiere->step() != tta::PremiereStep::Playback) return {};
	return Player->GetFrame();
}

FText ATTACinema::GetIntroLine() const
{
	if (!Premiere || Premiere->step() == tta::PremiereStep::Podium || Premiere->step() == tta::PremiereStep::Done) return FText::GetEmpty();
	const tta::PremiereCut& C = Premiere->current();
	return FText::Format(NSLOCTEXT("TTA", "CinemaIntro", "Directed by Player {0}. Starring Player {1}!"), C.cut.editor, C.victim);
}
