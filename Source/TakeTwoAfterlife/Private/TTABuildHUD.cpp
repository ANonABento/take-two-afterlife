#include "TTABuildHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "TTABuildPawn.h"
#include "TTACinema.h"
#include "TTAGameState.h"
#include "TTAPlayerState.h"
#include "TTACinema.h"
#include "TTAPodiumWidget.h"
#include "EngineUtils.h"
#include "tta/MenuFlow.h"
#include "tta/ScoreBoard.h"
#include "GameFramework/PlayerState.h"
#include "TTAPlayerState.h"
#include "TTAResultsWidget.h"
#include "TTAUIStyle.h"
#include "tta/MenuFlow.h"
#include "GameFramework/PlayerController.h"

namespace
{
constexpr float kDesignW = 1920.f, kPad = 20.f, kPanelOpacity = 0.85f;
constexpr int32 kTimerWarnSeconds = 10;
}

float ATTABuildHUD::UiScale() const
{
	return Canvas->ClipY / UTTAUIStyle::kDesignHeight;
}

void ATTABuildHUD::DrawCard(float X, float Y, float W, float H, int32 Fill)
{
	const float S = UiScale();
	const float O = UTTAUIStyle::kOutline;
	DrawRect(UTTAUIStyle::Colour(tta::kInk), X * S, Y * S, W * S, H * S);
	if (UTexture2D* Image = UTTAUIStyle::Get().LoadBackground(ETTAUIScreen::HUD))
	{
		DrawTexture(Image, (X + O) * S, (Y + O) * S, (W - 2 * O) * S, (H - 2 * O) * S, 0.f, 0.f, 1.f, 1.f);
		return;
	}
	DrawRect(UTTAUIStyle::Colour(Fill).CopyWithNewOpacity(kPanelOpacity), (X + O) * S, (Y + O) * S, (W - 2 * O) * S, (H - 2 * O) * S);
}

float ATTABuildHUD::MeasureText(const FString& Text, int32 Size, bool bDisplay)
{
	UFont* Font = bDisplay ? DisplayFont : BodyFont;
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, static_cast<float>(Size) / Font->GetMaxCharHeight());
	return W;
}

float ATTABuildHUD::DrawStyledText(const FString& Text, int32 Colour, float X, float Y, int32 Size, bool bDisplay)
{
	UFont* Font = bDisplay ? DisplayFont : BodyFont;
	const float S = UiScale();
	DrawText(Text, UTTAUIStyle::Colour(Colour), X * S, Y * S, Font, S * Size / Font->GetMaxCharHeight());
	return MeasureText(Text, Size, bDisplay);
}

void ATTABuildHUD::ShowResults()
{
	APlayerController* PC = GetOwningPlayerController();
	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	if (!PC || !State) return;
	if (!Results)
	{
		TArray<FTTAResultRow> Rows;
		for (const APlayerState* PS : State->PlayerArray)
		{
			const ATTAPlayerState* TTA = Cast<ATTAPlayerState>(PS);
			Rows.Add({PS->GetPlayerName(), FMath::RoundToInt(PS->GetScore()),
				TTA ? tta::kPlayerColours[TTA->GetLook().colour] : static_cast<int32>(tta::kBone)});
		}
		Results = CreateWidget<UTTAResultsWidget>(PC, UTTAResultsWidget::StaticClass());
		Results->SetRows(MoveTemp(Rows));
		Results->AddToViewport();
	}
	if (!PodiumWidget)
	{
		for (TActorIterator<ATTACinema> It(GetWorld()); It; ++It)
		{
			if (It->GetPodium().IsEmpty()) continue;
			PodiumWidget = CreateWidget<UTTAPodiumWidget>(PC, UTTAPodiumWidget::StaticClass());
			PodiumWidget->SetPodium(It->GetPodium());
			PodiumWidget->AddToViewport(1);
			break;
		}
	}
}

