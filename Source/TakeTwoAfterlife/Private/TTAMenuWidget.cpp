#include "TTAMenuWidget.h"
#include "TTAPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "TTAReels.h"
#include "tta/JoinCode.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAMenu, Log, All);

namespace
{
// Ref sizes at 1080p: H1 120, room code 160, body 32, small 24.
constexpr int32 kH1 = 120, kTitle = 96, kCode = 160, kBody = 32, kSmall = 24;
// Layout: screens are a centred 520 px column (two of them for Lobby / How to play).
constexpr float kScreenW = 520.f, kWideW = 2 * kScreenW + 40.f;
constexpr float kButtonW = 480.f, kButtonH = 72.f, kGap = 12.f, kCorner = 18.f;
constexpr float kRowLabelW = 180.f, kRowControlW = 280.f;  // fits inside a Card (520 - 2 * 30)
constexpr float kTitleTilt = -4.f, kSkyOpacity = 0.5f;
constexpr float kHoverDarken = 0.25f, kPressedDarken = 0.45f;  // lerp toward Ink
const TCHAR* const kDisplayFontPath = TEXT("/Game/UI/Fonts/LilitaOne.LilitaOne");
const TCHAR* const kBodyFontPath = TEXT("/Game/UI/Fonts/Nunito.Nunito");

UObject* LoadMenuFont(const TCHAR* Path)
{
	UObject* Font = LoadObject<UObject>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Font) UE_LOG(LogTTAMenu, Warning, TEXT("Menu font %s not imported; using the engine font"), Path);
	return Font;
}

// Puts a gap after the child along its box's direction.
void GapAfter(UPanelSlot* Slot)
{
	if (UVerticalBoxSlot* V = Cast<UVerticalBoxSlot>(Slot)) V->SetPadding(FMargin(0.f, 0.f, 0.f, kGap));
	if (UHorizontalBoxSlot* H = Cast<UHorizontalBoxSlot>(Slot)) H->SetPadding(FMargin(0.f, 0.f, kGap, 0.f));
}

// Switcher slot per screen, in tta::MenuScreen order.
constexpr int32 SlotOf(tta::MenuScreen Screen) { return static_cast<int32>(Screen); }
}

FLinearColor UTTAMenuWidget::Palette(int32 Index)
{
	check(Index >= 0 && Index < static_cast<int32>(tta::kPaletteHex.size()));
	return FLinearColor(FColor::FromHex(FString(tta::kPaletteHex[Index])));
}

UBorder* UTTAMenuWidget::Card(UPanelWidget* Parent, int32 Fill)
{
	// Sticker card: Ink outline around a palette fill.
	UBorder* Outline = WidgetTree->ConstructWidget<UBorder>();
	Outline->SetBrushColor(Palette(tta::kInk));
	Outline->SetPadding(FMargin(6.f));
	UBorder* Face = WidgetTree->ConstructWidget<UBorder>();
	Face->SetBrushColor(Palette(Fill));
	Face->SetPadding(FMargin(24.f));
	Outline->SetContent(Face);
	Parent->AddChild(Outline);
	return Face;
}

UTextBlock* UTTAMenuWidget::Label(UPanelWidget* Parent, const FString& Text, int32 Size, bool bDisplay)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
	Block->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = Size;
	// Lilita One / Nunito when imported; otherwise the engine face, Bold for display.
	if (UObject* Face = bDisplay ? DisplayFont : BodyFont)
	{
		Font.FontObject = Face;
		Font.TypefaceFontName = NAME_None;
	}
	else
	{
		Font.TypefaceFontName = bDisplay ? FName("Bold") : FName("Regular");
	}
	Font.OutlineSettings.OutlineSize = bDisplay ? 3 : 0;
	Font.OutlineSettings.OutlineColor = Palette(tta::kInk);
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(bDisplay ? Palette(tta::kBone) : Palette(tta::kInk)));
	Parent->AddChild(Block);
	return Block;
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, int32 Fill)
{
	return MenuButton(Parent, Text, Fill, kButtonW);
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, int32 Fill, float Width)
{
	const FLinearColor Base = Palette(Fill);
	const FLinearColor Ink = Palette(tta::kInk);
	FButtonStyle Style;
	Style.SetNormal(FSlateRoundedBoxBrush(Base, kCorner));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor::LerpUsingHSV(Base, Ink, kHoverDarken), kCorner));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor::LerpUsingHSV(Base, Ink, kPressedDarken), kCorner));
	Style.SetDisabled(FSlateRoundedBoxBrush(Base.CopyWithNewOpacity(0.4f), kCorner));
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetStyle(Style);
	Label(Button, Text, kBody, true);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(kButtonH);
	Size->SetContent(Button);
	GapAfter(Parent->AddChild(Size));
	return Button;
}

