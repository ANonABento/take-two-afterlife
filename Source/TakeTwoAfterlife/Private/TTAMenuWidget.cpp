#include "TTAMenuWidget.h"
#include "TTAPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
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
#include "Engine/Texture2D.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "TTAReels.h"
#include "TTAUIStyle.h"
#include "tta/JoinCode.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAMenu, Log, All);

namespace
{
// Layout: screens are a centred column (two of them for Title / How to play; Lobby is one on the left).
constexpr float kScreenW = 520.f, kWideW = 2 * kScreenW + 40.f;
constexpr float kButtonW = 480.f, kButtonH = 72.f;
constexpr float kRowLabelW = 180.f, kRowControlW = 280.f;  // fits inside a panel (520 - 2 * 32)
constexpr float kJoinW = 160.f;
constexpr float kLobbyW = 420.f;  // lobby panel hugs the left edge so Heaven stays visible
constexpr float kTitleTilt = -4.f, kSkyOpacity = 0.5f;
const float kGap = UTTAUIStyle::Space(3);

// Puts a gap after the child along its box's direction.
void GapAfter(UPanelSlot* Slot)
{
	if (UVerticalBoxSlot* V = Cast<UVerticalBoxSlot>(Slot)) V->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, kGap));
	if (UHorizontalBoxSlot* H = Cast<UHorizontalBoxSlot>(Slot)) H->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, kGap));
}

// Switcher slot per screen, in tta::MenuScreen order.
constexpr int32 SlotOf(tta::MenuScreen Screen) { return static_cast<int32>(Screen); }

// tta palette index -> colour token (ETTAColor is in palette order).
ETTAColor Token(int32 PaletteIndex) { return static_cast<ETTAColor>(PaletteIndex); }
}

FLinearColor UTTAMenuWidget::Palette(int32 Index)
{
	return UTTAUIStyle::Colour(Index);
}

UBorder* UTTAMenuWidget::Card(UPanelWidget* Parent, ETTAColor Fill)
{
	UBorder* Panel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
	if (Fill != ETTAColor::Bone) Panel->SetBrushColor(UTTAUIStyle::Color(Fill));
	GapAfter(Parent->AddChild(Panel));
	return Panel;
}

UTextBlock* UTTAMenuWidget::Label(UPanelWidget* Parent, const FString& Text, ETTAFont Size, ETTAColor Colour)
{
	UTextBlock* Block = UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(Text), Size, Colour);
	Parent->AddChild(Block);
	return Block;
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind)
{
	return MenuButton(Parent, Text, Kind, kButtonW);
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind, float Width)
{
	UButton* Button = UTTAUIStyle::MakeButton(WidgetTree, FText::FromString(Text), Kind);
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
	Label(LabelSize, Name, ETTAFont::Body);
	USizeBox* ControlSize = WidgetTree->ConstructWidget<USizeBox>();
	ControlSize->SetWidthOverride(kRowControlW);
	ControlSize->SetContent(Control);
	Row->AddChild(ControlSize);
	GapAfter(Parent->AddChild(Row));
}