namespace
{
UTextBlock* IconLabel(UHorizontalBox* Row)
{
	return CastChecked<UTextBlock>(Row->GetChildAt(1));
}

void SetShown(UWidget* Widget, bool bShown)
{
	Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void SetLine(UTextBlock* Line, const FString& Text, ETTAColor Colour)
{
	Line->SetText(FText::FromString(Text));
	Line->SetColorAndOpacity(FSlateColor(UTTAUIStyle::Color(Colour)));
}
}

void UTTABuildHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const float Gap = UTTAUIStyle::Space(3);

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	UVerticalBox* Screen = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* ScreenSlot = Root->AddChildToCanvas(Screen);
	ScreenSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	ScreenSlot->SetOffsets(UTTAUIStyle::Get().SafeArea);

	// Top bar: phase + round, timer banner, budget.
	UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>();
	Screen->AddChildToVerticalBox(TopBar)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, Gap));

	UBorder* PhasePanel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	UVerticalBox* PhaseBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PhasePanel->SetContent(PhaseBox);
	PhaseText = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::H2);
	RoundText = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Small);
	PhaseBox->AddChildToVerticalBox(PhaseText);
	PhaseBox->AddChildToVerticalBox(RoundText);
	UHorizontalBoxSlot* PhaseSlot = TopBar->AddChildToHorizontalBox(PhasePanel);
	PhaseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PhaseSlot->SetHorizontalAlignment(HAlign_Left);
	PhaseSlot->SetVerticalAlignment(VAlign_Top);

	UBorder* Banner = UTTAUIStyle::MakeBanner(WidgetTree, FText::GetEmpty());
	TimerText = CastChecked<UTextBlock>(Banner->GetContent());
	TimerText->SetFont(UTTAUIStyle::MakeFont(UTTAUIStyle::Get().LoadFont(true), true, UTTAUIStyle::kTimer));
	TopBar->AddChildToHorizontalBox(Banner)->SetVerticalAlignment(VAlign_Top);

	BudgetPanel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	UVerticalBox* BudgetBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BudgetPanel->SetContent(BudgetBox);
	BudgetText = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::H2);
	BudgetBox->AddChildToVerticalBox(BudgetText);
	BudgetBox->AddChildToVerticalBox(UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(TEXT("BUDGET")), ETTAFont::Small));
	UHorizontalBoxSlot* BudgetSlot = TopBar->AddChildToHorizontalBox(BudgetPanel);
	BudgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BudgetSlot->SetHorizontalAlignment(HAlign_Right);
	BudgetSlot->SetVerticalAlignment(VAlign_Top);

	// Under the top bar, left column.
	ReadyRow = UTTAUIStyle::MakeIconCount(WidgetTree, ETTAAsset::IconReady, FText::GetEmpty());
	ReadyText = IconLabel(ReadyRow);
	Screen->AddChildToVerticalBox(ReadyRow)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, Gap));
	FilmHint = UTTAUIStyle::MakeIconCount(WidgetTree, ETTAAsset::IconMicOn, FText::FromString(TEXT("[RMB] hold to film")));
	IconLabel(FilmHint)->SetFont(UTTAUIStyle::Font(ETTAFont::Small));
	Screen->AddChildToVerticalBox(FilmHint)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, Gap));

	BuildPanel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	Screen->AddChildToVerticalBox(BuildPanel)->SetHorizontalAlignment(HAlign_Left);
	UVerticalBox* Build = WidgetTree->ConstructWidget<UVerticalBox>();
	BuildPanel->SetContent(Build);
	UHorizontalBox* Cameras = WidgetTree->ConstructWidget<UHorizontalBox>();
	Build->AddChildToVerticalBox(Cameras)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, UTTAUIStyle::Space(2)));
	CameraTotal = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Body);
	Cameras->AddChildToHorizontalBox(CameraTotal)->SetVerticalAlignment(VAlign_Center);
	for (const ETTAAsset Icon : {ETTAAsset::IconCamFixed, ETTAAsset::IconCamPan, ETTAAsset::IconCamFollow, ETTAAsset::IconCamTrap})
	{
		UHorizontalBox* Count = UTTAUIStyle::MakeIconCount(WidgetTree, Icon, FText::GetEmpty());
		CameraCounts.Add(IconLabel(Count));
		UHorizontalBoxSlot* CountSlot = Cameras->AddChildToHorizontalBox(Count);
		CountSlot->SetPadding(FMargin(Gap, 0.f, 0.f, 0.f));
		CountSlot->SetVerticalAlignment(VAlign_Center);
	}
	for (TObjectPtr<UTextBlock>* Line : {&ItemText, &GhostText, &RejectedText})
	{
		*Line = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Small);
		Build->AddChildToVerticalBox(*Line);
	}
	SelectedBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Build->AddChildToVerticalBox(SelectedBox);
	for (int32 i = 0; i < 3; ++i)
	{
		SelectedLines.Add(UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Small, ETTAColor::Yellow));
		SelectedBox->AddChildToVerticalBox(SelectedLines.Last());
	}
}

void ATTABuildHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!DisplayFont) DisplayFont = UTTAUIStyle::Get().LoadCanvasFont(true);
	if (!BodyFont) BodyFont = UTTAUIStyle::Get().LoadCanvasFont(false);

	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	if (!State) return;
	const ETTAPhase Phase = State->GetPhase();
	if (Phase == ETTAPhase::Podium)
	{
		if (Overlay) SetShown(Overlay, false);
		ShowResults();
		return;
	}
	if (Phase == ETTAPhase::Premiere) DrawScoreBoard();
	UpdateOverlay(*State);
}

void ATTABuildHUD::UpdateOverlay(const ATTAGameState& State)
{
	if (!Overlay)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (!PC) return;
		Overlay = CreateWidget<UTTABuildHUDWidget>(PC, UTTABuildHUDWidget::StaticClass());
		Overlay->AddToViewport();
	}
	SetShown(Overlay, true);
	const ETTAPhase Phase = State.GetPhase();

	Overlay->PhaseText->SetText(FText::FromString(UEnum::GetDisplayValueAsText(Phase).ToString().ToUpper()));
	Overlay->RoundText->SetText(FText::FromString(FString::Printf(TEXT("ROUND %d"), State.GetRound())));
	const int32 SecondsLeft = State.GetSecondsLeft();
	SetLine(Overlay->TimerText, FString::Printf(TEXT("%d:%02d"), SecondsLeft / 60, SecondsLeft % 60),
		SecondsLeft <= kTimerWarnSeconds ? ETTAColor::Red : ETTAColor::Ink);

	const bool bReady = Phase == ETTAPhase::Lobby && State.IsGateOpen();
	SetShown(Overlay->ReadyRow, bReady);
	if (bReady) Overlay->ReadyText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d READY"), State.GetReadyCount(), State.GetReadyMax())));
	SetShown(Overlay->FilmHint, Phase == ETTAPhase::Run);

	const ATTABuildPawn* Pawn = Cast<ATTABuildPawn>(GetOwningPawn());
	SetShown(Overlay->BudgetPanel, Pawn != nullptr);
	SetShown(Overlay->BuildPanel, Pawn != nullptr);
	if (!Pawn) return;
	const int32 Me = ATTAPlacementMarker::PlayerIdOf(Pawn);
	const tta::Layout Own = Pawn->OwnLayout();

	SetLine(Overlay->BudgetText, FString::Printf(TEXT("$%d / %d"), Own.remaining(Me), tta::kBudget),
		Own.remaining(Me) > 0 ? ETTAColor::Grass : ETTAColor::Red);

	Overlay->CameraTotal->SetText(FText::FromString(FString::Printf(TEXT("CAMERAS %d / %d"), Own.cameraCount(Me), tta::kCameraCap)));
	const tta::CameraType Types[] = {tta::CameraType::Fixed, tta::CameraType::Pan, tta::CameraType::Follow, tta::CameraType::TrapCam};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Types); ++i)
	{
		Overlay->CameraCounts[i]->SetText(FText::AsNumber(Own.cameraCount(Me, Types[i])));
	}
	Overlay->ItemText->SetText(FText::FromString(FString::Printf(TEXT("%s   [wheel] item  [R] rotate  [LMB] place  [RMB] remove  [F] select"),
		*TTABuildItem::Label(Pawn->GetSelectedItem()))));
	const bool bGhostOk = Pawn->GetGhostOutcome() == ETTAPlaceOutcome::Ok;
	SetLine(Overlay->GhostText, bGhostOk ? FString(TEXT("Can place here")) : UEnum::GetDisplayValueAsText(Pawn->GetGhostOutcome()).ToString(),
		bGhostOk ? ETTAColor::Grass : ETTAColor::Red);
	const bool bRejected = Pawn->GetLastServerOutcome() != ETTAPlaceOutcome::Ok;
	SetShown(Overlay->RejectedText, bRejected);
	if (bRejected)
	{
		SetLine(Overlay->RejectedText, FString::Printf(TEXT("Rejected: %s"), *UEnum::GetDisplayValueAsText(Pawn->GetLastServerOutcome()).ToString()), ETTAColor::Red);
	}

	bool bTrapSelected = false;
	if (const ATTAPlacementMarker* Selected = Pawn->FindMarker(Pawn->GetSelectedPlacement()))
	{
		const tta::Placement P = Selected->GetData().ToCore();
		if (!P.isCamera)
		{
			bTrapSelected = true;
			const tta::TrapSpec& Spec = tta::trapSpec(P.trap);
			const TCHAR* Keys[] = {TEXT("[1]-  [2]+"), TEXT("[3]-  [4]+")};
			Overlay->SelectedLines[0]->SetText(FText::FromString(FString::Printf(TEXT("SELECTED %s #%d"), UTF8_TO_TCHAR(Spec.name), P.id)));
			for (int32 i = 0; i < 2; ++i)
			{
				const tta::SettingSpec& Setting = Spec.settings[static_cast<size_t>(i)];
				Overlay->SelectedLines[i + 1]->SetText(FText::FromString(FString::Printf(TEXT("%s  %s: %d  (%d-%d)"), Keys[i], UTF8_TO_TCHAR(Setting.name),
					P.settings[static_cast<size_t>(i)], Setting.min, Setting.max)));
			}
		}
	}
	SetShown(Overlay->SelectedBox, bTrapSelected);
}

