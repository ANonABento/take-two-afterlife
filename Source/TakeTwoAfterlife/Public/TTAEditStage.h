#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "tta/Replay.h"
#include "TTAEditStage.generated.h"

class UPoseableMeshComponent;
class USceneCaptureComponent2D;
class USkeletalMesh;
class UTextureRenderTarget2D;

// Renders replay data for the editor: one poseable puppet per runner, posed from
// the replay, filmed by a scene capture placed at the chosen camera (live preview),
// plus one still thumbnail per camera. Local only, never replicated.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAEditStage : public AActor
{
	GENERATED_BODY()

public:
	ATTAEditStage();

	// Camera index = index into Views (the editor's camera list).
	void Init(const tta::Replay& InReplay, const TArray<FTransform>& InViews, USkeletalMesh* PuppetMesh);
	// Poses every puppet at ReplaySeconds and captures the preview through camera Camera.
	void ShowAt(double ReplaySeconds, int32 Camera, double Zoom);
	// Captures each camera's thumbnail at ReplaySeconds (call once after Init).
	void CaptureThumbnails(double ReplaySeconds);

	UTextureRenderTarget2D* GetPreview() const { return PreviewTarget; }
	UTextureRenderTarget2D* GetThumbnail(int32 Camera) const { return Thumbnails.IsValidIndex(Camera) ? Thumbnails[Camera] : nullptr; }
	const tta::Replay& GetReplay() const { return *Replay; }
	int32 NumCameras() const { return Views.Num(); }

	UPROPERTY(EditAnywhere, Category = "TTA") float BaseFov = 90.f;
	UPROPERTY(EditAnywhere, Category = "TTA") FIntPoint PreviewSize = FIntPoint(960, 540);
	UPROPERTY(EditAnywhere, Category = "TTA") FIntPoint ThumbnailSize = FIntPoint(192, 108);

private:
	void Pose(double ReplaySeconds);

	TOptional<tta::Replay> Replay;
	TArray<FTransform> Views;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> PreviewTarget;
	UPROPERTY() TArray<TObjectPtr<UTextureRenderTarget2D>> Thumbnails;
	UPROPERTY() TArray<TObjectPtr<UPoseableMeshComponent>> Puppets;
};
