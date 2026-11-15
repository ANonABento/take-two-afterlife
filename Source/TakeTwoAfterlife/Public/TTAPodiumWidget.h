#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTAPodiumWidget.generated.h"

class UHorizontalBox;

// Podium: 2nd / 1st / 3rd blocks from ATTACinema::GetPodium() (player ids, winner first).
// Styled by UTTAUIStyle.
UCLASS()
class TAKETWOAFTERLIFE_API UTTAPodiumWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPodium(const TArray<int32>& PlayerIds);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY() TObjectPtr<UHorizontalBox> Steps;
};
