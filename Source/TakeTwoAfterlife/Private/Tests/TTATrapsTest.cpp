#include "Misc/AutomationTest.h"

#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TTARunDirector.h"
#include "TTARunnerPawn.h"
#include "TTATrapActor.h"
#include "tta/Placement.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
UWorld* MakeTestWorld()
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	return World;
}

void DestroyTestWorld(UWorld* World)
{
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
}

tta::Placement TrapPlacement(tta::TrapType Type, int Id, int Owner)
{
	tta::Placement P;
	P.id = Id;
	P.owner = Owner;
	P.trap = Type;
	const tta::TrapSpec& Spec = tta::trapSpec(Type);
	P.settings = {Spec.settings[0].max, Spec.settings[1].min};
	return P;
}
}

// Req 3 in the world: two actors with the same settings show the same pose at
// the same time, and that pose is exactly Core's pose().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTATrapsDeterministicTest, "TTA.Traps.Deterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTATrapsDeterministicTest::RunTest(const FString& Parameters)
{
	UWorld* World = MakeTestWorld();
	const double Times[] = {0.0, 0.05, 0.37, 1.0, 2.5, 7.25, 31.0};
	for (const tta::TrapSpec& Spec : tta::trapCatalogue())
	{
		ATTATrapActor* A = World->SpawnActor<ATTATrapActor>();
		ATTATrapActor* B = World->SpawnActor<ATTATrapActor>();
		const tta::Placement P = TrapPlacement(Spec.type, 1, 0);
		A->InitFromPlacement(P);
		B->InitFromPlacement(P);
		for (const double T : Times)
		{
			A->ApplyPoseAt(T);
			B->ApplyPoseAt(T);
			const tta::TrapPose Expected = tta::pose(P.trap, P.settings, T);
			const FString What = FString::Printf(TEXT("%hs at %.2f s"), Spec.name, T);
			TestTrue(What + TEXT(": actors agree"),
				A->Moving->GetRelativeTransform().Equals(B->Moving->GetRelativeTransform(), 0.f)
				&& A->Projectile->GetRelativeTransform().Equals(B->Projectile->GetRelativeTransform(), 0.f));
			TestTrue(What + TEXT(": matches Core pose()"),
				A->Moving->GetRelativeTransform().Equals(ATTATrapActor::MovingTransform(P.trap, Expected), 1e-4f)
				&& A->Projectile->GetRelativeTransform().Equals(ATTATrapActor::ProjectileTransform(P.trap, Expected), 1e-4f));
		}
		const std::int64_t Period = tta::trapPeriodMs(P.trap, P.settings);
		if (Period > 0)
		{
			A->ApplyPoseAt(0.37);
			const FTransform First = A->Moving->GetRelativeTransform();
			A->ApplyPoseAt(0.37 + Period / 1000.0);
			TestTrue(FString::Printf(TEXT("%hs loops"), Spec.name), A->Moving->GetRelativeTransform().Equals(First, 1e-4f));
		}
	}
	DestroyTestWorld(World);
	return true;
}

// Server-side hit -> Core replay event with trap id + owner; 20 Hz frames;
// timeout smites the unfinished runner and points a Heaven Cam at them.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTTATrapsHitRecordedTest, "TTA.Traps.HitRecorded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTATrapsHitRecordedTest::RunTest(const FString& Parameters)
{
	UWorld* World = MakeTestWorld();
	ATTARunnerPawn* Victim = World->SpawnActor<ATTARunnerPawn>(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator);
	ATTARunnerPawn* Builder = World->SpawnActor<ATTARunnerPawn>(FVector(0.f, 300.f, 200.f), FRotator::ZeroRotator);
	ATTARunDirector* Director = World->SpawnActor<ATTARunDirector>();
	ATTATrapActor* Trap = World->SpawnActor<ATTATrapActor>();
	if (TestNotNull(TEXT("victim"), Victim) && TestNotNull(TEXT("builder"), Builder)
		&& TestNotNull(TEXT("director"), Director) && TestNotNull(TEXT("trap"), Trap))
	{
		Trap->InitFromPlacement(TrapPlacement(tta::TrapType::Hammer, 7, 1));
		TestFalse(TEXT("no hit before Run"), Trap->TryHit(Victim));

		Director->StartRun({Victim, Builder}, 2.f);
		Director->AdvanceRun(0.2f);
		TestEqual(TEXT("20 Hz frames"), Director->GetReplay().frameCount(), 5);

		TestTrue(TEXT("hit accepted"), Trap->TryHit(Victim));
		TestFalse(TEXT("frozen victim not hit twice"), Trap->TryHit(Victim));
		const auto& Events = Director->GetReplay().events();
		if (TestEqual(TEXT("one event"), static_cast<int32>(Events.size()), 1))
		{
			TestTrue(TEXT("hit type"), Events[0].type == tta::ReplayEventType::Hit);
			TestEqual(TEXT("time"), static_cast<int32>(Events[0].timeMs), 200);
			TestEqual(TEXT("victim"), Events[0].victim, Director->PlayerId(Victim));
			TestEqual(TEXT("trap id"), Events[0].trapId, 7);
			TestEqual(TEXT("trap owner"), Events[0].trapOwner, 1);
		}
		TestTrue(TEXT("victim frozen"), Victim->GetLifeState() == tta::LifeState::Frozen);

		Victim->AdvanceLife(10.f); // back to alive at the start
		Director->MarkFinished(Builder);
		Director->AdvanceRun(2.f);
		const auto& After = Director->GetReplay().events();
		if (TestEqual(TEXT("smite added"), static_cast<int32>(After.size()), 2))
		{
			TestTrue(TEXT("smite type"), After[1].type == tta::ReplayEventType::Smite);
			TestEqual(TEXT("smite victim"), After[1].victim, Director->PlayerId(Victim));
			TestEqual(TEXT("smite at timeout"), static_cast<int32>(After[1].timeMs), 2000);
		}
		TestTrue(TEXT("smitten runner hit"), Victim->GetLifeState() == tta::LifeState::Frozen);
		TestTrue(TEXT("finished runner spared"), Builder->GetLifeState() == tta::LifeState::Alive);
		if (TestNotNull(TEXT("Heaven Cam"), Director->GetHeavenCam()))
		{
			const FVector ToVictim = (Victim->GetActorLocation() - Director->GetHeavenCam()->GetActorLocation()).GetSafeNormal();
			TestTrue(TEXT("Heaven Cam looks at victim"), FVector::DotProduct(ToVictim, Director->GetHeavenCam()->GetActorForwardVector()) > 0.99f);
		}
		Director->AdvanceRun(1.f);
		TestEqual(TEXT("smite only once"), static_cast<int32>(Director->GetReplay().events().size()), 2);
	}
	DestroyTestWorld(World);
	return true;
}

#endif
