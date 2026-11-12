#include "TTAEditorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TTACutPlayer.h"
#include "TTAEditStage.h"
#include "TTAGameState.h"
#include "TTAReels.h"
#include "TTATimelineWidget.h"

#include <stdexcept>

DEFINE_LOG_CATEGORY_STATIC(LogTTAEditor, Log, All);

namespace
{
constexpr double kSlowSteps[] = {1.0, 0.5, 0.25};
constexpr double kZoomSteps[] = {1.0, 1.5, 2.0, 3.0};

template <size_t N> double NextStep(const double (&Steps)[N], double Current)
{
	for (size_t i = 0; i < N; ++i)
	{
		if (FMath::IsNearlyEqual(Steps[i], Current)) return Steps[(i + 1) % N];
	}
	return Steps[0];
}
}

void UTTAEditorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Player = NewObject<UTTACutPlayer>(this);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = Root;

	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
	TimerText = WidgetTree->ConstructWidget<UTextBlock>();
	Top->AddChildToHorizontalBox(TimerText);
	AddButton(Top, TEXT("AUTO-EDIT"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnAutoEdit);
	AddButton(Top, TEXT("Play"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnPlay);
	StatusText = WidgetTree->ConstructWidget<UTextBlock>();
	Top->AddChildToHorizontalBox(StatusText);
	Root->AddChildToVerticalBox(Top);

	USizeBox* PreviewBox = WidgetTree->ConstructWidget<USizeBox>();
	PreviewBox->SetWidthOverride(960.f);
	PreviewBox->SetHeightOverride(540.f);
	Preview = WidgetTree->ConstructWidget<UImage>();
	PreviewBox->AddChild(Preview);
	Root->AddChildToVerticalBox(PreviewBox);

	Thumbs = WidgetTree->ConstructWidget<UHorizontalBox>();
	Root->AddChildToVerticalBox(Thumbs);

	Timeline = WidgetTree->ConstructWidget<UTTATimelineWidget>(UTTATimelineWidget::StaticClass());
	USizeBox* TimelineBox = WidgetTree->ConstructWidget<USizeBox>();
	TimelineBox->SetHeightOverride(Timeline->StripHeight * 2.f + 16.f);
	TimelineBox->AddChild(Timeline);
	Root->AddChildToVerticalBox(TimelineBox);
	Timeline->OnReplayClicked.BindLambda([this](uint32 ReplayMs) { AddClipAt(ReplayMs); });
	Timeline->OnCutClicked.BindLambda([this](int32 Clip, uint32 CutMs)
	{
		Selected = Clip;
		Player->Stop();
		Player->Seek(CutMs);
	});

	UHorizontalBox* Ops = WidgetTree->ConstructWidget<UHorizontalBox>();
	AddButton(Ops, TEXT("Trim in"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnTrimIn);
	AddButton(Ops, TEXT("Trim out"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnTrimOut);
	AddButton(Ops, TEXT("Split"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnSplit);
	AddButton(Ops, TEXT("<"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnLeft);
	AddButton(Ops, TEXT(">"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnRight);
	AddButton(Ops, TEXT("Delete"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnDelete);
	AddButton(Ops, TEXT("Slow-mo"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnSlowMo);
	AddButton(Ops, TEXT("Zoom"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnZoom);
	AddButton(Ops, TEXT("Sticker"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnSticker);
	CaptionBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	CaptionBox->SetHintText(FText::FromString(TEXT("caption")));
	Ops->AddChildToHorizontalBox(CaptionBox);
	AddButton(Ops, TEXT("Caption"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnCaption);
	AddButton(Ops, TEXT("WASTED"))->OnClicked.AddDynamic(this, &UTTAEditorWidget::OnWasted);
	Root->AddChildToVerticalBox(Ops);
}

UButton* UTTAEditorWidget::AddButton(UHorizontalBox* Row, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Button->AddChild(Text);
	Row->AddChildToHorizontalBox(Button);
	return Button;
}

void UTTAEditorWidget::Init(ATTAEditStage* InStage, int32 InHeavenCam, const FString& InReelPath)
{
	check(InStage && InStage->NumCameras() > InHeavenCam);
	Stage = InStage;
	HeavenCam = InHeavenCam;
	ReelPath = InReelPath;
	const tta::Replay& Replay = Stage->GetReplay();

	TArray<TPair<uint32, uint8>> Markers;
	for (const tta::ReplayEvent& E : Replay.events()) Markers.Emplace(E.timeMs, static_cast<uint8>(E.type));
	Timeline->SetData(&Cut, static_cast<uint32>(Replay.durationSeconds() * 1000.0), MoveTemp(Markers));

	// Thumbnails show the first hit, or the start when nobody got hit.
	Stage->CaptureThumbnails(Replay.events().empty() ? 0.0 : Replay.events().front().timeMs / 1000.0);
	Preview->SetBrushResourceObject(Stage->GetPreview());
	Thumbs->ClearChildren();
	for (int32 i = 0; i < Stage->NumCameras(); ++i)
	{
		UTTAIndexButton* Button = WidgetTree->ConstructWidget<UTTAIndexButton>(UTTAIndexButton::StaticClass());
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushResourceObject(Stage->GetThumbnail(i));
		Image->SetDesiredSizeOverride(FVector2D(Stage->ThumbnailSize));
		Button->AddChild(Image);
		Button->Index = i;
		Button->Bind();
		Button->OnIndexClicked.BindLambda([this](int32 Camera) { SetCamera(Camera); });
		Thumbs->AddChildToHorizontalBox(Button);
	}
	Changed();
}

template <typename Fn> bool UTTAEditorWidget::Apply(const TCHAR* What, Fn&& Op)
{
	try
	{
		Op();
	}
	catch (const std::exception& Error) // invalid_argument and out_of_range: Core left the cut unchanged
	{
		UE_LOG(LogTTAEditor, Warning, TEXT("%s rejected: %hs"), What, Error.what());
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s: %hs"), What, Error.what())));
		return false;
	}
	StatusText->SetText(FText::GetEmpty());
	Changed();
	return true;
}

void UTTAEditorWidget::Changed()
{
	const int32 Count = static_cast<int32>(Cut.clips().size());
	if (Selected >= Count) Selected = Count - 1;
	Player->SetCut(Cut);
	if (Stage && !ReelPath.IsEmpty() && Count > 0) TTAReels::Save(Cut, ReelPath);
}

uint32 UTTAEditorWidget::ClipStartCutMs(int32 Clip) const
{
	uint32 Start = 0;
	for (int32 i = 0; i < Clip; ++i) Start += Cut.clips()[i].durationMs();
	return Start;
}

TOptional<uint32> UTTAEditorWidget::PlayheadReplayMs() const
{
	if (!Cut.clips().size() || Selected == INDEX_NONE) return {};
	const tta::Clip& Clip = Cut.clips()[Selected];
	const uint32 IntoClip = FMath::Clamp<uint32>(Player->GetCutMs() - FMath::Min(Player->GetCutMs(), ClipStartCutMs(Selected)), 0, Clip.durationMs());
	return Clip.startMs + static_cast<uint32>(FMath::RoundToInt(IntoClip * Clip.speed));
}

bool UTTAEditorWidget::AutoEdit()
{
	std::vector<int> Cameras;
	for (int32 i = 0; i < Stage->NumCameras(); ++i)
	{
		if (i != HeavenCam) Cameras.push_back(i);
	}
	if (Cameras.empty()) Cameras.push_back(HeavenCam); // no placed cameras: the Heaven Cam films it all
	return Apply(TEXT("Auto-edit"), [&] { Cut = tta::autoEdit(Stage->GetReplay(), Cameras, HeavenCam); Selected = 0; });
}

bool UTTAEditorWidget::AddClipAt(uint32 ReplayMs)
{
	const uint32 RunMs = static_cast<uint32>(Stage->GetReplay().durationSeconds() * 1000.0);
	tta::Clip Clip;
	Clip.camera = Selected != INDEX_NONE ? Cut.clips()[Selected].camera : 0;
	Clip.startMs = ReplayMs > NewClipMs / 2 ? ReplayMs - NewClipMs / 2 : 0;
	Clip.endMs = FMath::Min(Clip.startMs + NewClipMs, FMath::Max(RunMs, Clip.startMs + 1));
	return Apply(TEXT("Add clip"), [&] { Cut.addClip(Clip); Selected = static_cast<int32>(Cut.clips().size()) - 1; });
}

bool UTTAEditorWidget::TrimIn()
{
	const TOptional<uint32> At = PlayheadReplayMs();
	return At && Apply(TEXT("Trim in"), [&] { Cut.trim(Selected, *At, Cut.clips()[Selected].endMs); });
}

bool UTTAEditorWidget::TrimOut()
{
	const TOptional<uint32> At = PlayheadReplayMs();
	return At && Apply(TEXT("Trim out"), [&] { Cut.trim(Selected, Cut.clips()[Selected].startMs, *At); });
}

bool UTTAEditorWidget::SplitHere()
{
	const TOptional<uint32> At = PlayheadReplayMs();
	return At && Apply(TEXT("Split"), [&] { Cut.split(Selected, *At); ++Selected; });
}

bool UTTAEditorWidget::MoveSelected(int32 By)
{
	return Selected != INDEX_NONE && Apply(TEXT("Move"), [&] { Cut.move(Selected, Selected + By); Selected += By; });
}

bool UTTAEditorWidget::DeleteSelected()
{
	return Selected != INDEX_NONE && Apply(TEXT("Delete"), [&] { Cut.removeClip(Selected); });
}

bool UTTAEditorWidget::SetCamera(int32 Camera)
{
	return Selected != INDEX_NONE && Apply(TEXT("Angle"), [&] { Cut.setCamera(Selected, Camera); });
}

bool UTTAEditorWidget::CycleSlowMo()
{
	return Selected != INDEX_NONE && Apply(TEXT("Slow-mo"), [&] { Cut.setSpeed(Selected, NextStep(kSlowSteps, Cut.clips()[Selected].speed)); });
}

bool UTTAEditorWidget::CycleZoom()
{
	return Selected != INDEX_NONE && Apply(TEXT("Zoom"), [&] { Cut.setZoom(Selected, NextStep(kZoomSteps, Cut.clips()[Selected].zoom)); });
}

bool UTTAEditorWidget::AddOverlay(uint8 Type, const FString& Content)
{
	tta::Overlay Overlay;
	Overlay.type = static_cast<tta::OverlayType>(Type);
	Overlay.atMs = Player->GetCutMs();
	Overlay.durationMs = OverlayMs;
	Overlay.content = TCHAR_TO_UTF8(*Content);
	return Apply(TEXT("Overlay"), [&] { Cut.addOverlay(Overlay); });
}

void UTTAEditorWidget::OnPlay()
{
	if (Player->IsPlaying()) Player->Stop();
	else Player->Play();
}

void UTTAEditorWidget::OnSticker() { AddOverlay(static_cast<uint8>(tta::OverlayType::Sticker), DefaultSticker); }
void UTTAEditorWidget::OnCaption() { AddOverlay(static_cast<uint8>(tta::OverlayType::Caption), CaptionBox->GetText().ToString()); }
void UTTAEditorWidget::OnWasted() { AddOverlay(static_cast<uint8>(tta::OverlayType::WastedCard), FString()); }

void UTTAEditorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Stage) return;
	Player->Advance(InDeltaTime);
	if (const TOptional<tta::CutFrame> Frame = Player->GetFrame())
	{
		if (!Player->IsPlaying()) Selected = Frame->clip;
		Stage->ShowAt(Frame->replaySeconds, Frame->camera, Frame->zoom);
	}
	Timeline->SetSelection(Selected, Player->GetCutMs());

	const ATTAGameState* State = GetWorld() ? GetWorld()->GetGameState<ATTAGameState>() : nullptr;
	if (!State || State->GetPhase() != ETTAPhase::Edit) return;
	const int32 Left = State->GetSecondsLeft();
	TimerText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Left / 60, Left % 60)));
	if (Left <= 0 && !bTimeoutHandled)
	{
		bTimeoutHandled = true;
		if (Cut.clips().empty()) AutoEdit(); // out of time: the fallback cut (autosaved by Apply)
	}
}