UVerticalBox* UTTAMenuWidget::ScreenPanel(float Width)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Size->SetContent(Box);
	UWidgetSwitcherSlot* SwitchSlot = CastChecked<UWidgetSwitcherSlot>(Switcher->AddChild(Size));
	SwitchSlot->SetHorizontalAlignment(HAlign_Center);
	SwitchSlot->SetVerticalAlignment(VAlign_Center);
	return Box;
}

void UTTAMenuWidget::SettingRow(UPanelWidget* Parent, const FString& Name, UWidget* Control)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>();
	LabelSize->SetWidthOverride(kRowLabelW);
	Row->AddChild(LabelSize);
	Label(LabelSize, Name, kSmall);
	USizeBox* ControlSize = WidgetTree->ConstructWidget<USizeBox>();
	ControlSize->SetWidthOverride(kRowControlW);
	ControlSize->SetContent(Control);
	Row->AddChild(ControlSize);
	GapAfter(Parent->AddChild(Row));
}

USlider* UTTAMenuWidget::SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01)
{
	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetValue(Value01);
	Slider->SetSliderBarColor(Palette(tta::kInk));
	Slider->SetSliderHandleColor(Palette(tta::kBlue));
	SettingRow(Parent, Name, Slider);
	return Slider;
}

UComboBoxString* UTTAMenuWidget::SettingCombo(UPanelWidget* Parent, const FString& Name)
{
	UComboBoxString* Combo = WidgetTree->ConstructWidget<UComboBoxString>();
	SettingRow(Parent, Name, Combo);
	return Combo;
}

void UTTAMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	DisplayFont = LoadMenuFont(kDisplayFontPath);
	BodyFont = LoadMenuFont(kBodyFontPath);

	// Overlay root: half-opacity Sky so a 3D backdrop shows through, screens on top.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(Palette(tta::kSky).CopyWithNewOpacity(kSkyOpacity));
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	UOverlaySlot* SwitcherSlot = Root->AddChildToOverlay(Switcher);
	SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
	SwitcherSlot->SetVerticalAlignment(VAlign_Fill);

	// Title
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		UVerticalBox* TitleRow = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Title = Label(TitleRow, TEXT("TAKE TWO: AFTERLIFE"), kTitle, true);
		Title->SetRenderTransformAngle(kTitleTilt);
		Title->SetJustification(ETextJustify::Center);
		Label(TitleRow, TEXT("Director's Cut"), kBody)->SetJustification(ETextJustify::Center);
		Box->AddChild(TitleRow);
		GapAfter(TitleRow->Slot);
		MenuButton(Box, TEXT("HOST"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHostClicked);
		MenuButton(Box, TEXT("JOIN"), tta::kGrass)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnJoinClicked);
		MenuButton(Box, TEXT("REELS"), tta::kTeal)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnReelsClicked);
		MenuButton(Box, TEXT("SETTINGS"), tta::kPurple)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Box, TEXT("HOW TO PLAY"), tta::kPink)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHowToClicked);
		MenuButton(Box, TEXT("QUIT"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnQuitClicked);
	}
	// Join
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		JoinCard = Card(Box, tta::kBone);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		JoinCard->SetContent(Inner);
		Label(Inner, TEXT("Ask the host for the code"), kBody);
		CodeBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		CodeBox->SetHintText(FText::FromString(TEXT("ABCDEF")));
		CodeBox->OnTextCommitted.AddDynamic(this, &UTTAMenuWidget::OnCodeCommitted);
		Inner->AddChild(CodeBox);
		JoinError = Label(Inner, TEXT(""), kSmall);
		IpBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		IpBox->SetHintText(FText::FromString(TEXT("127.0.0.1:7777")));
		Inner->AddChild(IpBox);
		MenuButton(Inner, TEXT("DIRECT IP"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnDirectClicked);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Lobby
	{
		// Two columns (players | cosmetics), action buttons in a bottom row.
		UVerticalBox* Box = ScreenPanel(kWideW);
		UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
		Box->AddChild(Columns);
		GapAfter(Columns->Slot);
		auto Column = [&]()
		{
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
			Size->SetWidthOverride(kScreenW);
			UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
			Size->SetContent(Inner);
			Columns->AddChild(Size);
			return Inner;
		};
		UVerticalBox* Players = Column();
		Cast<UHorizontalBoxSlot>(Players->GetParent()->Slot)->SetPadding(FMargin(0.f, 0.f, kWideW - 2 * kScreenW, 0.f));
		RoomCodeText = Label(Card(Players, tta::kBone), TEXT("------"), kCode, true);
		WaitingText = Label(Players, TEXT(""), kBody);
		UVerticalBox* Cosmetics = Column();
		LookSwatch = Card(Cosmetics, tta::kRed);
		LookText = Label(LookSwatch, TEXT(""), kBody, true);
		MenuButton(Cosmetics, TEXT("COLOUR >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnColourNext);
		MenuButton(Cosmetics, TEXT("HAT >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHatNext);
		MenuButton(Cosmetics, TEXT("FACE >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnFaceNext);
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
		const float ActionW = (kWideW - 2 * kGap) / 3.f;
		StartButton = MenuButton(Bottom, TEXT("START"), tta::kGrass, ActionW);
		StartButton->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnStartClicked);
		MenuButton(Bottom, TEXT("SETTINGS"), tta::kPurple, ActionW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Bottom, TEXT("LEAVE"), tta::kInk, ActionW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		Box->AddChild(Bottom);
	}
	// Settings
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		UBorder* Face = Card(Box, tta::kBone);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		Face->SetContent(Inner);
		SettingSlider(Inner, TEXT("Master"), Settings.master / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMasterChanged);
		SettingSlider(Inner, TEXT("Music"), Settings.music / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMusicChanged);
		SettingSlider(Inner, TEXT("Voice"), Settings.vo / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnVoChanged);
		SettingSlider(Inner, TEXT("SFX"), Settings.sfx / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSfxChanged);
		SettingSlider(Inner, TEXT("Mouse sensitivity"), Settings.sensitivity / 500.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSensitivityChanged);
		WindowModeBox = SettingCombo(Inner, TEXT("Display"));
		for (const char* Name : tta::kWindowModeNames) WindowModeBox->AddOption(UTF8_TO_TCHAR(Name));
		WindowModeBox->OnSelectionChanged.AddDynamic(this, &UTTAMenuWidget::OnWindowModeChanged);
		ResolutionBox = SettingCombo(Inner, TEXT("Resolution"));
		ResolutionBox->OnSelectionChanged.AddDynamic(this, &UTTAMenuWidget::OnResolutionChanged);
		RefreshVideo();
		MicMeter = WidgetTree->ConstructWidget<UProgressBar>();
		MicMeter->SetFillColorAndOpacity(Palette(tta::kGrass));
		SettingRow(Inner, TEXT("Mic test"), MicMeter);
		SettingsText = Label(Inner, TEXT(""), kSmall);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// How to play
	{
		UVerticalBox* Box = ScreenPanel(kWideW);
		UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
		const int32 Fills[] = {tta::kOrange, tta::kRed, tta::kTeal, tta::kPurple};
		for (size_t i = 0; i < tta::kHowToPlay.size(); ++i)
		{
			UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
			Card(Cards, Fills[i])->SetContent(Inner);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].title), kBody, true);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].line), kSmall)->SetAutoWrapText(true);
		}
		Box->AddChild(Cards);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Reels
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		Label(Box, TEXT("REELS"), kH1, true);
		ReelList = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChild(ReelList);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}

	SetPlayers(1, 0);
	RefreshLook();
	RefreshSettings();
	Switcher->SetActiveWidgetIndex(SlotOf(Flow.current()));
}

