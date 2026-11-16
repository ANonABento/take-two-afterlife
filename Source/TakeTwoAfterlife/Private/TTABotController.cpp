#include "TTABotController.h"
#include "TTAPlacementMarker.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TTACinema.h"
#include "TTAFinishGate.h"
#include "TTARunnerPawn.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTABots, Log, All);

const FName ATTABotController::RouteTag(TEXT("TTARoute"));

ATTABotController::ATTABotController()
{
	bWantsPlayerState = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ATTABotController::InitBrain(int32 PlayerId, uint64 Seed)
{
	checkf(PlayerState, TEXT("ATTABotController: no PlayerState (spawn it in a world with a game mode)"));
	PlayerState->SetPlayerId(PlayerId);
	Brain = std::make_unique<tta::BotBrain>(PlayerId, Seed);
}

void ATTABotController::BuildTraps(tta::Layout& Layout)
{
	check(Brain);
	if (bBuiltThisRound) return;
	Brain->build(Layout);
	bBuiltThisRound = true;
	// Placement is instant in Core; the walk to each spot is only for show during Build.
	BuildStops.Reset();
	for (const tta::Placement& P : Layout.all())
	{
		if (P.owner == Brain->playerId() && P.round == Layout.round())
			BuildStops.Add(ATTAPlacementMarker::WorldTransform(FTTAPlacementRep::FromCore(P)).GetLocation());
	}
	BuildStop = 0;
	BuildPause = 0.f;
}

void ATTABotController::StartRun(const TArray<FVector>& RouteCm)
{
	check(Brain);
	checkf(RouteCm.Num() > 0, TEXT("ATTABotController: empty route (no TTARoute markers or finish gate)"));
	std::vector<tta::RoutePoint> Points;
	for (const FVector& P : RouteCm) Points.push_back({P.X / 100.0, P.Y / 100.0, P.Z / 100.0});
	Route = std::make_unique<tta::BotRoute>(std::move(Points));
	BuildStops.Reset();
	Plan = Brain->planRun();
	bBuiltThisRound = false;
	bWasGroundAhead = true;
	bSkipThisGap = false;
	LastLife = tta::LifeState::Alive;
	LogClock = 0.f;
	UE_LOG(LogTTABots, Log, TEXT("%s: run start, %d route points, pawn %s, PlayerState %s"), *GetName(), RouteCm.Num(), *GetNameSafe(GetPawn()), *GetNameSafe(PlayerState));
}

void ATTABotController::StopRun()
{
	Route.reset();
}

void ATTABotController::DriveRunner(float DeltaSeconds)
{
	ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(GetPawn());
	if (!Route || !Runner) return;
	const tta::LifeState Life = Runner->GetLifeState();
	if (Life != tta::LifeState::Alive)
	{
		LastLife = Life;
		return;
	}
	if (LastLife != tta::LifeState::Alive)
	{
		Route->restart(); // PawnLife put the runner back at the start
		LastLife = tta::LifeState::Alive;
	}
	const FVector Pos = Runner->GetActorLocation();
	FVector Dir = Runner->GetVelocity().GetSafeNormal2D();
	if (Dir.IsNearlyZero()) Dir = Runner->GetActorForwardVector().GetSafeNormal2D();
	const FVector From = Pos + Dir * ProbeAheadCm;
	FHitResult Floor;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TTABotFloor), false, Runner);
	const bool bGroundAhead = GetWorld()->LineTraceSingleByChannel(Floor, From, From - FVector(0.f, 0.f, 500.f), ECC_Visibility, Params);
	if (!bGroundAhead && bWasGroundAhead) bSkipThisGap = Brain->rollFail(Plan); // one roll per gap
	bWasGroundAhead = bGroundAhead;

	const tta::RouteStep Step = Route->step({{Pos.X / 100.0, Pos.Y / 100.0, Pos.Z / 100.0}, bGroundAhead, bSkipThisGap, DeltaSeconds});
	if (Step.respawn)
	{
		Runner->Hit(); // freeze -> ragdoll -> respawn at the start
		return;
	}
	if ((LogClock += DeltaSeconds) >= 1.f)
	{
		LogClock = 0.f; // once a second: enough to see a bot that never moves on the user's PC
		UE_LOG(LogTTABots, Verbose, TEXT("%s: pos %s target %d/%d dir (%.2f, %.2f) ground %d done %d"), *GetName(), *Pos.ToString(), static_cast<int32>(Route->target()), static_cast<int32>(Route->size()), Step.dirX, Step.dirY, bGroundAhead, Step.done);
	}
	if (Step.done) return;
	Runner->AddMovementInput(FVector(Step.dirX, Step.dirY, 0.0));
	if (Step.jump) Runner->Jump();
}

void ATTABotController::FillCut(const tta::Replay& Replay)
{
	Cut = tta::autoEdit(Replay, {HeavenCam}, HeavenCam);
}

bool ATTABotController::RateCurrent(ATTACinema* Cinema)
{
	check(Brain);
	const tta::Premiere* Premiere = Cinema ? Cinema->GetPremiere() : nullptr;
	if (!Premiere || Premiere->step() != tta::PremiereStep::Rating) return false;
	const std::optional<std::pair<int, int>> Vote = Brain->vote(Premiere->current().cut);
	return Vote && Cinema->Rate(Brain->playerId(), Vote->first, Vote->second);
}

TArray<FVector> ATTABotController::FindRoute(UWorld* World)
{
	TArray<FVector> Points;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(RouteTag)) Points.Add(It->GetActorLocation());
	}
	Points.Sort([](const FVector& A, const FVector& B) { return A.X < B.X; });
	for (TActorIterator<ATTAFinishGate> It(World); It; ++It)
	{
		Points.Add(It->GetActorLocation());
		break;
	}
	return Points;
}

void ATTABotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Route) DriveRunner(DeltaSeconds);
	else WalkBuildStops(DeltaSeconds);
}

void ATTABotController::WalkBuildStops(float DeltaSeconds)
{
	ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(GetPawn());
	if (!Runner || BuildStops.Num() == 0) return;
	if (BuildPause > 0.f)
	{
		BuildPause -= DeltaSeconds;
		return;
	}
	const FVector ToStop = BuildStops[BuildStop] - Runner->GetActorLocation();
	if (ToStop.Size2D() <= BuildArriveCm)
	{
		BuildPause = BuildPauseSeconds; // stands there "placing" it
		BuildStop = (BuildStop + 1) % BuildStops.Num();
		return;
	}
	Runner->AddMovementInput(ToStop.GetSafeNormal2D());
}