USlider* UTTAMenuWidget::SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01)
{
	USlider* Slider = UTTAUIStyle::MakeSlider(WidgetTree);
	Slider->SetValue(Value01);
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

	// Overlay root: half-opacity Sky so a 3D backdrop shows through, screens on top.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	UTTAUIStyle::Get().ApplyBackground(Background, ETTAUIScreen::Menu, tta::kSky, kSkyOpacity);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	UOverlaySlot* SwitcherSlot = Root->AddChildToOverlay(Switcher);
	SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
	SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
	SwitcherSlot->SetPadding(UTTAUIStyle::Get().SafeArea);

	// Title: menu column | angel placeholder (v3/title.png).
	{
		UVerticalBox* Wide = ScreenPanel(kWideW);
		UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
		Wide->AddChild(Columns);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* MenuSlot = Columns->AddChildToHorizontalBox(Box);
		MenuSlot->SetVerticalAlignment(VAlign_Center);
		GapAfter(MenuSlot);
		UVerticalBox* TitleRow = WidgetTree->ConstructWidget<UVerticalBox>();
		// Logo art slot when set, else the text title.
		if (UTexture2D* Logo = UTTAUIStyle::Get().LoadSlotTexture(UTTAUIStyle::Get().MenuLogo))
		{
			UImage* LogoImage = WidgetTree->ConstructWidget<UImage>();
			LogoImage->SetBrushFromTexture(Logo, true);
			Cast<UVerticalBoxSlot>(TitleRow->AddChild(LogoImage))->SetHorizontalAlignment(HAlign_Center);
		}
		else
		{
			UTextBlock* Title = Label(TitleRow, TEXT("TAKE TWO: AFTERLIFE"), ETTAFont::H1, ETTAColor::Bone);
			Title->SetRenderTransformAngle(kTitleTilt);
			Title->SetJustification(ETextJustify::Center);
		}
		Label(TitleRow, TEXT("Director's Cut"), ETTAFont::H2, ETTAColor::Yellow)->SetJustification(ETextJustify::Center);
		Box->AddChild(TitleRow);
		GapAfter(TitleRow->Slot);
		MenuButton(Box, TEXT("PLAY"), ETTAButtonKind::Primary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnPlayClicked);
		MenuButton(Box, TEXT("SETTINGS"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Box, TEXT("REELS"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnReelsClicked);
		MenuButton(Box, TEXT("QUIT"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnQuitClicked);
		USizeBox* AngelSize = WidgetTree->ConstructWidget<USizeBox>();
		AngelSize->SetWidthOverride(kScreenW);
		AngelSize->SetContent(UTTAUIStyle::MakeAngel(WidgetTree));
		Columns->AddChildToHorizontalBox(AngelSize)->SetVerticalAlignment(VAlign_Center);
	}
	// Join: the Play page (Host, Play Solo, code-or-IP + JOIN, Back), opened from PLAY like Settings.
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		Label(Box, TEXT("PLAY"), ETTAFont::H1, ETTAColor::Bone)->SetJustification(ETextJustify::Center);
		JoinCard = Card(Box);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		JoinCard->SetContent(Inner);
		const float InnerW = kScreenW - 2 * UTTAUIStyle::Space(4);
		MenuButton(Inner, TEXT("HOST"), ETTAButtonKind::Primary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHostClicked);
		// Solo also adds bots (HostSolo), which HOST doesn't, so it keeps its own button.
		MenuButton(Inner, TEXT("PLAY SOLO"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnPlaySoloClicked);
		Label(Inner, TEXT("Join with the host's code or IP"), ETTAFont::Body);
		UHorizontalBox* JoinRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		CodeBox = UTTAUIStyle::MakeTextField(WidgetTree);
		CodeBox->SetHintText(FText::FromString(TEXT("ABCDEF or 127.0.0.1:7777")));
		CodeBox->OnTextCommitted.AddDynamic(this, &UTTAMenuWidget::OnCodeCommitted);
		UHorizontalBoxSlot* FieldSlot = JoinRow->AddChildToHorizontalBox(CodeBox);
		FieldSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		FieldSlot->SetVerticalAlignment(VAlign_Center);
		GapAfter(FieldSlot);
		MenuButton(JoinRow, TEXT("JOIN"), ETTAButtonKind::Primary, kJoinW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnJoinClicked);
		Inner->AddChild(JoinRow);
		JoinError = Label(Inner, TEXT(""), ETTAFont::Small, ETTAColor::Red);
		MenuButton(Box, TEXT("BACK"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Lobby
	{
		// Compact left panel over the walkable Heaven: code pill, look pickers, actions, ready count.
		UVerticalBox* Box = ScreenPanel(kLobbyW);
		CastChecked<UWidgetSwitcherSlot>(Box->GetParent()->Slot)->SetHorizontalAlignment(HAlign_Left);
		UBorder* Face = Card(Box);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		Face->SetContent(Inner);
		const float InnerW = kLobbyW - 2 * UTTAUIStyle::Space(4);
		Label(Inner, TEXT("ROOM CODE"), ETTAFont::Small);
		UBorder* Pill = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::CodePill);
		GapAfter(Inner->AddChild(Pill));
		RoomCodeText = UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(TEXT("------")), ETTAFont::H2);
		RoomCodeText->SetJustification(ETextJustify::Center);
		Pill->SetContent(RoomCodeText);
		LookSwatch = Card(Inner, ETTAColor::Red);
		LookText = Label(LookSwatch, TEXT(""), ETTAFont::Small);
		MenuButton(Inner, TEXT("COLOUR >"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnColourNext);
		MenuButton(Inner, TEXT("HAT >"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHatNext);
		MenuButton(Inner, TEXT("FACE >"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnFaceNext);
		StartButton = MenuButton(Inner, TEXT("START"), ETTAButtonKind::Primary, InnerW);
		StartButton->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnStartClicked);
		MenuButton(Inner, TEXT("SETTINGS"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Inner, TEXT("LEAVE"), ETTAButtonKind::Secondary, InnerW)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		// Ready count: tick icon + the label SetPlayers writes to.
		UHorizontalBox* Ready = UTTAUIStyle::MakeIconCount(WidgetTree, ETTAAsset::IconReady, FText::GetEmpty());
		WaitingText = CastChecked<UTextBlock>(Ready->GetChildAt(1));
		WaitingText->SetAutoWrapText(true);
		Inner->AddChild(Ready);
	}
	// Settings
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		Label(Box, TEXT("SETTINGS"), ETTAFont::H1, ETTAColor::Bone)->SetJustification(ETextJustify::Center);
		UBorder* Face = Card(Box);
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
		// Video options fill in when Settings opens: Initialize must not need GameUserSettings/RHI.
		MicMeter = WidgetTree->ConstructWidget<UProgressBar>();
		MicMeter->SetFillColorAndOpacity(UTTAUIStyle::Color(ETTAColor::Grass));
		SettingRow(Inner, TEXT("Mic test"), MicMeter);
		SettingsText = Label(Inner, TEXT(""), ETTAFont::Small);
		MenuButton(Box, TEXT("BACK"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// How to play
	{
		UVerticalBox* Box = ScreenPanel(kWideW);
		UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
		const ETTAColor Fills[] = {ETTAColor::Orange, ETTAColor::Red, ETTAColor::Teal, ETTAColor::Purple};
		for (size_t i = 0; i < tta::kHowToPlay.size(); ++i)
		{
			UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
			Card(Cards, Fills[i])->SetContent(Inner);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].title), ETTAFont::H2, ETTAColor::Bone);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].line), ETTAFont::Small)->SetAutoWrapText(true);
		}
		Box->AddChild(Cards);
		MenuButton(Box, TEXT("BACK"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Reels
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		Label(Box, TEXT("REELS"), ETTAFont::H1, ETTAColor::Bone);
		ReelList = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChild(ReelList);
		MenuButton(Box, TEXT("BACK"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
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
		JoinCard->SetBrush(UTTAUIStyle::Brush(ETTAAsset::Panel));  // clears the error tint
		JoinError->SetText(FText::GetEmpty());
	}
	if (Screen == tta::MenuScreen::Settings) RefreshVideo();
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

void UTTAMenuWidget::PlaySolo()
{
	if (!Open(tta::MenuScreen::Lobby)) return;
	bHost = true;
	OnPlaySolo.ExecuteIfBound();
}

bool UTTAMenuWidget::SubmitJoinCode(const FString& Typed)
{
	check(Flow.current() == tta::MenuScreen::Join);
	const std::optional<std::string> Code = tta::normalizeJoinCode(TCHAR_TO_UTF8(*Typed));
	if (!Code)
	{
		JoinCard->SetBrushColor(UTTAUIStyle::Color(ETTAColor::Red));
		JoinError->SetText(FText::FromString(TEXT("Not a code (6 letters, no I or O) or an IP")));
		return false;
	}
	bHost = false;
	Open(tta::MenuScreen::Lobby);
	OnJoinCode.ExecuteIfBound(UTF8_TO_TCHAR(Code->c_str()));
	return true;
}

void UTTAMenuWidget::SubmitCodeOrAddress(const FString& Typed)
{
	const FString Trimmed = Typed.TrimStartAndEnd();
	if (Trimmed.Contains(TEXT(".")) || Trimmed.Contains(TEXT(":")))
	{
		OnJoinDirect.ExecuteIfBound(Trimmed);
		return;
	}
	SubmitJoinCode(Trimmed);
}

void UTTAMenuWidget::OnCodeCommitted(const FText& Text, ETextCommit::Type Method)
{
	if (Method == ETextCommit::OnEnter) SubmitCodeOrAddress(Text.ToString());
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
	if (!ResolutionBox || !WindowModeBox) return;
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	if (!Video)
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("No GameUserSettings (bare automation world); video settings skipped"));
		return;
	}
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
	if (!Video)
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("No GameUserSettings (bare automation world); video settings skipped"));
		return;
	}
	Video->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(static_cast<int32>(Mode)));
	Video->ApplySettings(false);
}

bool UTTAMenuWidget::ApplyResolution(int32 ChoiceIndex)
{
	// Saved/combo indices are 0-based; a stale one (e.g. a 1-based save or a shorter list) is clamped.
	const int32 Index = tta::clampResolutionIndex(ChoiceIndex, NumResolutionChoices());
	if (Index < 0)
	{
		UE_LOG(LogTTAMenu, Log, TEXT("No resolution choices; resolution not applied"));
		return false;
	}
	if (Index != ChoiceIndex)
	{
		UE_LOG(LogTTAMenu, Verbose, TEXT("Resolution choice %d clamped to %d"), ChoiceIndex, Index);
	}
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	if (!Video)
	{
		UE_LOG(LogTTAMenu, Warning, TEXT("No GameUserSettings (bare automation world); video settings skipped"));
		return false;
	}
	const tta::Resolution& R = ResolutionChoices[Index];
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
		MenuButton(ReelList, FPaths::GetBaseFilename(Path), ETTAButtonKind::Secondary);
	}
	if (Reels.IsEmpty()) Label(ReelList, TEXT("No reels yet - go make a film!"), ETTAFont::Body);
}