int32 UTTAMenuWidget::GetVisibleIndex() const
{
	return Switcher->GetActiveWidgetIndex();
}

bool UTTAMenuWidget::Open(tta::MenuScreen Screen)
{
	if (!Flow.open(Screen))
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("Can't open screen %d from %d"), SlotOf(Screen), SlotOf(Flow.current()));
		return false;
	}
	if (Screen == tta::MenuScreen::Reels) RefreshReels();
	if (Screen == tta::MenuScreen::Join)
	{
		JoinCard->SetBrushColor(Palette(tta::kBone));
		JoinError->SetText(FText::GetEmpty());
	}
	Switcher->SetActiveWidgetIndex(SlotOf(Flow.current()));
	return true;
}

bool UTTAMenuWidget::Back()
{
	if (!Flow.back()) return false;
	Switcher->SetActiveWidgetIndex(SlotOf(Flow.current()));
	return true;
}

void UTTAMenuWidget::Host()
{
	if (!Open(tta::MenuScreen::Lobby)) return;
	bHost = true;
	OnHost.ExecuteIfBound();
}

bool UTTAMenuWidget::SubmitJoinCode(const FString& Typed)
{
	check(Flow.current() == tta::MenuScreen::Join);
	const std::optional<std::string> Code = tta::normalizeJoinCode(TCHAR_TO_UTF8(*Typed));
	if (!Code)
	{
		JoinCard->SetBrushColor(Palette(tta::kRed));
		JoinError->SetText(FText::FromString(TEXT("That's not a code - 6 letters, no I or O")));
		return false;
	}
	bHost = false;
	Open(tta::MenuScreen::Lobby);
	OnJoinCode.ExecuteIfBound(UTF8_TO_TCHAR(Code->c_str()));
	return true;
}

void UTTAMenuWidget::OnCodeCommitted(const FText& Text, ETextCommit::Type Method)
{
	if (Method == ETextCommit::OnEnter) SubmitJoinCode(Text.ToString());
}

void UTTAMenuWidget::SetRoomCode(const FString& Code, bool bIsHost)
{
	bHost = bIsHost;
	RoomCodeText->SetText(FText::FromString(Code));
}

void UTTAMenuWidget::SetPlayers(int32 Players, int32 Ready)
{
	StartBlockedReason = UTF8_TO_TCHAR(tta::startBlockedReason(Players, Ready).c_str());
	WaitingText->SetText(FText::FromString(StartBlockedReason.IsEmpty() ? TEXT("Everyone's ready!") : StartBlockedReason));
	StartButton->SetIsEnabled(CanStart());
	StartButton->SetToolTipText(FText::FromString(StartBlockedReason));
}

void UTTAMenuWidget::OnStartClicked()
{
	if (CanStart()) OnStart.ExecuteIfBound();
}

void UTTAMenuWidget::CycleLook(tta::LookPart Part, int32 Step)
{
	Look = tta::cycleLook(Look, Part, Step);
	RefreshLook();
	if (ATTAPlayerState* State = GetOwningPlayerState<ATTAPlayerState>()) State->ServerSetLook(Look.colour, Look.hat, Look.face);
}

FLinearColor UTTAMenuWidget::GetLookColour() const
{
	return Palette(tta::kPlayerColours[Look.colour]);
}

void UTTAMenuWidget::RefreshLook()
{
	LookSwatch->SetBrushColor(GetLookColour());
	LookText->SetText(FText::FromString(FString::Printf(TEXT("Hat: %s  Face: %s"),
		UTF8_TO_TCHAR(tta::kHats[Look.hat]), UTF8_TO_TCHAR(tta::kFaces[Look.face]))));
}

