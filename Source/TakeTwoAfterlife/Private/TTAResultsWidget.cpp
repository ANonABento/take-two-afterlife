#include "TTAResultsWidget.h"

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
constexpr float kPanelW = 760.f, kBackgroundOpacity = 0.85f;
}

void UTTAResultsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const UTTAUIStyle& Style = UTTAUIStyle::Get();

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Style.ApplyBackground(Background, ETTAUIScreen::Results, tta::kPurple, kBackgroundOpacity);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* Column = WidgetTree->ConstructWidget<USizeBox>();
	Column->SetWidthOverride(kPanelW);
	UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column);
	ColumnSlot->SetPadding(Style.SafeArea);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Center);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->SetContent(Box);
	UTTAUIStyle::AddLabel(WidgetTree, Box, TEXT("RESULTS"), UTTAUIStyle::kH1, true, tta::kBone);
	List = WidgetTree->ConstructWidget<UVerticalBox>();
	Box->AddChild(List);
}

void UTTAResultsWidget::SetRows(TArray<FTTAResultRow> Rows)
{
	check(List);
	Rows.StableSort([](const FTTAResultRow& A, const FTTAResultRow& B) { return A.Points > B.Points; });
	List->ClearChildren();
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		UBorder* Card = UTTAUIStyle::AddCard(WidgetTree, List, tta::kBone);
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Card->SetContent(Line);
		UTTAUIStyle::AddLabel(WidgetTree, Line, FString::Printf(TEXT("%d."), i + 1), UTTAUIStyle::kH2, true, Rows[i].Colour);
		UTTAUIStyle::AddLabel(WidgetTree, Line, Rows[i].Name, UTTAUIStyle::kBody, false, tta::kInk);
		UTTAUIStyle::AddLabel(WidgetTree, Line, FString::Printf(TEXT("%d pts"), Rows[i].Points), UTTAUIStyle::kH2, true, tta::kInk);
		for (UPanelSlot* Cell : Line->GetSlots())
		{
			UHorizontalBoxSlot* H = CastChecked<UHorizontalBoxSlot>(Cell);
			H->SetVerticalAlignment(VAlign_Center);
			H->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, UTTAUIStyle::kCardPadding));
		}
		CastChecked<UHorizontalBoxSlot>(Line->GetSlots()[1])->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}
