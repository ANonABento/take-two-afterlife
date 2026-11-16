#include "TTABuildHUD.h"
#include "TTAGameMode.h"

#include "Engine/Canvas.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
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
constexpr float kTileSize = 96.f, kTileGap = 12.f;
// Stay inside the card stroke so the picture fills the tile without covering the border.
constexpr float kTileIconInset = UTTAUIStyle::kOutline + 2.f;
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
	if (Mode && Mode->IsHeavenChoice())
	{
		DrawStyledText(TEXT("Walk through the gate for the next round.  [R] restarts at round 1."),
			tta::kBone, Left, Y, UTTAUIStyle::kSmall, false);
	}
	else if (Mode && Mode->IsWatching())
	{
		DrawStyledText(TEXT("You're in heaven. The TV stays on the race, then plays the death reel."),
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
	Lines.Emplace(FString::Printf(TEXT("%s   [1-7] item  [R] rotate  [LMB] place  [RMB] remove  [F] select"),
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
		const FString Number = FString::FromInt(Item + 1);
		if (UTexture* Icon = TrapThumbnail(Item))
		{
			const float Side = kTileSize - 2 * kTileIconInset;
			DrawTexture(Icon, (TileX + kTileIconInset) * S, (Y + kTileIconInset) * S, Side * S, Side * S,
				0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Opaque);
		}
		else
		{
			DrawStyledText(Number, tta::kInk, TileX + (kTileSize - MeasureText(Number, UTTAUIStyle::kH2, true)) * 0.5f,
				Y + (kTileSize - UTTAUIStyle::kH2) * 0.5f, UTTAUIStyle::kH2, true);
		}
		DrawStyledText(Number, tta::kInk, TileX + kTileIconInset, Y + kTileSize - kTileIconInset - UTTAUIStyle::kSmall,
			UTTAUIStyle::kSmall, true);
	}
	return Y + kTileSize;
}

UTexture* ATTABuildHUD::TrapThumbnail(int32 Item)
{
	if (const TObjectPtr<UTextureRenderTarget2D>* Cached = TrapThumbnails.Find(Item)) return *Cached;
	UStaticMesh* Mesh = GetDefault<ATTAPlacementMarker>()->MeshForItem(Item);
	UWorld* World = GetWorld();
	if (!Mesh || !World) return nullptr;
	// Just under the course. ShowOnly hides the level; staying near the origin keeps the
	// depth buffer precise so the outline pass cannot ink the whole mesh one flat colour.
	const FVector At(0.f, 0.f, -3000.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Prop = World->SpawnActor<AStaticMeshActor>(At, FRotator(-22.f, 38.f, 0.f), Params);
	Prop->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* PropMesh = Prop->GetStaticMeshComponent();
	PropMesh->SetStaticMesh(Mesh);
	PropMesh->UpdateBounds();
	// Orthographic fit to the rotated bounds so a hammer and a banana fill the same square.
	const FBox Box = PropMesh->Bounds.GetBox();
	const FVector Center = Box.GetCenter();
	const float Span = Box.GetExtent().GetMax() * 2.f * 1.08f;
	const FVector Eye = Center + FVector(-1.f, -0.65f, 0.55f).GetSafeNormal() * Span;
	// ShowOnly drops every light that is not in the list, which left these lit meshes a flat gray.
	ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(Center, FRotator(-50.f, -35.f, 0.f), Params);
	ADirectionalLight* Fill = World->SpawnActor<ADirectionalLight>(Center, FRotator(-15.f, 145.f, 0.f), Params);
	for (ADirectionalLight* Light : {Key, Fill})
	{
		UDirectionalLightComponent* Comp = Light->GetComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetCastShadows(false);
		Comp->SetIntensity(Light == Key ? 12.f : 5.f);
		Comp->SetLightColor(FLinearColor::White);
	}
	ASceneCapture2D* Cam = World->SpawnActor<ASceneCapture2D>(Eye, (Center - Eye).Rotation(), Params);
	UTextureRenderTarget2D* Target = UKismetRenderingLibrary::CreateRenderTarget2D(this, 256, 256, RTF_RGBA8);
	Target->ClearColor = UTTAUIStyle::Colour(tta::kBone);
	USceneCaptureComponent2D* Capture = Cam->GetCaptureComponent2D();
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->ShowOnlyActors.Add(Prop);
	Capture->ShowOnlyActors.Add(Key);
	Capture->ShowOnlyActors.Add(Fill);
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = false;
	Capture->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture->OrthoWidth = Span;
	Capture->bUpdateOrthoPlanes = true;
	Capture->ShowFlags.SetPostProcessMaterial(false);
	Capture->ShowFlags.SetEyeAdaptation(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->PostProcessBlendWeight = 1.f;
	Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 0.6f;
	Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Capture->TextureTarget = Target;
	Capture->CaptureScene();
	Prop->Destroy();
	Key->Destroy();
	Fill->Destroy();
	Cam->Destroy();
	TrapThumbnails.Add(Item, Target);
	return Target;
}
