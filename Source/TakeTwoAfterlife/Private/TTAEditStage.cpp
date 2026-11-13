#include "TTAEditStage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

namespace
{
FTransform ToUE(const tta::BoneTransform& B)
{
	const FQuat Q(B.rotation.x, B.rotation.y, B.rotation.z, B.rotation.w);
	return FTransform(Q, FVector(B.position.x, B.position.y, B.position.z) * 100.0); // m -> cm
}
}

ATTAEditStage::ATTAEditStage()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(RootComponent);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
}

void ATTAEditStage::Init(const tta::Replay& InReplay, const TArray<FTransform>& InViews, USkeletalMesh* PuppetMesh)
{
	check(InViews.Num() > 0);
	Replay = InReplay;
	Views = InViews;
	PreviewTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, PreviewSize.X, PreviewSize.Y);
	Thumbnails.Reset();
	for (int32 i = 0; i < Views.Num(); ++i)
	{
		Thumbnails.Add(UKismetRenderingLibrary::CreateRenderTarget2D(this, ThumbnailSize.X, ThumbnailSize.Y));
	}
	for (UPoseableMeshComponent* Puppet : Puppets) Puppet->DestroyComponent();
	Puppets.Reset();
	for (size_t i = 0; i < Replay->players().size(); ++i)
	{
		UPoseableMeshComponent* Puppet = NewObject<UPoseableMeshComponent>(this);
		Puppet->SetSkinnedAssetAndUpdate(PuppetMesh);
		Puppet->SetupAttachment(RootComponent);
		Puppet->RegisterComponent();
		Puppets.Add(Puppet);
	}
}

void ATTAEditStage::Pose(double ReplaySeconds)
{
	for (int32 i = 0; i < Puppets.Num(); ++i)
	{
		const tta::ReplayPose Pose = Replay->sample(Replay->players()[i], ReplaySeconds);
		UPoseableMeshComponent* Puppet = Puppets[i];
		Puppet->SetWorldTransform(ToUE(Pose[0]));
		// Same bone order the run director recorded: the first 15 bones of the skeleton.
		const int32 BoneCount = Puppet->GetNumBones();
		for (int32 b = 1; b < tta::kReplayBones && b - 1 < BoneCount; ++b)
		{
			Puppet->SetBoneTransformByName(Puppet->GetBoneName(b - 1), ToUE(Pose[b]), EBoneSpaces::WorldSpace);
		}
	}
}

void ATTAEditStage::ShowAt(double ReplaySeconds, int32 Camera, double Zoom)
{
	check(Views.IsValidIndex(Camera));
	Pose(ReplaySeconds);
	Capture->SetWorldTransform(Views[Camera]);
	Capture->FOVAngle = BaseFov / Zoom;
	Capture->TextureTarget = PreviewTarget;
	Capture->CaptureScene();
}

void ATTAEditStage::CaptureThumbnails(double ReplaySeconds)
{
	Pose(ReplaySeconds);
	Capture->FOVAngle = BaseFov;
	for (int32 i = 0; i < Views.Num(); ++i)
	{
		Capture->SetWorldTransform(Views[i]);
		Capture->TextureTarget = Thumbnails[i];
		Capture->CaptureScene();
	}
}
