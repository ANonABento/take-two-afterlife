#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "tta/MenuFlow.h"
#include "TTAMenuWidget.generated.h"

class UImage;
class UBorder;
class UButton;
class UComboBoxString;
class UPanelWidget;
class UProgressBar;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetSwitcher;

DECLARE_DELEGATE(FTTAMenuAction);
DECLARE_DELEGATE_OneParam(FTTAMenuCode, const FString&);

// Front-end menus (plan task 24): Title (Play Solo / Host / Join / Settings / Quit;
// logo, background and button art come from UTTAUIStyle slots), Join (code or IP), Lobby (colour / hat / face, room
// code, START gate), Settings (audio, sensitivity, window mode, resolution, mic test,
// accessibility), How to play,
// Reels. Screen rules live in Core tta::MenuFlow; this widget builds the tree in C++ and
// skins it per choomfie ref ui-party-games (sticker cards, Ink outlines, palette colours).
// The owner binds the delegates to UTTASessionSubsystem / the cut player.
// One lobby card: avatar tint, name, ready tick.
struct FTTALobbyPlayer
{
	FString Name;
	FLinearColor Colour;
	bool bReady = false;
};

UCLASS()
class TAKETWOAFTERLIFE_API UTTAMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FTTAMenuAction OnHost;
	// Play Solo: host the Backyard offline with bots.
	FTTAMenuAction OnPlaySolo;
	FTTAMenuAction OnStart;
	FTTAMenuAction OnQuit;
	FTTAMenuCode OnJoinCode;
	// Direct-IP dev fallback (Join screen): fires with the typed address as-is.
	FTTAMenuCode OnJoinDirect;

	tta::MenuScreen GetScreen() const { return Flow.current(); }
	int32 GetVisibleIndex() const;
	bool Open(tta::MenuScreen Screen);
	bool Back();

	void Host();
	void PlaySolo();
	// Normalizes the typed code; on success fires OnJoinCode and opens the Lobby.
	// On failure the card turns Red, shows why, and the screen stays on Join.
	bool SubmitJoinCode(const FString& Typed);

	// Lobby
	void SetRoomCode(const FString& Code, bool bIsHost);
	void SetPlayers(int32 Players, int32 Ready);
	// Fills the card row (first kMaxLobbyCards players, rest hidden) and the Start state.
	void SetPlayerCards(const TArray<FTTALobbyPlayer>& InPlayers);
	int32 NumVisibleCards() const;
	bool IsStartEnabled() const;
	bool IsCustomizeOpen() const;
	static constexpr int32 kMaxLobbyCards = 4;
	bool CanStart() const { return bHost && StartBlockedReason.IsEmpty(); }
	const FString& GetStartBlockedReason() const { return StartBlockedReason; }
	void CycleLook(tta::LookPart Part, int32 Step);
	const tta::PlayerLook& GetLook() const { return Look; }
	FLinearColor GetLookColour() const;

	// Settings
	void SetSettings(const tta::ClientSettings& InSettings);
	const tta::ClientSettings& GetSettings() const { return Settings; }
	void SetMicSamples(const int16* Samples, int32 Count);
	float GetMicLevel() const { return MicLevel; }
	// Video: applied and saved through UGameUserSettings (GameUserSettings.ini). One fixed
	// graphics quality, so there are no scalability presets here.
	void ApplyWindowMode(tta::WindowMode Mode);
	bool ApplyResolution(int32 ChoiceIndex);
	int32 NumResolutionChoices() const { return static_cast<int32>(ResolutionChoices.size()); }

	// Reels: reload the gallery from TTAReels::List().
	void RefreshReels();
	int32 NumReels() const { return Reels.Num(); }

	static FLinearColor Palette(int32 Index);

protected:
	virtual void NativeOnInitialized() override;

