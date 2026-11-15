#include "TTAResultsWidget.h"

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
constexpr float kPanelW = 760.f, kBackgroundOpacity = 0.85f;
}

void UTTAResultsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const UTTAUIStyle& Style = UTTAUIStyle::Get();

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Style.ApplyBackground(Background, ETTAUIScreen::Results, tta::kSky, kBackgroundOpacity);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* Column = WidgetTree->ConstructWidget<USizeBox>();
	Column->SetWidthOverride(kPanelW);
	UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column);
	ColumnSlot->SetPadding(Style.SafeArea);
	ColumnSlot->SetHorizontalAlignment(HAlign_Center);
	ColumnSlot->SetVerticalAlignment(VAlign_Center);
	UBorder* Panel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	Column->SetContent(Panel);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Column->SetContent(Box);
	Box->AddChild(UTTAUIStyle::MakeBanner(WidgetTree, FText::FromString(TEXT("RESULTS"))));
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
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		List->AddChildToVerticalBox(Line)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, UTTAUIStyle::Space(2)));
		// FTTAResultRow::Colour is a palette index, which is also the ETTAColor value.
		Line->AddChild(UTTAUIStyle::MakeLabel(WidgetTree, FText::Format(NSLOCTEXT("TTA", "ResultsRank", "{0}."), FText::AsNumber(i + 1)),
			ETTAFont::H2, static_cast<ETTAColor>(Rows[i].Colour)));
		Line->AddChild(UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(Rows[i].Name), ETTAFont::Body));
		Line->AddChild(UTTAUIStyle::MakeLabel(WidgetTree,
			FText::Format(NSLOCTEXT("TTA", "ResultsPoints", "{0} pts"), FText::AsNumber(Rows[i].Points)), ETTAFont::H2));
		for (UPanelSlot* Cell : Line->GetSlots())
		{
			UHorizontalBoxSlot* H = CastChecked<UHorizontalBoxSlot>(Cell);
			H->SetVerticalAlignment(VAlign_Center);
			H->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, UTTAUIStyle::Space(4)));
		}
		CastChecked<UHorizontalBoxSlot>(Line->GetSlots()[1])->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}
