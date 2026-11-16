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
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "TTALookIcons.h"
#include "TTAReels.h"
#include "TTAUIStyle.h"
#include "tta/JoinCode.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAMenu, Log, All);

namespace
{
// Room code size at 1080p; every other size comes from the UTTAUIStyle type scale.
constexpr int32 kCode = 160;
// Layout: screens are a centred 520 px column (two of them for Lobby / How to play).
constexpr float kScreenW = 520.f, kWideW = 2 * kScreenW + 40.f;
constexpr float kButtonW = 480.f, kButtonH = 72.f, kGap = 12.f, kCorner = 18.f;
// The title painting is the 3D backdrop. This wash sits on top of it and the angels, so it stays faint.
constexpr float kTitleTilt = -4.f, kSkyOpacity = 0.12f;
constexpr float kHoverDarken = 0.25f, kPressedDarken = 0.45f;  // lerp toward Ink

// Puts a gap after the child along its box's direction.
void GapAfter(UPanelSlot* Slot)
{
	if (UVerticalBoxSlot* V = Cast<UVerticalBoxSlot>(Slot)) V->SetPadding(UTTAUIStyle::PadAfter(Orient_Vertical, kGap));
	if (UHorizontalBoxSlot* H = Cast<UHorizontalBoxSlot>(Slot)) H->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, kGap));
}

// Switcher slot per screen, in tta::MenuScreen order.
constexpr int32 SlotOf(tta::MenuScreen Screen) { return static_cast<int32>(Screen); }
}

FLinearColor UTTAMenuWidget::Palette(int32 Index)
{
	return UTTAUIStyle::Colour(Index);
}

UBorder* UTTAMenuWidget::Card(UPanelWidget* Parent, int32 Fill)
{
	return UTTAUIStyle::AddCard(WidgetTree, Parent, Fill);
}

UTextBlock* UTTAMenuWidget::Label(UPanelWidget* Parent, const FString& Text, int32 Size, bool bDisplay)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
	Block->SetText(FText::FromString(Text));
	Block->SetFont(UTTAUIStyle::MakeFont(bDisplay ? DisplayFont : BodyFont, bDisplay, Size));
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
	const UTTAUIStyle& UIStyle = UTTAUIStyle::Get();
	// A button art slot set in TTAUIStyle replaces the palette rounded box for that state.
	auto StateBrush = [&](const FSoftObjectPath& Art, const FLinearColor& Colour) -> FSlateBrush
	{
		if (UTexture2D* Image = UIStyle.LoadSlotTexture(Art))
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(Image);
			return Brush;
		}
		return FSlateRoundedBoxBrush(Colour, kCorner);
	};
	FButtonStyle Style;
	Style.SetNormal(StateBrush(UIStyle.ButtonNormal, Base));
	Style.SetHovered(StateBrush(UIStyle.ButtonHovered, FLinearColor::LerpUsingHSV(Base, Ink, kHoverDarken)));
	Style.SetPressed(StateBrush(UIStyle.ButtonPressed, FLinearColor::LerpUsingHSV(Base, Ink, kPressedDarken)));
	Style.SetDisabled(FSlateRoundedBoxBrush(Base.CopyWithNewOpacity(0.4f), kCorner));
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetStyle(Style);
	Label(Button, Text, UTTAUIStyle::kBody, true);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(kButtonH);
	Size->SetContent(Button);
	GapAfter(Parent->AddChild(Size));
	return Button;
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind)
{
	UButton* Button = UTTAUIStyle::MakeButton(WidgetTree, FText::FromString(Text), Kind);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(kButtonW);
	Size->SetHeightOverride(kButtonH);
	Size->SetContent(Button);
	GapAfter(Parent->AddChild(Size));
	return Button;
}

