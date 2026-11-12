#include "TTARunDirector.h"

#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "TTAAudioDirector.h"
#include "TTARunnerPawn.h"

namespace
{
tta::BoneTransform ToBone(const FTransform& T)
{
	const FVector P = T.GetLocation() / 100.0; // cm -> m
	const FQuat Q = T.GetRotation();
	return {{static_cast<float>(P.X), static_cast<float>(P.Y), static_cast<float>(P.Z)},
		{static_cast<float>(Q.X), static_cast<float>(Q.Y), static_cast<float>(Q.Z), static_cast<float>(Q.W)}};
}
}

ATTARunDirector::ATTARunDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATTARunDirector::StartRun(const TArray<ATTARunnerPawn*>& InRunners, float RunSeconds, int32 InAbsentOwners)
{
	check(HasAuthority());
	Runners.Reset();
	Finished.Reset();
	HeavenCam = nullptr;
	std::vector<int> Ids;
	for (ATTARunnerPawn* Runner : InRunners)
	{
		Ids.push_back(Runners.Add(Runner));
	}
	check(InAbsentOwners >= 0);
	AbsentOwners = InAbsentOwners;
	for (int32 i = 0; i < AbsentOwners; ++i)
	{
		Ids.push_back(Runners.Num() + i);
	}
	Recorder = MakeUnique<tta::RunRecorder>(std::move(Ids), RunSeconds);
	AdvanceRun(0.f); // frame 0
}

void ATTARunDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsRunning() && HasAuthority())
	{
		AdvanceRun(DeltaSeconds);
	}
}

void ATTARunDirector::AdvanceRun(float DeltaSeconds)
{
	check(IsRunning());
	for (int32 Due = Recorder->advance(DeltaSeconds); Due > 0; --Due)
	{
		CaptureFrame();
	}
	std::vector<int> Unfinished;
	for (int32 i = 0; i < Runners.Num(); ++i)
	{
		if (!Finished.Contains(Runners[i])) Unfinished.push_back(i);
	}
	for (const int Victim : Recorder->takeSmites(Unfinished))
	{
		Smite(Runners[Victim]);
	}
}

void ATTARunDirector::CaptureFrame()
{
	std::vector<tta::ReplayPose> Poses;
	for (const ATTARunnerPawn* Runner : Runners)
	{
		tta::ReplayPose Pose;
		Pose[0] = ToBone(Runner->GetActorTransform());
		const USkeletalMeshComponent* Mesh = Runner->GetMesh();
		const int32 BoneCount = Mesh && Mesh->GetSkeletalMeshAsset() ? Mesh->GetNumBones() : 0;
		for (int32 b = 1; b < tta::kReplayBones; ++b)
		{
			// First 15 bones of the skeleton; a mesh with fewer repeats the root.
			Pose[b] = b - 1 < BoneCount ? ToBone(Mesh->GetBoneTransform(b - 1)) : Pose[0];
		}
		Poses.push_back(Pose);
	}
	Poses.resize(Poses.size() + AbsentOwners); // not on the course: identity poses, never shown
	Recorder->addFrame(Poses);
}

tta::RoundInput ATTARunDirector::GetRoundInput() const
{
	std::set<int> FinishedIds;
	for (const ATTARunnerPawn* Runner : Finished) FinishedIds.insert(PlayerId(Runner));
	return Recorder->roundInput(FinishedIds);
}

void ATTARunDirector::MarkFinished(ATTARunnerPawn* Runner)
{
	if (!Runners.Contains(Runner) || Finished.Contains(Runner)) return;
	Finished.Add(Runner);
	GetWorld()->GetSubsystem<UTTAAudioDirector>()->PlayEvent(tta::AudioEvent::GateReached);
}

void ATTARunDirector::RecordHit(const tta::Placement& Trap, ATTARunnerPawn* Victim)
{
	check(IsRunning());
	const int32 Id = PlayerId(Victim);
	check(Id != INDEX_NONE);
	Recorder->recordHit(Trap, Id);
	GetWorld()->GetSubsystem<UTTAAudioDirector>()->PlayEvent(tta::AudioEvent::Wipeout);
}

void ATTARunDirector::Smite(ATTARunnerPawn* Victim)
{
	GetWorld()->GetSubsystem<UTTAAudioDirector>()->PlayEvent(tta::AudioEvent::Smite);
	const FRotator Yaw(0.f, Victim->GetActorRotation().Yaw, 0.f);
	const FVector Target = Victim->GetActorLocation();
	const FVector CamLocation = Target + Yaw.RotateVector(HeavenCamOffset);
	const FRotator LookAt = (Target - CamLocation).Rotation();
	if (!HeavenCam)
	{
		HeavenCam = GetWorld()->SpawnActor<ACameraActor>(CamLocation, LookAt);
	}
	HeavenCam->SetActorLocationAndRotation(CamLocation, LookAt);
	if (APlayerController* PC = Cast<APlayerController>(Victim->GetController()))
	{
		PC->SetViewTargetWithBlend(HeavenCam, 0.25f);
	}
	Victim->Hit(); // lightning VFX/SFX hang off OnImpact (task 25)
}
