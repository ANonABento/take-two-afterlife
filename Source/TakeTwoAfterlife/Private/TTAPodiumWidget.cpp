#include "TTAPodiumWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TTAUIStyle.h"
#include "tta/MenuFlow.h"

namespace
{
constexpr float kStepW = 300.f, kBackgroundOpacity = 0.6f;
// Place (0 = winner) -> block height. Shown left to right as 2nd, 1st, 3rd.
constexpr float kStepH[] = {420.f, 300.f, 220.f};
constexpr int32 kShowOrder[] = {1, 0, 2};
}

void UTTAPodiumWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const UTTAUIStyle& Style = UTTAUIStyle::Get();

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Style.ApplyBackground(Background, ETTAUIScreen::Podium, tta::kSky, kBackgroundOpacity);
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
	UOverlaySlot* StepsSlot = Root->AddChildToOverlay(Steps);
	StepsSlot->SetPadding(Style.SafeArea);
	StepsSlot->SetHorizontalAlignment(HAlign_Center);
	StepsSlot->SetVerticalAlignment(VAlign_Bottom);
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
		UHorizontalBoxSlot* Cell = Steps->AddChildToHorizontalBox(Size);
		Cell->SetVerticalAlignment(VAlign_Bottom);
		Cell->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, UTTAUIStyle::Space(3)));
		UBorder* Block = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
		Size->SetContent(Block);
		UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>();
		Block->SetContent(Text);
		const ETTAColor RankColour = Place == 0 ? ETTAColor::Yellow : ETTAColor::Ink;
		Text->AddChildToVerticalBox(UTTAUIStyle::MakeLabel(WidgetTree, FText::AsNumber(Place + 1), ETTAFont::H1, RankColour))
			->SetHorizontalAlignment(HAlign_Center);
		Text->AddChildToVerticalBox(UTTAUIStyle::MakeLabel(WidgetTree,
			FText::Format(NSLOCTEXT("TTA", "PodiumPlayer", "Player {0}"), FText::AsNumber(PlayerIds[Place])), ETTAFont::Body))
			->SetHorizontalAlignment(HAlign_Center);
	}
}
