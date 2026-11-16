#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "TTAHeavenWatch.generated.h"

class ACameraActor;
class ATTACinema;
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

// The heaven TV in the race map. The game mode streams Heaven.umap into the sky and
// hands its ATTACinema to UseStage: the feed goes on that screen, the player sits on
// that couch, and the map's own gate starts the next round. With no stage (the map
// failed to load) the actor shows a stand-in built from engine shapes in the same layout.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAHeavenWatch : public AActor
{
	GENERATED_BODY()

public:
	ATTAHeavenWatch();

	// Views are world transforms for the feed camera (location + look rotation).
	void Start(const TArray<FTransform>& InViews);
	// Move onto the streamed Heaven map's cinema and hide the stand-in.
	void UseStage(ATTACinema* InStage);
	bool HasStage() const { return Stage != nullptr; }
	AActor* GetSeatCamera();
	// Where the dead angel stands, facing the TV. The seat camera stays behind them.
	FTransform GetSeatTransform() const;
	// Swap the screen off the live race and onto a replay render target. Stops camera cycling.
	void ShowTexture(UTexture* Texture);
	// Where the player stands once the reel is over, facing the continue gate.
	FTransform GetWalkTransform() const;
	// Stand-in only: a gate whose trigger asks the game mode for the next round. On the
	// Heaven map the pearly gate is already there, so this does nothing.
	void SpawnContinueGate();
	void ClearContinueGate();

	virtual void Tick(float DeltaSeconds) override;

private:
	void ShowView(int32 Index);
	// Seat camera and screen widget onto their spots.
	void PlaceRig();
	// Cinema-space cm to world.
	FVector Spot(const FVector& Local) const;

	UPROPERTY() TObjectPtr<ATTACinema> Stage;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Cloud;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Couch;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Screen;
	UPROPERTY() TObjectPtr<UWidgetComponent> ScreenWidget;
	UPROPERTY() TObjectPtr<UTTACamFeedWidget> Feed;
	UPROPERTY() TObjectPtr<ACameraActor> SeatCamera;
	UPROPERTY() TObjectPtr<ATTAFinishGate> ContinueGate;
	UPROPERTY() TObjectPtr<ASceneCapture2D> FeedCam;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
	TArray<FTransform> Views;
	float SwitchIn = 0.f;
	int32 ViewIndex = 0;
	bool bLive = true;
};