private:
	tta::MenuFlow Flow;
	tta::PlayerLook Look;
	tta::ClientSettings Settings;
	bool bHost = false;
	FString StartBlockedReason;
	float MicLevel = 0.f;
	TArray<FString> Reels;
	std::vector<tta::Resolution> ResolutionChoices;

	UPROPERTY() TObjectPtr<UWidgetSwitcher> Switcher;
	UPROPERTY() TObjectPtr<UBorder> JoinCard;
	UPROPERTY() TObjectPtr<UEditableTextBox> CodeBox;
	UPROPERTY() TObjectPtr<UTextBlock> JoinError;
	UPROPERTY() TObjectPtr<UEditableTextBox> IpBox;
	UPROPERTY() TObjectPtr<UTextBlock> RoomCodeText;
	UPROPERTY() TObjectPtr<UTextBlock> WaitingText;
	// Card row, kMaxLobbyCards slots built once; SetPlayerCards fills and shows them.
	UPROPERTY() TArray<TObjectPtr<UBorder>> PlayerCards;
	UPROPERTY() TArray<TObjectPtr<UBorder>> CardAvatars;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CardNames;
	UPROPERTY() TArray<TObjectPtr<UImage>> CardReady;
	UPROPERTY() TObjectPtr<UVerticalBox> CustomizePanel;
	UPROPERTY() TObjectPtr<UTextBlock> LookText;
	UPROPERTY() TObjectPtr<UBorder> LookSwatch;
	UPROPERTY() TObjectPtr<UButton> StartButton;
	UPROPERTY() TObjectPtr<UProgressBar> MicMeter;
	// One per ClientSettings field, in RefreshSettings order (master, music, vo, sfx, sensitivity).
	UPROPERTY() TArray<TObjectPtr<USlider>> SettingSliders;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SettingValues;
	UPROPERTY() TObjectPtr<UComboBoxString> WindowModeBox;
	UPROPERTY() TObjectPtr<UComboBoxString> ResolutionBox;
	UPROPERTY() TObjectPtr<UVerticalBox> ReelList;
	// Lilita One (display) / Nunito (body) from /Game/UI/Fonts; null = engine font.
	UPROPERTY() TObjectPtr<UObject> DisplayFont;
	UPROPERTY() TObjectPtr<UObject> BodyFont;

	UBorder* Card(UPanelWidget* Parent, int32 Fill);
	UTextBlock* Label(UPanelWidget* Parent, const FString& Text, int32 Size, bool bDisplay = false);
	UButton* MenuButton(UPanelWidget* Parent, const FString& Text, int32 Fill);
	UButton* MenuButton(UPanelWidget* Parent, const FString& Text, int32 Fill, float Width);
	// Title stack button: style-guide art by kind, fixed kButtonW x kButtonH so the stack is one width.
	UButton* MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind);
	UVerticalBox* ScreenPanel(float Width, EHorizontalAlignment HAlign = HAlign_Center, EVerticalAlignment VAlign = VAlign_Center);
	static constexpr float kVolumeSliderMax = 100.f;      // slider 0..1 -> volume 0..100
	static constexpr float kSensitivitySliderMax = 500.f; // slider 0..1 -> sensitivity 0..500 %
	static constexpr float kSettingValueW = 96.f;         // number column right of each slider
	void SettingRow(UPanelWidget* Parent, const FString& Name, UWidget* Control);
	USlider* SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01);
	UComboBoxString* SettingCombo(UPanelWidget* Parent, const FString& Name);
	void RefreshVideo();
	void RefreshLook();
	void RefreshSettings();

	UFUNCTION() void OnHostClicked() { Host(); }
	UFUNCTION() void OnPlaySoloClicked() { PlaySolo(); }
	UFUNCTION() void OnJoinClicked() { Open(tta::MenuScreen::Join); }
	UFUNCTION() void OnSettingsClicked() { Open(tta::MenuScreen::Settings); }
	UFUNCTION() void OnQuitClicked() { OnQuit.ExecuteIfBound(); }
	UFUNCTION() void OnBackClicked() { Back(); }
	UFUNCTION() void OnCodeCommitted(const FText& Text, ETextCommit::Type Method);
	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnDirectClicked() { OnJoinDirect.ExecuteIfBound(IpBox->GetText().ToString()); }
	UFUNCTION() void OnCustomizeClicked();
	UFUNCTION() void OnColourNext() { CycleLook(tta::LookPart::Colour, 1); }
	UFUNCTION() void OnHatNext() { CycleLook(tta::LookPart::Hat, 1); }
	UFUNCTION() void OnFaceNext() { CycleLook(tta::LookPart::Face, 1); }
	UFUNCTION() void OnMasterChanged(float V) { Settings.master = FMath::RoundToInt(V * kVolumeSliderMax); RefreshSettings(); }
	UFUNCTION() void OnMusicChanged(float V) { Settings.music = FMath::RoundToInt(V * kVolumeSliderMax); RefreshSettings(); }
	UFUNCTION() void OnVoChanged(float V) { Settings.vo = FMath::RoundToInt(V * kVolumeSliderMax); RefreshSettings(); }
	UFUNCTION() void OnSfxChanged(float V) { Settings.sfx = FMath::RoundToInt(V * kVolumeSliderMax); RefreshSettings(); }
	UFUNCTION() void OnWindowModeChanged(FString Item, ESelectInfo::Type Select);
	UFUNCTION() void OnResolutionChanged(FString Item, ESelectInfo::Type Select);
	UFUNCTION() void OnSensitivityChanged(float V) { Settings.sensitivity = FMath::RoundToInt(V * kSensitivitySliderMax); RefreshSettings(); }
};
