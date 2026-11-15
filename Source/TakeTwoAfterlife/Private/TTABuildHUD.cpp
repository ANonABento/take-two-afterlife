#include "TTABuildHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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
constexpr float kDesignW = 1920.f, kBarH = 120.f, kLineH = 40.f, kPad = 20.f, kPanelOpacity = 0.85f;
// Build tiles: one equal square per TTABuildItem, in a row under the phase card.
constexpr float kTileSize = 96.f, kTileGap = 12.f, kTileIconInset = 16.f;

// Icon art for a camera build item; traps have none and show their number.
ETTAAsset CameraIcon(tta::CameraType Type)
{
	switch (Type)
	{
	case tta::CameraType::Fixed: return ETTAAsset::IconCamFixed;
	case tta::CameraType::Pan: return ETTAAsset::IconCamPan;
	case tta::CameraType::Follow: return ETTAAsset::IconCamFollow;
	case tta::CameraType::TrapCam: return ETTAAsset::IconCamTrap;
	}
	checkNoEntry();
	return ETTAAsset::IconCamFixed;
}
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

	PremiereBox = WidgetTree->ConstructWidget<UVerticalBox>();
	UCanvasPanelSlot* PremiereSlot = Root->AddChildToCanvas(PremiereBox);
	PremiereSlot->SetAnchors(FAnchors(0.5f, 0.f));
	PremiereSlot->SetAlignment(FVector2D(0.5f, 0.f));
	PremiereSlot->SetAutoSize(true);
	PremiereSlot->SetPosition(FVector2D(0.f, UTTAUIStyle::Get().SafeArea.Top + kPremiereTop));
	NowShowing = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
	NowShowing->SetContent(Card);
	Card->AddChildToVerticalBox(UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(TEXT("NOW SHOWING")), ETTAFont::Small, ETTAColor::Pink))->SetHorizontalAlignment(HAlign_Center);
	IntroText = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::H2);
	Card->AddChildToVerticalBox(IntroText)->SetHorizontalAlignment(HAlign_Center);
	CutText = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Body);
	Card->AddChildToVerticalBox(CutText)->SetHorizontalAlignment(HAlign_Center);
	UVerticalBoxSlot* CardSlot = PremiereBox->AddChildToVerticalBox(NowShowing);
	CardSlot->SetHorizontalAlignment(HAlign_Center);
	CardSlot->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, Gap));
	ScorePanel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	ScoreList = WidgetTree->ConstructWidget<UVerticalBox>();
	ScorePanel->SetContent(ScoreList);
	PremiereBox->AddChildToVerticalBox(ScorePanel)->SetHorizontalAlignment(HAlign_Center);
}

void UTTABuildHUDWidget::EnsureScoreRows(int32 Count)
{
	while (ScoreBars.Num() < Count)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		ScoreList->AddChildToVerticalBox(Row)->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, UTTAUIStyle::Space(2)));
		USizeBox* Name = WidgetTree->ConstructWidget<USizeBox>();
		Name->SetWidthOverride(kScoreNameW);
		ScoreNames.Add(UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::H2));
		Name->AddChild(ScoreNames.Last());
		Row->AddChildToHorizontalBox(Name)->SetVerticalAlignment(VAlign_Center);
		ScoreBars.Add(WidgetTree->ConstructWidget<USizeBox>());
		ScoreBars.Last()->SetHeightOverride(kScoreBarH);
		ScoreFills.Add(UTTAUIStyle::AddCard(WidgetTree, ScoreBars.Last(), tta::kBone));
		Row->AddChildToHorizontalBox(ScoreBars.Last())->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, UTTAUIStyle::Space(3)));
		ScoreValues.Add(UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::H2));
		Row->AddChildToHorizontalBox(ScoreValues.Last())->SetVerticalAlignment(VAlign_Center);
	}
}