UVerticalBox* UTTAMenuWidget::ScreenPanel(float Width, EHorizontalAlignment HAlign, EVerticalAlignment VAlign)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Size->SetContent(Box);
	UWidgetSwitcherSlot* SwitchSlot = CastChecked<UWidgetSwitcherSlot>(Switcher->AddChild(Size));
	SwitchSlot->SetHorizontalAlignment(HAlign);
	SwitchSlot->SetVerticalAlignment(VAlign);
	return Box;
}

void UTTAMenuWidget::SettingRow(UPanelWidget* Parent, const FString& Name, UWidget* Control)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* NameText = Label(Row, Name, UTTAUIStyle::kSmall);
	UHorizontalBoxSlot* NameSlot = Cast<UHorizontalBoxSlot>(NameText->Slot);
	FSlateChildSize NameSize(ESlateSizeRule::Fill);
	NameSize.Value = 0.34f;
	NameSlot->SetSize(NameSize);
	NameSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(Control);
	FSlateChildSize ControlSize(ESlateSizeRule::Fill);
	ControlSize.Value = 0.66f;
	ControlSlot->SetSize(ControlSize);
	ControlSlot->SetVerticalAlignment(VAlign_Center);
	GapAfter(Parent->AddChild(Row));
}

USlider* UTTAMenuWidget::SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01)
{
	// Style-guide SliderBar/SliderKnob art, with the current number to the right of the bar.
	USlider* Slider = UTTAUIStyle::MakeSlider(WidgetTree);
	Slider->SetValue(Value01);
	UHorizontalBox* Control = WidgetTree->ConstructWidget<UHorizontalBox>();
	UHorizontalBoxSlot* BarSlot = Control->AddChildToHorizontalBox(Slider);
	BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BarSlot->SetVerticalAlignment(VAlign_Center);
	SettingValues.Add(Label(Control, TEXT(""), UTTAUIStyle::kSmall));
	UHorizontalBoxSlot* NumberSlot = Cast<UHorizontalBoxSlot>(SettingValues.Last()->Slot);
	FSlateChildSize NumberSize(ESlateSizeRule::Fill);
	NumberSize.Value = 0.22f;
	NumberSlot->SetSize(NumberSize);
	NumberSlot->SetVerticalAlignment(VAlign_Center);
	SettingSliders.Add(Slider);
	SettingRow(Parent, Name, Control);
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

	DisplayFont = UTTAUIStyle::Get().LoadFont(true);
	BodyFont = UTTAUIStyle::Get().LoadFont(false);

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

	// Title: logo top-left of the safe area, one-width button stack under it, Host is the gold primary.
	{
		UVerticalBox* Box = ScreenPanel(kButtonW, HAlign_Left, VAlign_Top);
		// Logo art slot when set, else the text title.
		if (UTexture2D* Logo = UTTAUIStyle::Get().LoadSlotTexture(UTTAUIStyle::Get().MenuLogo))
		{
			UImage* LogoImage = WidgetTree->ConstructWidget<UImage>();
			LogoImage->SetBrushFromTexture(Logo, true);
			Cast<UVerticalBoxSlot>(Box->AddChild(LogoImage))->SetHorizontalAlignment(HAlign_Left);
		}
		else
		{
			UTextBlock* Title = Label(Box, TEXT("TAKE TWO: AFTERLIFE"), UTTAUIStyle::kH1, true);
			Title->SetRenderTransformAngle(kTitleTilt);
		}
		GapAfter(Label(Box, TEXT("Director's Cut"), UTTAUIStyle::kBody)->Slot);
		MenuButton(Box, TEXT("PLAY SOLO"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnPlaySoloClicked);
		MenuButton(Box, TEXT("HOST GAME"), ETTAButtonKind::Primary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHostClicked);
		MenuButton(Box, TEXT("JOIN (CODE OR IP)"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnJoinClicked);
		MenuButton(Box, TEXT("SETTINGS"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Box, TEXT("QUIT"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnQuitClicked);
	}
	// Join: one centred panel, heading to Back.
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		JoinCard = Card(Box, tta::kBone);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		JoinCard->SetContent(Inner);
		Label(Inner, TEXT("JOIN A GAME"), UTTAUIStyle::kH2);
		Label(Inner, TEXT("Ask the host for the code"), UTTAUIStyle::kBody);
		CodeBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		CodeBox->SetHintText(FText::FromString(TEXT("ABCDEF")));
		CodeBox->OnTextCommitted.AddDynamic(this, &UTTAMenuWidget::OnCodeCommitted);
		Inner->AddChild(CodeBox);
		JoinError = Label(Inner, TEXT(""), UTTAUIStyle::kSmall);
		IpBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		IpBox->SetHintText(FText::FromString(TEXT("127.0.0.1:7777")));
		Inner->AddChild(IpBox);
		MenuButton(Inner, TEXT("DIRECT IP"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnDirectClicked);
		MenuButton(Inner, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Lobby
	{
		// mock-lobby.png: code pill top centre, up to 4 player cards, Customize bottom-left, Start bottom-right.
		UVerticalBox* Box = ScreenPanel(kWideW);
		UBorder* Pill = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::CodePill);
		RoomCodeText = UTTAUIStyle::MakeLabel(WidgetTree, FText::FromString(TEXT("------")), ETTAFont::H1);
		Pill->SetContent(RoomCodeText);
		UVerticalBoxSlot* PillSlot = Box->AddChildToVerticalBox(Pill);
		PillSlot->SetHorizontalAlignment(HAlign_Center);
		GapAfter(PillSlot);
		UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		const float CardW = (kWideW - (kMaxLobbyCards - 1) * kGap) / kMaxLobbyCards;
		for (int32 i = 0; i < kMaxLobbyCards; ++i)
		{
			UBorder* PlayerCard = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
			UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
			PlayerCard->SetContent(Inner);
			UBorder* Avatar = WidgetTree->ConstructWidget<UBorder>();
			Avatar->SetBrush(FSlateRoundedBoxBrush(UTTAUIStyle::Color(ETTAColor::Bone), kCorner));
			USizeBox* AvatarSize = WidgetTree->ConstructWidget<USizeBox>();
			AvatarSize->SetHeightOverride(CardW * 0.6f);
			AvatarSize->SetContent(Avatar);
			GapAfter(Inner->AddChild(AvatarSize));
			UTextBlock* Name = UTTAUIStyle::MakeLabel(WidgetTree, FText::GetEmpty(), ETTAFont::Body);
			GapAfter(Inner->AddChild(Name));
			UImage* Tick = WidgetTree->ConstructWidget<UImage>();
			Tick->SetBrush(UTTAUIStyle::Brush(ETTAAsset::IconReady));
			Tick->SetDesiredSizeOverride(FVector2D(UTTAUIStyle::kH2));
			Inner->AddChildToVerticalBox(Tick)->SetHorizontalAlignment(HAlign_Center);
			USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
			CardSize->SetWidthOverride(CardW);
			CardSize->SetContent(PlayerCard);
			UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(CardSize);
			if (i + 1 < kMaxLobbyCards) GapAfter(CardSlot);
			PlayerCards.Add(PlayerCard);
			CardAvatars.Add(Avatar);
			CardNames.Add(Name);
			CardReady.Add(Tick);
		}
		GapAfter(Box->AddChild(CardRow));
		WaitingText = Label(Box, TEXT(""), UTTAUIStyle::kBody);
		// Customize opens the hat and face steppers under the cards.
		CustomizePanel = WidgetTree->ConstructWidget<UVerticalBox>();
		CustomizePanel->SetVisibility(ESlateVisibility::Collapsed);
		MenuButton(CustomizePanel, TEXT("HAT >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHatNext);
		MenuButton(CustomizePanel, TEXT("FACE >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnFaceNext);
		GapAfter(Box->AddChild(CustomizePanel));
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
		MenuButton(Bottom, TEXT("CUSTOMIZE"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnCustomizeClicked);
		MenuButton(Bottom, TEXT("LEAVE"), ETTAButtonKind::Secondary)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		StartButton = MenuButton(Bottom, TEXT("START"), ETTAButtonKind::Primary);
		StartButton->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnStartClicked);
		UHorizontalBoxSlot* StartSlot = Cast<UHorizontalBoxSlot>(StartButton->GetParent()->Slot);
		StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		StartSlot->SetHorizontalAlignment(HAlign_Right);
		Box->AddChild(Bottom);
	}
	// Settings
	// Fills the safe area. Pictures on the right, sliders on the left, BACK pinned under both.
	{
		UVerticalBox* Box = ScreenPanel(kScreenW, HAlign_Fill, VAlign_Fill);
		if (USizeBox* Frame = Cast<USizeBox>(Box->GetParent())) Frame->ClearWidthOverride();
		Label(Box, TEXT("SETTINGS"), UTTAUIStyle::kH2, true);
		UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* BodySlot = Box->AddChildToVerticalBox(Body);
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		UHorizontalBoxSlot* Left = Body->AddChildToHorizontalBox(Scroll);
		Left->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Left->SetVerticalAlignment(VAlign_Fill);
		Left->SetPadding(UTTAUIStyle::PadAfter(Orient_Horizontal, kGap));
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		Scroll->AddChild(Inner);
		UVerticalBox* Looks = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* Right = Body->AddChildToHorizontalBox(Looks);
		Right->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Right->SetVerticalAlignment(VAlign_Fill);
		UUniformGridPanel* Hats = WidgetTree->ConstructWidget<UUniformGridPanel>();
		UUniformGridPanel* Faces = WidgetTree->ConstructWidget<UUniformGridPanel>();
		GapAfter(Looks->AddChild(Hats));
		Looks->AddChild(Faces);
		BuildLookGrid(Hats, true);
		BuildLookGrid(Faces, false);
		WindowModeBox = SettingCombo(Inner, TEXT("Display"));
		for (const char* Name : tta::kWindowModeNames) WindowModeBox->AddOption(UTF8_TO_TCHAR(Name));
		WindowModeBox->OnSelectionChanged.AddDynamic(this, &UTTAMenuWidget::OnWindowModeChanged);
		ResolutionBox = SettingCombo(Inner, TEXT("Resolution"));
		ResolutionBox->OnSelectionChanged.AddDynamic(this, &UTTAMenuWidget::OnResolutionChanged);
		SettingSlider(Inner, TEXT("Master"), Settings.master / kVolumeSliderMax)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMasterChanged);
		SettingSlider(Inner, TEXT("Music"), Settings.music / kVolumeSliderMax)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMusicChanged);
		SettingSlider(Inner, TEXT("Voice"), Settings.vo / kVolumeSliderMax)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnVoChanged);
		SettingSlider(Inner, TEXT("SFX"), Settings.sfx / kVolumeSliderMax)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSfxChanged);
		SettingSlider(Inner, TEXT("Sensitivity"), Settings.sensitivity / kSensitivitySliderMax)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSensitivityChanged);
		// Video options fill in when Settings opens: Initialize must not need GameUserSettings/RHI.
		MicMeter = WidgetTree->ConstructWidget<UProgressBar>();
		MicMeter->SetFillColorAndOpacity(Palette(tta::kGrass));
		SettingRow(Inner, TEXT("Mic test"), MicMeter);
		UButton* Back = UTTAUIStyle::MakeButton(WidgetTree, FText::FromString(TEXT("BACK")), ETTAButtonKind::Secondary);
		Back->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		SettingsBackSize = WidgetTree->ConstructWidget<USizeBox>();
		SettingsBackSize->SetWidthOverride(kButtonW);
		SettingsBackSize->SetHeightOverride(kButtonH);
		SettingsBackSize->SetContent(Back);
		Box->AddChildToVerticalBox(SettingsBackSize)->SetHorizontalAlignment(HAlign_Left);
	}
	// How to play: one centred panel holding the step cards and Back.
	{
		UVerticalBox* Box = ScreenPanel(kWideW);
		UBorder* Panel = UTTAUIStyle::MakePanel(WidgetTree, ETTAPanelKind::Panel);
		Box->AddChild(Panel);
		UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(PanelBox);
		Label(PanelBox, TEXT("HOW TO PLAY"), UTTAUIStyle::kH2);
		UHorizontalBox* Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
		const int32 Fills[] = {tta::kOrange, tta::kRed, tta::kTeal, tta::kPurple};
		for (size_t i = 0; i < tta::kHowToPlay.size(); ++i)
		{
			UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
			Card(Cards, Fills[i])->SetContent(Inner);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].title), UTTAUIStyle::kBody, true);
			Label(Inner, UTF8_TO_TCHAR(tta::kHowToPlay[i].line), UTTAUIStyle::kSmall)->SetAutoWrapText(true);
		}
		GapAfter(PanelBox->AddChild(Cards));
		MenuButton(PanelBox, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}
	// Reels
	{
		UVerticalBox* Box = ScreenPanel(kScreenW);
		Label(Box, TEXT("REELS"), UTTAUIStyle::kTitle, true);
		ReelList = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChild(ReelList);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
	}

	SetPlayerCards({});
	RefreshLook();
	RefreshSettings();
	HideUnreachableTitleEntries();
	Switcher->SetActiveWidgetIndex(SlotOf(Flow.current()));
}

void UTTAMenuWidget::HideUnreachableTitleEntries()
{
	USizeBox* Screen = Cast<USizeBox>(Switcher->GetChildAt(static_cast<int32>(tta::MenuScreen::Title)));
	UVerticalBox* Title = Cast<UVerticalBox>(Screen ? Screen->GetContent() : nullptr);
	if (!Title) return;
	for (int32 i = 0; i < Title->GetChildrenCount(); ++i)
	{
		USizeBox* ButtonBox = Cast<USizeBox>(Title->GetChildAt(i));
		UButton* Button = Cast<UButton>(ButtonBox ? ButtonBox->GetContent() : nullptr);
		UTextBlock* Text = Cast<UTextBlock>(Button ? Button->GetContent() : nullptr);
		if (!Text) continue;
		const FString Label = Text->GetText().ToString();
		if (Label == TEXT("PLAY SOLO")) Text->SetText(FText::FromString(TEXT("PLAY")));
		if (Label == TEXT("HOST GAME") || Label.StartsWith(TEXT("JOIN")))
		{
			ButtonBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
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
	if (bTravelStarted) return;
	bTravelStarted = true;
	// One painted frame of LOADING, then travel. The movie-player screen covers the map load itself.
	if (UOverlay* Root = Cast<UOverlay>(WidgetTree ? WidgetTree->RootWidget : nullptr))
	{
		UBorder* Veil = WidgetTree->ConstructWidget<UBorder>();
		Veil->SetBrushColor(Palette(tta::kInk));
		Veil->SetHorizontalAlignment(HAlign_Center);
		Veil->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Word = WidgetTree->ConstructWidget<UTextBlock>();
		Word->SetText(FText::FromString(TEXT("LOADING")));
		Word->SetFont(UTTAUIStyle::Font(ETTAFont::H1));
		Word->SetColorAndOpacity(FSlateColor(UTTAUIStyle::Color(ETTAColor::Bone)));
		Veil->SetContent(Word);
		UOverlaySlot* VeilSlot = Root->AddChildToOverlay(Veil);
		VeilSlot->SetHorizontalAlignment(HAlign_Fill);
		VeilSlot->SetVerticalAlignment(VAlign_Fill);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bHost = true;
			OnPlaySolo.ExecuteIfBound();
		}));
		return;
	}
	bHost = true;
	OnPlaySolo.ExecuteIfBound();
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

void UTTAMenuWidget::SetPlayerCards(const TArray<FTTALobbyPlayer>& InPlayers)
{
	int32 Ready = 0;
	for (int32 i = 0; i < kMaxLobbyCards; ++i)
	{
		const bool bShown = InPlayers.IsValidIndex(i);
		PlayerCards[i]->SetVisibility(bShown ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		if (!bShown) continue;
		CardAvatars[i]->SetBrushColor(InPlayers[i].Colour);
		CardNames[i]->SetText(FText::FromString(InPlayers[i].Name));
		CardReady[i]->SetVisibility(InPlayers[i].bReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	for (const FTTALobbyPlayer& Player : InPlayers) Ready += Player.bReady ? 1 : 0;
	SetPlayers(InPlayers.Num(), Ready);
}

int32 UTTAMenuWidget::NumVisibleCards() const
{
	int32 Count = 0;
	for (const UBorder* PlayerCard : PlayerCards) Count += PlayerCard->GetVisibility() == ESlateVisibility::Visible ? 1 : 0;
	return Count;
}

bool UTTAMenuWidget::IsStartEnabled() const
{
	return StartButton->GetIsEnabled();
}

bool UTTAMenuWidget::IsCustomizeOpen() const
{
	return CustomizePanel->GetVisibility() != ESlateVisibility::Collapsed;
}

void UTTAMenuWidget::OnCustomizeClicked()
{
	CustomizePanel->SetVisibility(IsCustomizeOpen() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
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

void UTTAMenuWidget::PickLook(bool bHat, int32 Index)
{
	const int32 Count = bHat ? static_cast<int32>(tta::kHats.size()) : static_cast<int32>(tta::kFaces.size());
	if (Index < 0 || Index >= Count) return;
	if (bHat) Look.hat = Index;
	else Look.face = Index;
	RefreshLook();
	if (ATTAPlayerState* State = GetOwningPlayerState<ATTAPlayerState>()) State->ServerSetLook(Look.colour, Look.hat, Look.face);
}

void UTTALookButton::HandleClick()
{
	if (Menu) Menu->PickLook(bHat, Index);
}

FLinearColor UTTAMenuWidget::GetLookColour() const
{
	return Palette(tta::kPlayerColours[Look.colour]);
}

void UTTAMenuWidget::BuildLookGrid(UUniformGridPanel* Grid, bool bHat)
{
	const int32 Count = bHat ? static_cast<int32>(tta::kHats.size()) : static_cast<int32>(tta::kFaces.size());
	constexpr int32 Columns = 4;
	for (int32 i = 0; i < Count; ++i)
	{
		UTexture2D* Tex = bHat ? TTALoadHatIcon(tta::kHats[i]) : TTALoadFaceTexture(tta::kFaces[i]);
		if (Tex) LookTextures.Add(Tex);
		UTTALookButton* Button = WidgetTree->ConstructWidget<UTTALookButton>();
		Button->Index = i;
		Button->bHat = bHat;
		Button->Menu = this;
		Button->OnClicked.AddDynamic(Button, &UTTALookButton::HandleClick);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		if (Tex) Image->SetBrushFromTexture(Tex, false);
		Button->AddChild(Image);
		USizeBox* Tile = WidgetTree->ConstructWidget<USizeBox>();
		Tile->SetWidthOverride(96.f);
		Tile->SetHeightOverride(96.f);
		Tile->SetContent(Button);
		Grid->AddChildToUniformGrid(Tile, i / Columns, i % Columns);
		LookTiles.Add(Tile);
		(bHat ? HatPicks : FacePicks).Add(Button);
	}
}

void UTTAMenuWidget::ApplyPickStyle(UButton* Button, bool bSelected)
{
	if (!Button) return;
	const FMargin Pad(LookPad);
	FButtonStyle Style;
	Style.SetNormal(UTTAUIStyle::Brush(bSelected ? ETTAAsset::ButtonPrimary : ETTAAsset::Button));
	Style.SetHovered(UTTAUIStyle::Brush(bSelected ? ETTAAsset::ButtonPrimary : ETTAAsset::ButtonHover));
	Style.SetPressed(UTTAUIStyle::Brush(ETTAAsset::ButtonPressed));
	Style.SetNormalPadding(Pad);
	Style.SetPressedPadding(Pad);
	Button->SetStyle(Style);
}

void UTTAMenuWidget::ApplyRelativeLayout(FVector2D Size)
{
	if (Size.X < 8.f || Size.Y < 8.f || LookTiles.Num() == 0) return;
	const FMargin Safe = UTTAUIStyle::Get().SafeArea;
	const float InnerW = FMath::Max(320.f, Size.X - Safe.Left - Safe.Right);
	const float InnerH = FMath::Max(240.f, Size.Y - Safe.Top - Safe.Bottom);
	const float Gap = InnerW * 0.012f;
	const float TileW = (InnerW * 0.5f - 3.f * Gap) / 4.f;
	const float TileH = (InnerH * 0.62f - 3.f * Gap) / 4.f;
	const float Tile = FMath::Max(36.f, FMath::Min(TileW, TileH));
	LookPad = Tile * 0.08f;
	for (USizeBox* TileBox : LookTiles)
	{
		TileBox->SetWidthOverride(Tile);
		TileBox->SetHeightOverride(Tile);
		// Uniform grid slots have no padding of their own; the panel pads every slot.
		if (UUniformGridPanel* Grid = Cast<UUniformGridPanel>(TileBox->GetParent()))
		{
			Grid->SetSlotPadding(FMargin(Gap * 0.5f));
		}
	}
	if (SettingsBackSize)
	{
		SettingsBackSize->SetWidthOverride(InnerW * 0.28f);
		SettingsBackSize->SetHeightOverride(FMath::Clamp(InnerH * 0.08f, 44.f, 84.f));
	}
	RefreshLook();
}

void UTTAMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const FVector2D Size = MyGeometry.GetLocalSize();
	if (!Size.Equals(CachedMenuSize, 1.f))
	{
		CachedMenuSize = Size;
		ApplyRelativeLayout(Size);
	}
}

void UTTAMenuWidget::RefreshLook()
{
	for (int32 i = 0; i < HatPicks.Num(); ++i) ApplyPickStyle(HatPicks[i], i == Look.hat);
	for (int32 i = 0; i < FacePicks.Num(); ++i) ApplyPickStyle(FacePicks[i], i == Look.face);
}

void UTTAMenuWidget::SetSettings(const tta::ClientSettings& InSettings)
{
	Settings = InSettings;
	RefreshSettings();
}

void UTTAMenuWidget::RefreshSettings()
{
	Settings = tta::clampClientSettings(Settings);
	const int32 Values[] = {Settings.master, Settings.music, Settings.vo, Settings.sfx, Settings.sensitivity};
	check(SettingSliders.Num() == UE_ARRAY_COUNT(Values) && SettingValues.Num() == UE_ARRAY_COUNT(Values));
	for (int32 i = 0; i < UE_ARRAY_COUNT(Values); ++i)
	{
		const float Max = i == UE_ARRAY_COUNT(Values) - 1 ? kSensitivitySliderMax : kVolumeSliderMax;
		SettingSliders[i]->SetValue(Values[i] / Max);
		SettingValues[i]->SetText(FText::AsNumber(Values[i]));
	}
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
		MenuButton(ReelList, FPaths::GetBaseFilename(Path), tta::kTeal);
	}
	if (Reels.IsEmpty()) Label(ReelList, TEXT("No reels yet - go make a film!"), UTTAUIStyle::kBody);
}
