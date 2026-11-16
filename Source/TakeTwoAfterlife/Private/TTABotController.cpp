#include "TTABotController.h"
#include "TTAPlacementMarker.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TTACinema.h"
#include "TTAGameMode.h"
#include "TTARunnerPawn.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTABots, Log, All);

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

void ATTABotController::StartRun(const FVector& InGateCm)
{
	check(Brain);
	GateCm = InGateCm;
	Race = std::make_unique<tta::BotRace>(static_cast<uint64>(PlayerState->GetPlayerId()));
	bRacing = true;
	BuildStops.Reset();
	Plan = Brain->planRun();
	bBuiltThisRound = false;
	bWasGroundAhead = true;
	bSkipThisGap = false;
	LastLife = tta::LifeState::Alive;
	LogClock = 0.f;
	UE_LOG(LogTTABots, Log, TEXT("%s: race to %s, pawn %s, PlayerState %s"), *GetName(), *GateCm.ToString(), *GetNameSafe(GetPawn()), *GetNameSafe(PlayerState));
}

void ATTABotController::StopRun()
{
	bRacing = false;
	Race.reset();
}

void ATTABotController::DriveRunner(float DeltaSeconds)
{
	ATTARunnerPawn* Runner = Cast<ATTARunnerPawn>(GetPawn());
	if (!bRacing || !Race || !Runner) return;
	const tta::LifeState Life = Runner->GetLifeState();
	if (Life != tta::LifeState::Alive)
	{
		LastLife = Life;
		return;
	}
	if (LastLife != tta::LifeState::Alive)
	{
		Race->restart(); // PawnLife put the runner back at the start
		LastLife = tta::LifeState::Alive;
	}
	const FVector Pos = Runner->GetActorLocation();
	FVector Fwd = Runner->GetActorForwardVector().GetSafeNormal2D();
	if (Fwd.IsNearlyZero()) Fwd = FVector(1.f, 0.f, 0.f);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal2D();
	auto GroundAt = [&](const FVector& Side)
	{
		const FVector From = Pos + Fwd * ProbeAheadCm + Side;
		FHitResult Floor;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TTABotFloor), false, Runner);
		return GetWorld()->LineTraceSingleByChannel(Floor, From, From - FVector(0.f, 0.f, 500.f), ECC_Visibility, Params);
	};

	tta::RaceSense Sense;
	Sense.pos = {Pos.X / 100.0, Pos.Y / 100.0, Pos.Z / 100.0};
	Sense.gate = {GateCm.X / 100.0, GateCm.Y / 100.0, GateCm.Z / 100.0};
	Sense.rightX = Right.X;
	Sense.rightY = Right.Y;
	Sense.groundLeft = GroundAt(-Right * 80.f);
	Sense.groundCenter = GroundAt(FVector::ZeroVector);
	Sense.groundRight = GroundAt(Right * 80.f);
	if (Sense.groundCenter) bSkipThisGap = false;
	else if (bWasGroundAhead) bSkipThisGap = Brain->rollFail(Plan); // one roll per gap
	bWasGroundAhead = Sense.groundCenter;
	Sense.refuseJump = bSkipThisGap;
	Sense.punchReady = Runner->CanPunch();
	Sense.skill = Plan.skill;
	Sense.dt = DeltaSeconds;
	if (const ATTAGameMode* Mode = GetWorld()->GetAuthGameMode<ATTAGameMode>())
	{
		for (const tta::Placement& P : Mode->GetLayout().all())
		{
			if (P.isCamera) continue;
			const FVector At = ATTAPlacementMarker::WorldTransform(FTTAPlacementRep::FromCore(P)).GetLocation();
			Sense.hazards.push_back({At.X / 100.0, At.Y / 100.0, At.Z / 100.0});
		}
	}
	for (TActorIterator<ATTARunnerPawn> It(GetWorld()); It; ++It)
	{
		if (*It == Runner || It->GetLifeState() != tta::LifeState::Alive) continue;
		const FVector At = It->GetActorLocation();
		Sense.rivals.push_back({At.X / 100.0, At.Y / 100.0, At.Z / 100.0});
	}

	const tta::RaceAction Step = Race->step(Sense);
	if (Step.respawn)
	{
		Runner->Hit(); // freeze -> ragdoll -> respawn at the start
		return;
	}
	if ((LogClock += DeltaSeconds) >= 1.f)
	{
		LogClock = 0.f;
		UE_LOG(LogTTABots, Verbose, TEXT("%s: pos %s dir (%.2f, %.2f) ground %d punch %d done %d"), *GetName(), *Pos.ToString(), Step.dirX, Step.dirY, Sense.groundCenter, Step.punch, Step.done);
	}
	if (Step.done) return;
	Runner->AddMovementInput(FVector(Step.dirX, Step.dirY, 0.0));
	if (Step.jump) Runner->Jump();
	if (Step.punch) Runner->Punch();
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

void ATTABotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bRacing) DriveRunner(DeltaSeconds);
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