void ATTABuildHUD::DrawHUD()
{
	Super::DrawHUD();
	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	if (!State) return;
	const ETTAPhase Phase = State->GetPhase();
	if (Phase == ETTAPhase::Podium)
	{
		if (Overlay) SetShown(Overlay, false);
		ShowResults();
		return;
	}
	UpdateOverlay(*State);
	if (Overlay) UpdatePremiere(Phase == ETTAPhase::Premiere);
}

	// Phase + round, top left.
	const FString PhaseName = UEnum::GetDisplayValueAsText(Phase).ToString().ToUpper();
	const FString RoundText = FString::Printf(TEXT("ROUND %d"), State->GetRound());
	const float PhaseW = FMath::Max(MeasureText(PhaseName, UTTAUIStyle::kH2, true), MeasureText(RoundText, UTTAUIStyle::kSmall, false));
	DrawCard(Left, Top, PhaseW + 2 * kPad, kBarH, tta::kPurple);
	DrawStyledText(PhaseName, tta::kBone, Left + kPad, Top + kPad * 0.5f, UTTAUIStyle::kH2, true);
	DrawStyledText(RoundText, tta::kBone, Left + kPad, Top + kBarH - kPad - UTTAUIStyle::kSmall, UTTAUIStyle::kSmall, false);

	// Timer, top centre.
	const int32 SecondsLeft = State->GetSecondsLeft();
	const FString Timer = FString::Printf(TEXT("%d:%02d"), SecondsLeft / 60, SecondsLeft % 60);
	const float TimerW = MeasureText(Timer, UTTAUIStyle::kH1, true) + 2 * kPad;
	const float TimerX = (kDesignW - TimerW) * 0.5f;
	DrawCard(TimerX, Top, TimerW, kBarH, tta::hudTimerFill(SecondsLeft));
	DrawStyledText(Timer, tta::kBone, TimerX + kPad, Top, UTTAUIStyle::kH1, true);

	float Y = Top + kBarH + kPad;
	if (Phase == ETTAPhase::Lobby && State->IsGateOpen())
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

	// Budget, top right.
	const FString Budget = FString::Printf(TEXT("$%d / %d"), Own.remaining(Me), tta::kBudget);
	const float BudgetW = MeasureText(Budget, UTTAUIStyle::kH2, true) + 2 * kPad;
	DrawCard(Right - BudgetW, Top, BudgetW, kBarH, tta::hudBudgetFill(Own.remaining(Me)));
	DrawStyledText(TEXT("BUDGET"), tta::kBone, Right - BudgetW + kPad, Top + kBarH - kPad - UTTAUIStyle::kSmall, UTTAUIStyle::kSmall, false);
	DrawStyledText(Budget, tta::kBone, Right - BudgetW + kPad, Top + kPad * 0.5f, UTTAUIStyle::kH2, true);

	Y = DrawBuildTiles(Left, Y, Pawn->GetSelectedItem()) + kPad;

	TArray<TPair<FString, int32>> Lines;
	Lines.Emplace(FString::Printf(TEXT("CAMERAS %d / %d   fixed %d  pan %d  follow %d  trap-cam %d"), Own.cameraCount(Me), tta::kCameraCap,
		Own.cameraCount(Me, tta::CameraType::Fixed), Own.cameraCount(Me, tta::CameraType::Pan),
		Own.cameraCount(Me, tta::CameraType::Follow), Own.cameraCount(Me, tta::CameraType::TrapCam)), tta::kBone);
	const bool bGhostOk = Pawn->GetGhostOutcome() == ETTAPlaceOutcome::Ok;
	Lines.Emplace(FString::Printf(TEXT("%s   [wheel] item  [R] rotate  [LMB] place  [RMB] remove  [F] select"),
		*TTABuildItem::Label(Pawn->GetSelectedItem())), tta::kBone);
	Lines.Emplace(bGhostOk ? FString(TEXT("Can place here")) : UEnum::GetDisplayValueAsText(Pawn->GetGhostOutcome()).ToString(),
		bGhostOk ? tta::kGrass : tta::kRed);
	if (Pawn->GetLastServerOutcome() != ETTAPlaceOutcome::Ok)
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

