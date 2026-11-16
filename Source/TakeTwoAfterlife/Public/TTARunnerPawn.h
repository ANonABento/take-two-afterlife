#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "tta/PawnLife.h"
#include "tta/Punch.h"
#include "TTACosmetics.h"
#include "TTARunnerPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UCameraShakeBase;
class USoundAttenuation;
class UVOIPTalker;
class UTTAMicRecorder;
class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;
class UDataTable;

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
	// Ragdoll off and movement off. Demo death uses this to stand the angel on the heaven couch.
	void SitOut();
	// Movement back on, so the heaven hub can be walked.
	void WalkHeaven();
	// Forward lunge; only from the ground while alive.
	UFUNCTION(BlueprintCallable) void Dive();
	// [LMB]/[Gamepad right face] punch (punch plan req 1-5). Asks the server to start it.
	UFUNCTION(BlueprintCallable) void Punch();
	// Gen 3 mesh already has wings. Both forms keep the hat.
	UFUNCTION(BlueprintCallable) void SetForm(ETTAForm NewForm);
	// Hat mesh/transform from DT_Cosmetics row HatIndex, face texture from row FaceIndex.
	UFUNCTION(BlueprintCallable) void ApplyLook(int32 HatIndex, int32 FaceIndex);
	// Mesh, absolute scale (so a scaled head bone cannot blow the hat up) and the painted MI_Hat_*.
	static void DressHat(UStaticMeshComponent* HatComp, USceneComponent* Body, FName Socket, const FTTACosmeticRow& Row);
	// Podium statue: angel form with this hat, face and body colour, no controller,
	// no movement, animation paused on its current pose.
	void FreezeAsStatue(int32 HatIndex, int32 FaceIndex, int32 Colour);
	UFUNCTION(BlueprintCallable) void SetStartLocation(const FVector& Location) { StartLocation = Location; }

	// Advances the hit timers in world seconds (not slowed by the freeze). Called from Tick.
	void AdvanceLife(float WorldDeltaSeconds);

	// Advances the punch timers (server only); the hit check runs when the window opens.
	void AdvancePunch(float WorldDeltaSeconds);

	tta::LifeState GetLifeState() const { return Life.state(); }
	tta::PunchPhase GetPunchPhase() const { return PunchState.phase(); }
	bool CanPunch() const;
	bool IsPunchImmune() const { return Life.punchImmune(); }
	FVector GetStartLocation() const { return StartLocation; }
	float GetFlash() const { return Flash; }
	ETTAForm GetForm() const { return Form; }

	// SFX hook: fires on every accepted hit, at the start of the impact frame.
	UPROPERTY(BlueprintAssignable) FTTAImpactSignature OnImpact;

	UPROPERTY(Config, EditAnywhere, Category = "TTA") TSoftObjectPtr<USkeletalMesh> RunnerMesh;
	UPROPERTY(Config, EditAnywhere, Category = "TTA") TSoftObjectPtr<UDataTable> CosmeticsTable;
	static inline const FName StatueTag = TEXT("TTA_Statue");
	UPROPERTY(EditAnywhere, Category = "TTA") FName HatSocket = TEXT("head");
	UPROPERTY(EditAnywhere, Category = "TTA") TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY(EditAnywhere, Category = "TTA") TSubclassOf<UCameraShakeBase> ImpactShake;
	UPROPERTY(EditAnywhere, Category = "TTA") float DiveSpeed = 900.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float DiveLift = 250.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float PunchRadius = 120.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float PunchReach = 90.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float PunchSpeed = 600.f;
	UPROPERTY(EditAnywhere, Category = "TTA") float PunchLift = 200.f;
	// Placeholder wind-up pose: the body swells to this scale over the wind-up, then snaps back.
	// Bone the capsule is snapped to when a stunned runner gets up.
	UPROPERTY(EditAnywhere, Category = "TTA") FName PelvisBone = TEXT("pelvis");
	// How far below the pelvis StopStun looks for floor.
	UPROPERTY(EditAnywhere, Category = "TTA") float GetUpTraceDepth = 500.f;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wings;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Halo;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Hat;
	// Content Warning-style handheld: hold [RMB] to look (and later edit) through it.
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Handheld;
	// Third-person view: boom behind the angel, pulled in by its collision probe instead of clipping meshes.
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
	// Records this runner's mic during Run (locally controlled runner only).
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTTAMicRecorder> MicRecorder;
	// Proximity voice: full volume inside VoiceFullVolumeCm, silent past VoiceSilentCm.
	UPROPERTY(Config, EditAnywhere, Category = "TTA") float VoiceFullVolumeCm = 500.f;
	UPROPERTY(Config, EditAnywhere, Category = "TTA") float VoiceSilentCm = 2500.f;

	// [M] mutes/unmutes this player's outgoing voice. Only stops network voice;
	// UTTAMicRecorder keeps its own capture, so the replay mic track still records.
	UFUNCTION(BlueprintCallable) void ToggleVoiceMute();
	bool IsVoiceMuted() const { return bVoiceMuted; }

	UFUNCTION(BlueprintCallable) void SetFilming(bool bOn);
	bool IsFilming() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

protected:
	virtual void BeginPlay() override;
	// Proximity voice: other players' voices play from their runner, fading with distance.
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;

private:
	void RaiseCamera() { SetFilming(true); }
	void LowerCamera() { SetFilming(false); }
	void MoveForward(float Value);
	void MoveRight(float Value);
	void SetFlash(float Value);
	void StartRagdoll();
	void Respawn();
	// Ragdoll off, mesh back on the capsule, collision and walking restored.
	void RestoreBody();
	// Get up in place after a punch stun. Server only: trace the ragdoll pelvis down to the floor
	// and send that landing spot to everyone through MulticastGetUp. Never the start or an old spot.
	void StopStun();
	UFUNCTION(NetMulticast, Reliable) void MulticastGetUp(FVector_NetQuantize Landing);
	void PunchHitCheck();

	UFUNCTION(Server, Reliable) void ServerPunch();
	UFUNCTION(NetMulticast, Reliable) void MulticastPunched(FVector_NetQuantizeNormal Direction);
	// Server -> everyone: a punch started, show the wind-up and play the whoosh.
	UFUNCTION(NetMulticast, Unreliable) void MulticastWindUp();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// Arm pose (punch arm task): server mirrors PunchState.phase() here, every machine poses from it.
	void UpdatePunchArm(float WorldDeltaSeconds);
	UPROPERTY(Replicated) uint8 PunchPhaseRep = 0;
	tta::PunchPhase ArmPhase = tta::PunchPhase::Idle;
	float ArmPhaseTime = 0.f;
	bool bWarnedAnimClass = false;

	tta::PawnLife Life;
	tta::Punch PunchState;
	FVector StartLocation = FVector::ZeroVector;
	FTransform MeshRelative;
	ETTAForm Form = ETTAForm::Human;
	float Flash = 0.f;
	bool bVoiceMuted = false;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> FlashMaterials;
	UPROPERTY() TObjectPtr<UVOIPTalker> Talker;
	UPROPERTY() TObjectPtr<USoundAttenuation> VoiceAttenuation;
};
