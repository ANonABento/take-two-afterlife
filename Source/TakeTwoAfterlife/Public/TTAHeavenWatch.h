#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "TTAHeavenWatch.generated.h"

class ACameraActor;
class ASceneCapture2D;
class ATTAFinishGate;
class UImage;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWidgetComponent;

// Big-screen feed of the live race. The image is a render target, so the widget
// only stores the brush.
UCLASS()
class TAKETWOAFTERLIFE_API UTTACamFeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetFeed(UTexture* Texture);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY() TObjectPtr<UImage> Image;
};

// Couch, cloud and TV spawned in the sky of the race map. The player's view sits
// on the couch; the screen cycles the race cameras. Not ATTACinema: that actor
// switches the whole-world look, which would tint the course.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAHeavenWatch : public AActor
{
	GENERATED_BODY()

public:
	ATTAHeavenWatch();

	// Views are world transforms for the feed camera (location + look rotation).
	void Start(const TArray<FTransform>& InViews);
	AActor* GetSeatCamera();
	// Where the dead angel stands, facing the TV. The seat camera stays behind them.
	FTransform GetSeatTransform() const;
	// Swap the screen off the live race and onto a replay render target. Stops camera cycling.
	void ShowTexture(UTexture* Texture);
	// Where the player stands once the reel is over, facing the continue gate.
	FTransform GetWalkTransform() const;
	// A walkable pad and a gate. Walking through the gate asks the game mode for the next round.
	void SpawnContinueGate();
	void ClearContinueGate();

	virtual void Tick(float DeltaSeconds) override;

private:
	void ShowView(int32 Index);

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Cloud;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Couch;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Screen;
	UPROPERTY() TObjectPtr<UWidgetComponent> ScreenWidget;
	UPROPERTY() TObjectPtr<UTTACamFeedWidget> Feed;
	UPROPERTY() TObjectPtr<ACameraActor> SeatCamera;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> WalkFloor;
	UPROPERTY() TObjectPtr<ATTAFinishGate> ContinueGate;
	UPROPERTY() TObjectPtr<ASceneCapture2D> FeedCam;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	TArray<FTransform> Views;
	float SwitchIn = 0.f;
	int32 ViewIndex = 0;
	bool bLive = true;
};