void ATTABuildHUD::UpdatePremiere(bool bPremiere)
{
	const ATTACinema* Cinema = nullptr;
	if (bPremiere)
	{
		for (TActorIterator<ATTACinema> It(GetWorld()); It; ++It)
		{
			Cinema = *It;
			break;
		}
	}
	const tta::Premiere* Premiere = Cinema ? Cinema->GetPremiere() : nullptr;
	const bool bCut = Premiere && Premiere->index() < Premiere->order().size();
	SetShown(Overlay->PremiereBox, bCut);
	if (!bCut) return;
	Overlay->IntroText->SetText(Cinema->GetIntroLine());
	Overlay->CutText->SetText(FText::Format(NSLOCTEXT("TTA", "PremiereCutOf", "CUT {0} OF {1}"),
		static_cast<int32>(Premiere->index()) + 1, static_cast<int32>(Premiere->order().size())));

	const std::vector<tta::ScoreRow>& Rows = Cinema->GetScoreBoard();
	SetShown(Overlay->ScorePanel, !Rows.empty());
	if (Rows.empty()) return;
	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	Overlay->EnsureScoreRows(static_cast<int32>(Rows.size()));
	for (int32 i = 0; i < Overlay->ScoreBars.Num(); ++i) SetShown(Overlay->ScoreBars[i]->GetParent(), i < static_cast<int32>(Rows.size()));
	double MaxTotal = 1.0;
	for (const tta::ScoreRow& Row : Rows) MaxTotal = FMath::Max(MaxTotal, FMath::Max(Row.from, Row.to));
	for (const tta::ScoreRow& Row : Rows)
	{
		const tta::ScoreBar Bar = tta::scoreBarAt(Row, Cinema->GetScoreBoardAlpha());
		const int32 Slot = static_cast<int32>(Bar.slot);
		int32 Fill = tta::kBone;
		for (const APlayerState* PS : State ? State->PlayerArray : TArray<TObjectPtr<APlayerState>>())
		{
			const ATTAPlayerState* TTA = Cast<ATTAPlayerState>(PS);
			if (TTA && TTA->GetPlayerId() == Row.player) Fill = tta::kPlayerColours[TTA->GetLook().colour];
		}
		Overlay->ScoreNames[Slot]->SetText(FText::FromString(FString::Printf(TEXT("P%d"), Row.player)));
		Overlay->ScoreBars[Slot]->SetWidthOverride(FMath::Max(UTTAUIStyle::Space(2), static_cast<float>(Bar.value / MaxTotal) * kScoreBarMaxW));
		Overlay->ScoreFills[Slot]->SetBrushColor(UTTAUIStyle::Colour(Fill));
		Overlay->ScoreValues[Slot]->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Bar.value)));
	}
}

float ATTABuildHUD::DrawBuildTiles(float X, float Y, int32 Selected)
{
	const float S = UiScale();
	for (int32 Item = 0; Item < TTABuildItem::Count(); ++Item)
	{
		const float TileX = X + Item * (kTileSize + kTileGap);
		DrawCard(TileX, Y, kTileSize, kTileSize, Item == Selected ? tta::kYellow : tta::kBone);
		UTexture2D* Icon = TTABuildItem::IsCamera(Item)
			? Cast<UTexture2D>(UTTAUIStyle::Brush(CameraIcon(TTABuildItem::Camera(Item))).GetResourceObject())
			: nullptr;
		if (Icon)
		{
			const float Side = kTileSize - 2 * kTileIconInset;
			DrawTexture(Icon, (TileX + kTileIconInset) * S, (Y + kTileIconInset) * S, Side * S, Side * S, 0.f, 0.f, 1.f, 1.f);
			continue;
		}
		const FString Number = FString::FromInt(Item + 1);
		DrawStyledText(Number, tta::kInk, TileX + (kTileSize - MeasureText(Number, UTTAUIStyle::kH2, true)) * 0.5f,
			Y + (kTileSize - UTTAUIStyle::kH2) * 0.5f, UTTAUIStyle::kH2, true);
	}
	return Y + kTileSize;
}