void UTTAMenuWidget::SetSettings(const tta::ClientSettings& InSettings)
{
	Settings = InSettings;
	RefreshSettings();
}

void UTTAMenuWidget::RefreshSettings()
{
	Settings = tta::clampClientSettings(Settings);
	SettingsText->SetText(FText::FromString(FString::Printf(TEXT("Master %d  Music %d  Voice %d  SFX %d  Sensitivity %d%%"),
		Settings.master, Settings.music, Settings.vo, Settings.sfx, Settings.sensitivity)));
}

static_assert(static_cast<int32>(tta::WindowMode::Fullscreen) == EWindowMode::Fullscreen
	&& static_cast<int32>(tta::WindowMode::WindowedFullscreen) == EWindowMode::WindowedFullscreen
	&& static_cast<int32>(tta::WindowMode::Windowed) == EWindowMode::Windowed,
	"tta::WindowMode must match EWindowMode");

void UTTAMenuWidget::RefreshVideo()
{
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	check(Video);
	TArray<FIntPoint> Supported;
	if (!UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported))
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("No supported fullscreen resolutions reported; resolution list is empty"));
	}
	std::vector<tta::Resolution> Raw;
	for (const FIntPoint& R : Supported) Raw.push_back({R.X, R.Y});
	ResolutionChoices = tta::resolutionChoices(std::move(Raw));

	ResolutionBox->ClearOptions();
	for (const tta::Resolution& R : ResolutionChoices) ResolutionBox->AddOption(FString::Printf(TEXT("%d x %d"), R.width, R.height));
	const FIntPoint Current = Video->GetScreenResolution();
	const int32 Index = tta::resolutionIndex(ResolutionChoices, {Current.X, Current.Y});
	if (Index >= 0) ResolutionBox->SetSelectedIndex(Index);
	WindowModeBox->SetSelectedIndex(static_cast<int32>(Video->GetFullscreenMode()));
}

void UTTAMenuWidget::ApplyWindowMode(tta::WindowMode Mode)
{
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	check(Video);
	Video->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(static_cast<int32>(Mode)));
	Video->ApplySettings(false);
}

bool UTTAMenuWidget::ApplyResolution(int32 ChoiceIndex)
{
	if (ChoiceIndex < 0 || ChoiceIndex >= NumResolutionChoices())
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("Resolution choice %d out of range (%d choices)"), ChoiceIndex, NumResolutionChoices());
		return false;
	}
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	check(Video);
	const tta::Resolution& R = ResolutionChoices[ChoiceIndex];
	Video->SetScreenResolution(FIntPoint(R.width, R.height));
	Video->ApplySettings(false);
	return true;
}

void UTTAMenuWidget::OnWindowModeChanged(FString Item, ESelectInfo::Type Select)
{
	if (Select == ESelectInfo::Direct) return;  // set by RefreshVideo, not the player
	ApplyWindowMode(static_cast<tta::WindowMode>(WindowModeBox->GetSelectedIndex()));
}

void UTTAMenuWidget::OnResolutionChanged(FString Item, ESelectInfo::Type Select)
{
	if (Select == ESelectInfo::Direct) return;
	ApplyResolution(ResolutionBox->GetSelectedIndex());
}

void UTTAMenuWidget::SetMicSamples(const int16* Samples, int32 Count)
{
	MicLevel = tta::micLevel(Samples, static_cast<size_t>(Count));
	MicMeter->SetPercent(MicLevel);
}

void UTTAMenuWidget::RefreshReels()
{
	Reels = TTAReels::List();
	ReelList->ClearChildren();
	for (const FString& Path : Reels)
	{
		MenuButton(ReelList, FPaths::GetBaseFilename(Path), tta::kTeal);
	}
	if (Reels.IsEmpty()) Label(ReelList, TEXT("No reels yet - go make a film!"), kBody);
}
