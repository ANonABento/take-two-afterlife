#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "tta/MenuFlow.h"
#include "TTAPlayerState.generated.h"

// Lobby cosmetics (colour, hat, face), replicated and kept across seamless travel.
UCLASS()
class TAKETWOAFTERLIFE_API ATTAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	tta::PlayerLook GetLook() const { return tta::PlayerLook{Colour, Hat, Face}; }
	// Ignored (with a warning) when an index is out of range.
	UFUNCTION(Server, Reliable) void ServerSetLook(int32 NewColour, int32 NewHat, int32 NewFace);

	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated) int32 Colour = tta::PlayerLook{}.colour;
	UPROPERTY(Replicated) int32 Hat = tta::PlayerLook{}.hat;
	UPROPERTY(Replicated) int32 Face = tta::PlayerLook{}.face;
};
