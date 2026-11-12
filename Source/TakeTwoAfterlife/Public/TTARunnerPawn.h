#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "tta/PawnLife.h"
#include "TTARunnerPawn.generated.h"

class UCameraShakeBase;
class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETTAForm : uint8
{
	Angel,
	Human
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTTAImpactSignature, ATTARunnerPawn*, Victim);

// Runner pawn (plan req 8): walk, jump, dive; a hit plays the victim-only impact
// frame, then ragdolls, then respawns at the start. Timing lives in tta::PawnLife.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API ATTARunnerPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ATTARunnerPawn();

	// A trap or ball hit this runner. Ignored unless alive.
	UFUNCTION(BlueprintCallable) void Hit();
	// Forward lunge; only from the ground while alive.
	UFUNCTION(BlueprintCallable) void Dive();
	// Angel shows wings + halo; human (runner) form hides both and shows the hat.
	UFUNCTION(BlueprintCallable) void SetForm(ETTAForm NewForm);
	UFUNCTION(BlueprintCallable) void SetStartLocation(const FVector& Location) { StartLocation = Location; }

	// Advances the hit timers in world seconds (not slowed by the freeze). Called from Tick.
	void AdvanceLife(float WorldDeltaSeconds);

	tta::LifeState GetLifeState() const { return Life.state(); }
	FVector GetStartLocation() const { return StartLocation; }
	float GetFlash() const { return Flash; }
	ETTAForm GetForm() const { return Form; }

	// SFX hook: fires on every accepted hit, at the start of the impact frame.
	UPROPERTY(BlueprintAssignable) FTTAImpactSignature OnImpact;

	UPROPERTY(Config, EditAnywhere, Category = "TTA") TSoftObjectPtr<USkeletalMesh> RunnerMesh;
	UPROPERTY(EditAnywhere, Category = "TTA") TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY(EditAnywhere, Category = "TTA") TSubclassOf<UCameraShakeBase> ImpactShake;
	UPROPERTY(EditAnywhere, Category = "TTA") float DiveSpeed = 900.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float DiveLift = 250.f;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wings;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Halo;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hat;

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

protected:
	virtual void BeginPlay() override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void SetFlash(float Value);
	void StartRagdoll();
	void Respawn();

	tta::PawnLife Life;
	FVector StartLocation = FVector::ZeroVector;
	FTransform MeshRelative;
	ETTAForm Form = ETTAForm::Human;
	float Flash = 0.f;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMaterials;
};
