#include "TTABotController.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TTACinema.h"
#include "TTAFinishGate.h"
#include "TTARunnerPawn.h"

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
}

void ATTABotController::StartRun(const TArray<FVector>& RouteCm)
{
	check(Brain);
	checkf(RouteCm.Num() > 0, TEXT("ATTABotController: empty route (no TTARoute markers or finish gate)"));
	std::vector<tta::RoutePoint> Points;
	for (const FVector& P : RouteCm) Points.push_back({P.X / 100.0, P.Y / 100.0, P.Z / 100.0});
	Route = std::make_unique<tta::BotRoute>(std::move(Points));
	Plan = Brain->planRun();
	bBuiltThisRound = false;
	bWasGroundAhead = true;
	bSkipThisGap = false;
	LastLife = tta::LifeState::Alive;
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
	DriveRunner(DeltaSeconds);
}
