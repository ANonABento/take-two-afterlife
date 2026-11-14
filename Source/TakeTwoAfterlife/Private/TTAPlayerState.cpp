#include "TTAPlayerState.h"

#include "Net/UnrealNetwork.h"

void ATTAPlayerState::ServerSetLook_Implementation(int32 NewColour, int32 NewHat, int32 NewFace)
{
	const bool bValid = NewColour >= 0 && NewColour < static_cast<int32>(tta::kPlayerColours.size())
		&& NewHat >= 0 && NewHat < static_cast<int32>(tta::kHats.size())
		&& NewFace >= 0 && NewFace < static_cast<int32>(tta::kFaces.size());
	if (!bValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATTAPlayerState: look %d/%d/%d out of range"), NewColour, NewHat, NewFace);
		return;
	}
	Colour = NewColour;
	Hat = NewHat;
	Face = NewFace;
}

void ATTAPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (ATTAPlayerState* Next = Cast<ATTAPlayerState>(PlayerState))
	{
		Next->Colour = Colour;
		Next->Hat = Hat;
		Next->Face = Face;
	}
}

void ATTAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATTAPlayerState, Colour);
	DOREPLIFETIME(ATTAPlayerState, Hat);
	DOREPLIFETIME(ATTAPlayerState, Face);
}
