#include "TTABuildHUD.h"
#include "TTAGameMode.h"

#include "Engine/Canvas.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TTAPlacementMarker.h"
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
constexpr float kDesignW = 1920.f, kBarH = 120.f, kLineH = 40.f, kPad = 20.f, kPanelOpacity = 0.85f;
// Build tiles: one equal square per TTABuildItem, in a row under the phase card.
constexpr float kTileSize = 96.f, kTileGap = 12.f, kTileIconInset = 16.f;

// Icon art for a camera build item; traps use TrapThumbnail.
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

void ATTABuildHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!DisplayFont) DisplayFont = UTTAUIStyle::Get().LoadCanvasFont(true);
	if (!BodyFont) BodyFont = UTTAUIStyle::Get().LoadCanvasFont(false);
	const FMargin& Safe = UTTAUIStyle::Get().SafeArea;
	const float Left = Safe.Left, Right = kDesignW - Safe.Right, Top = Safe.Top;

	const ATTAGameState* State = GetWorld()->GetGameState<ATTAGameState>();
	if (!State) return;
	const ATTAGameMode* Mode = GetWorld()->GetAuthGameMode<ATTAGameMode>();
	const ETTAPhase Phase = State->GetPhase();
	if (Phase == ETTAPhase::Podium)
	{
		ShowResults();
		return;
	}
	if (Phase == ETTAPhase::Premiere) DrawScoreBoard();

	// Phase + round, top left.
	const FString PhaseName = (Mode && Mode->IsWatching()) ? FString(TEXT("HEAVEN")) : UEnum::GetDisplayValueAsText(Phase).ToString().ToUpper();
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
		const FString Ready = FString::Printf(TEXT("%d/%d READY"), State->GetReadyCount(), State->GetReadyMax());
		const float ReadyY = Top + kBarH + kPad;
		DrawCard(Left, ReadyY, MeasureText(Ready, UTTAUIStyle::kH2, true) + 2 * kPad, kBarH, tta::kGrass);
		DrawStyledText(Ready, tta::kInk, Left + kPad, ReadyY + kPad * 0.5f, UTTAUIStyle::kH2, true);
	}
	if (Phase == ETTAPhase::Run && !(Mode && Mode->IsDemo()))
	{
		DrawStyledText(TEXT("[RMB] hold to film"), tta::kBone, Left, Y, UTTAUIStyle::kSmall, false);
	}
	if (Phase == ETTAPhase::Build && Mode && Mode->IsDemo())
	{
		DrawStyledText(TEXT("Fly the course and place your traps. The race starts when the timer ends."),
			tta::kBone, Left, Y, UTTAUIStyle::kSmall, false);
		Y += kLineH;
	}
	if (Mode && Mode->IsWatching())
	{
		DrawStyledText(TEXT("Watch the TV. When this race ends you place traps again, unless the match is over."),
			tta::kBone, Left, Y, UTTAUIStyle::kSmall, false);
	}

	const ATTABuildPawn* Pawn = Cast<ATTABuildPawn>(GetOwningPawn());
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
		Lines.Emplace(FString::Printf(TEXT("Rejected: %s"), *UEnum::GetDisplayValueAsText(Pawn->GetLastServerOutcome()).ToString()), tta::kRed);
	}
	if (const ATTAPlacementMarker* Selected = Pawn->FindMarker(Pawn->GetSelectedPlacement()))
	{
		const tta::Placement P = Selected->GetData().ToCore();
		if (!P.isCamera)
		{
			const tta::TrapSpec& Spec = tta::trapSpec(P.trap);
			const TCHAR* Keys[] = {TEXT("[1]-  [2]+"), TEXT("[3]-  [4]+")};
			Lines.Emplace(FString::Printf(TEXT("SELECTED %s #%d"), UTF8_TO_TCHAR(Spec.name), P.id), tta::kYellow);
			for (int32 i = 0; i < 2; ++i)
			{
				const tta::SettingSpec& Setting = Spec.settings[static_cast<size_t>(i)];
				Lines.Emplace(FString::Printf(TEXT("%s  %s: %d  (%d-%d)"), Keys[i], UTF8_TO_TCHAR(Setting.name),
					P.settings[static_cast<size_t>(i)], Setting.min, Setting.max), tta::kYellow);
			}
		}
	}

	float PanelW = 0.f;
	for (const TPair<FString, int32>& Line : Lines) PanelW = FMath::Max(PanelW, MeasureText(Line.Key, UTTAUIStyle::kSmall, false));
	DrawCard(Left, Y, PanelW + 2 * kPad, Lines.Num() * kLineH + 2 * kPad - (kLineH - UTTAUIStyle::kSmall), tta::kInk);
	Y += kPad;
	for (const TPair<FString, int32>& Line : Lines)
	{
		DrawStyledText(Line.Key, Line.Value, Left + kPad, Y, UTTAUIStyle::kSmall, false);
		Y += kLineH;
	}
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

float ATTABuildHUD::DrawBuildTiles(float X, float Y, int32 Selected)
{
	const float S = UiScale();
	for (int32 Item = 0; Item < TTABuildItem::Count(); ++Item)
	{
		const float TileX = X + Item * (kTileSize + kTileGap);
		DrawCard(TileX, Y, kTileSize, kTileSize, Item == Selected ? tta::kYellow : tta::kBone);
		UTexture* Icon = TTABuildItem::IsCamera(Item)
			? Cast<UTexture>(UTTAUIStyle::Brush(CameraIcon(TTABuildItem::Camera(Item))).GetResourceObject())
			: TrapThumbnail(Item);
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

UTexture* ATTABuildHUD::TrapThumbnail(int32 Item)
{
	if (const TObjectPtr<UTextureRenderTarget2D>* Cached = TrapThumbnails.Find(Item)) return *Cached;
	UStaticMesh* Mesh = GetDefault<ATTAPlacementMarker>()->MeshForItem(Item);
	UWorld* World = GetWorld();
	if (!Mesh || !World) return nullptr;
	// Far below the level so nothing else is in frame; ShowOnly keeps it to this one mesh.
	const FVector At(0.f, 0.f, -100000.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Prop = World->SpawnActor<AStaticMeshActor>(At, FRotator(0.f, 35.f, 0.f), Params);
	Prop->SetMobility(EComponentMobility::Movable);
	Prop->GetStaticMeshComponent()->SetStaticMesh(Mesh);
	const FBoxSphereBounds B = Mesh->GetBounds();
	const FVector Center = At + B.Origin;
	const FVector Eye = Center + FVector(-1.f, -0.6f, 0.7f).GetSafeNormal() * B.SphereRadius * 2.4f;
	ASceneCapture2D* Cam = World->SpawnActor<ASceneCapture2D>(Eye, (Center - Eye).Rotation(), Params);
	UTextureRenderTarget2D* Target = UKismetRenderingLibrary::CreateRenderTarget2D(this, 256, 256, RTF_RGBA8);
	Target->ClearColor = FLinearColor::Transparent;
	USceneCaptureComponent2D* Capture = Cam->GetCaptureComponent2D();
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->ShowOnlyActors.Add(Prop);
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->bCaptureEveryFrame = false;
	Capture->FOVAngle = 40.f;
	Capture->TextureTarget = Target;
	Capture->CaptureScene();
	Prop->Destroy();
	Cam->Destroy();
	TrapThumbnails.Add(Item, Target);
	return Target;
}
