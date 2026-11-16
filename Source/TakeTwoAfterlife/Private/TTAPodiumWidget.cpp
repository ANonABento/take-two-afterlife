#include "TTAPodiumWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "TTAUIStyle.h"
#include "tta/MenuFlow.h"

namespace
{
constexpr float kStepW = 300.f, kBackgroundOpacity = 0.6f;
// Place (0 = winner) -> block height and fill. Shown left to right as 2nd, 1st, 3rd.
constexpr float kStepH[] = {420.f, 300.f, 220.f};
constexpr int32 kStepFill[] = {tta::kYellow, tta::kSky, tta::kOrange};
constexpr int32 kShowOrder[] = {1, 0, 2};
}

void UTTAPodiumWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const UTTAUIStyle& Style = UTTAUIStyle::Get();

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Style.ApplyBackground(Background, ETTAUIScreen::Podium, tta::kInk, kBackgroundOpacity);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box);
	BoxSlot->SetPadding(Style.SafeArea);
	BoxSlot->SetHorizontalAlignment(HAlign_Center);
	BoxSlot->SetVerticalAlignment(VAlign_Bottom);
	Box->AddChild(UTTAUIStyle::MakeBanner(WidgetTree, FText::FromString(TEXT("AND THE WINNER IS..."))));
	Steps = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChild(Steps);
}

void UTTAPodiumWidget::SetPodium(const TArray<int32>& PlayerIds)
{
	check(Steps);
	Steps->ClearChildren();
	for (const int32 Place : kShowOrder)
	{
		if (!PlayerIds.IsValidIndex(Place)) continue;
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(kStepW);
		Size->SetHeightOverride(kStepH[Place]);
		UHorizontalBoxSlot* Cell = CastChecked<UHorizontalBoxSlot>(Steps->AddChild(Size));
		Cell->SetVerticalAlignment(VAlign_Bottom);
		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
		Size->SetContent(Stack);
		UBorder* Card = UTTAUIStyle::AddCard(WidgetTree, Stack, kStepFill[Place]);
		UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>();
		Card->SetContent(Text);
		UTTAUIStyle::AddLabel(WidgetTree, Text, FString::Printf(TEXT("%d"), Place + 1), UTTAUIStyle::kH1, true, tta::kBone);
		UTTAUIStyle::AddLabel(WidgetTree, Text, FString::Printf(TEXT("Player %d"), PlayerIds[Place]), UTTAUIStyle::kBody, false, tta::kInk);
	}
}
