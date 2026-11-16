#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TTAResultsWidget.generated.h"

class UVerticalBox;

USTRUCT()
struct FTTAResultRow
{
	GENERATED_BODY()

	UPROPERTY() FString Name;
	UPROPERTY() int32 Points = 0;
	// Palette index of the player's lobby colour.
	UPROPERTY() int32 Colour = 0;
};

// Results screen: one sticker row per player, best first. Styled by UTTAUIStyle.
UCLASS()
class TAKETWOAFTERLIFE_API UTTAResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetRows(TArray<FTTAResultRow> Rows);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY() TObjectPtr<UVerticalBox> List;
};