void ATTABuildHUD::DrawScoreBoard()
{
	const ATTACinema* Cinema = nullptr;
	for (TActorIterator<ATTACinema> It(GetWorld()); It; ++It)
	{
		Cinema = *It;
		break;
	}
	if (!Cinema || Cinema->GetScoreBoard().empty()) return;
	const std::vector<tta::ScoreRow>& Rows = Cinema->GetScoreBoard();
	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	constexpr float BoardW = 900.f, RowH = 90.f, LabelW = 220.f;
	const float BoardX = (kDesignW - BoardW) * 0.5f;
	const float BoardY = 260.f;
	double MaxTotal = 1.0;
	for (const tta::ScoreRow& Row : Rows) MaxTotal = FMath::Max(MaxTotal, FMath::Max(Row.from, Row.to));
	DrawCard(BoardX - kPad, BoardY - kPad, BoardW + 2 * kPad, Rows.size() * RowH + kPad, tta::kInk);
	for (const tta::ScoreRow& Row : Rows)
	{
		const tta::ScoreBar Bar = tta::scoreBarAt(Row, Cinema->GetScoreBoardAlpha());
		const float Y = BoardY + static_cast<float>(Bar.slot) * RowH;
		int32 Fill = tta::kBone;
		for (const APlayerState* PS : State ? State->PlayerArray : TArray<TObjectPtr<APlayerState>>())
		{
			const ATTAPlayerState* TTA = Cast<ATTAPlayerState>(PS);
			if (TTA && TTA->GetPlayerId() == Row.player) Fill = tta::kPlayerColours[TTA->GetLook().colour];
		}
		DrawStyledText(FString::Printf(TEXT("P%d"), Row.player), tta::kBone, BoardX, Y + kPad * 0.5f, UTTAUIStyle::kH2, true);
		const float BarW = FMath::Max(8.f, static_cast<float>(Bar.value / MaxTotal) * (BoardW - LabelW - 120.f));
		DrawCard(BoardX + LabelW, Y + 10.f, BarW, RowH - 20.f, Fill);
		DrawStyledText(FString::Printf(TEXT("%.1f"), Bar.value), tta::kBone, BoardX + LabelW + BarW + kPad, Y + kPad * 0.5f, UTTAUIStyle::kH2, true);
	}
}
