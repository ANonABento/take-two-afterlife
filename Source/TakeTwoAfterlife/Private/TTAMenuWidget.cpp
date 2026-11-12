#include "TTAMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Misc/Paths.h"
#include "TTAReels.h"
#include "tta/JoinCode.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAMenu, Log, All);

namespace
{
// Ref sizes at 1080p: H1 120, room code 160, body 32, small 24.
constexpr int32 kH1 = 120, kCode = 160, kBody = 32, kSmall = 24;

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
	// Lilita One / Nunito ship as Runtime-cached Font assets once imported (ref); until
	// then keep the default face with the 3 px Ink outline the ref asks for.
	Font.TypefaceFontName = bDisplay ? FName("Bold") : FName("Regular");
	Font.OutlineSettings.OutlineSize = bDisplay ? 3 : 0;
	Font.OutlineSettings.OutlineColor = Palette(tta::kInk);
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(bDisplay ? Palette(tta::kBone) : Palette(tta::kInk)));
	Parent->AddChild(Block);
	return Block;
}

UButton* UTTAMenuWidget::MenuButton(UPanelWidget* Parent, const FString& Text, int32 Fill)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(Palette(Fill));
	Label(Button, Text, kBody, true);
	Parent->AddChild(Button);
	return Button;
}

USlider* UTTAMenuWidget::SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Label(Row, Name, kSmall);
	USlider* Slider = WidgetTree->ConstructWidget<USlider>();
	Slider->SetValue(Value01);
	Slider->SetSliderBarColor(Palette(tta::kInk));
	Slider->SetSliderHandleColor(Palette(tta::kBlue));
	Row->AddChild(Slider);
	Parent->AddChild(Row);
	return Slider;
}

void UTTAMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(Palette(tta::kSky));
	WidgetTree->RootWidget = Background;
	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Background->SetContent(Switcher);

	// Title
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Label(Card(Box, tta::kYellow), TEXT("TAKE TWO: AFTERLIFE"), kH1, true);
		MenuButton(Box, TEXT("HOST"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHostClicked);
		MenuButton(Box, TEXT("JOIN"), tta::kGrass)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnJoinClicked);
		MenuButton(Box, TEXT("REELS"), tta::kTeal)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnReelsClicked);
		MenuButton(Box, TEXT("SETTINGS"), tta::kPurple)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Box, TEXT("HOW TO PLAY"), tta::kPink)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHowToClicked);
		MenuButton(Box, TEXT("QUIT"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnQuitClicked);
		Switcher->AddChild(Box);
	}
	// Join
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		JoinCard = Card(Box, tta::kBone);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		JoinCard->SetContent(Inner);
		Label(Inner, TEXT("Ask the host for the code"), kBody);
		CodeBox = WidgetTree->ConstructWidget<UEditableTextBox>();
		CodeBox->SetHintText(FText::FromString(TEXT("ABCDEF")));
		CodeBox->OnTextCommitted.AddDynamic(this, &UTTAMenuWidget::OnCodeCommitted);
		Inner->AddChild(CodeBox);
		JoinError = Label(Inner, TEXT(""), kSmall);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		Switcher->AddChild(Box);
	}
	// Lobby
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		RoomCodeText = Label(Card(Box, tta::kBone), TEXT("------"), kCode, true);
		WaitingText = Label(Box, TEXT(""), kBody);
		LookSwatch = Card(Box, tta::kRed);
		LookText = Label(LookSwatch, TEXT(""), kBody, true);
		UHorizontalBox* Carousels = WidgetTree->ConstructWidget<UHorizontalBox>();
		MenuButton(Carousels, TEXT("COLOUR >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnColourNext);
		MenuButton(Carousels, TEXT("HAT >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnHatNext);
		MenuButton(Carousels, TEXT("FACE >"), tta::kBlue)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnFaceNext);
		Box->AddChild(Carousels);
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
		StartButton = MenuButton(Bottom, TEXT("START"), tta::kGrass);
		StartButton->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnStartClicked);
		MenuButton(Bottom, TEXT("SETTINGS"), tta::kPurple)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnSettingsClicked);
		MenuButton(Bottom, TEXT("LEAVE"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		Box->AddChild(Bottom);
		Switcher->AddChild(Box);
	}
	// Settings
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		UBorder* Face = Card(Box, tta::kBone);
		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>();
		Face->SetContent(Inner);
		SettingSlider(Inner, TEXT("Master"), Settings.master / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMasterChanged);
		SettingSlider(Inner, TEXT("Music"), Settings.music / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnMusicChanged);
		SettingSlider(Inner, TEXT("Voice"), Settings.vo / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnVoChanged);
		SettingSlider(Inner, TEXT("SFX"), Settings.sfx / 100.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSfxChanged);
		SettingSlider(Inner, TEXT("Mouse sensitivity"), Settings.sensitivity / 500.f)->OnValueChanged.AddDynamic(this, &UTTAMenuWidget::OnSensitivityChanged);
		Label(Inner, TEXT("Mic test"), kSmall);
		MicMeter = WidgetTree->ConstructWidget<UProgressBar>();
		MicMeter->SetFillColorAndOpacity(Palette(tta::kGrass));
		Inner->AddChild(MicMeter);
		SettingsText = Label(Inner, TEXT(""), kSmall);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		Switcher->AddChild(Box);
	}
	// How to play
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
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
		Switcher->AddChild(Box);
	}
	// Reels
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Label(Box, TEXT("REELS"), kH1, true);
		ReelList = WidgetTree->ConstructWidget<UVerticalBox>();
		Box->AddChild(ReelList);
		MenuButton(Box, TEXT("BACK"), tta::kInk)->OnClicked.AddDynamic(this, &UTTAMenuWidget::OnBackClicked);
		Switcher->AddChild(Box);
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
