#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "TTAUIStyle.h"
#include "tta/MenuFlow.h"
#include "TTAMenuWidget.generated.h"

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

// Front-end menus: Title (Play / Settings / Reels / Quit, angel placeholder), Play page on
// the Join slot (Host, Play Solo, code-or-IP field + Join, Back), Lobby (colour / hat / face,
// room code, START gate), Settings (audio, sensitivity, window mode, resolution, mic test),
// How to play, Reels. Screen rules live in Core tta::MenuFlow; the tree is built in C++ from
// UTTAUIStyle builders only (docs/design/ui-style.md).
// The owner binds the delegates to UTTASessionSubsystem / the cut player.
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
	// Direct IP (Play page): fires with the typed address as-is when it is not a code.
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
	// Play page JOIN: a host:port / dotted address goes to OnJoinDirect, anything else to SubmitJoinCode.
	void SubmitCodeOrAddress(const FString& Typed);

	// Lobby
	void SetRoomCode(const FString& Code, bool bIsHost);
	void SetPlayers(int32 Players, int32 Ready);
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
	UPROPERTY() TObjectPtr<UTextBlock> RoomCodeText;
	UPROPERTY() TObjectPtr<UTextBlock> WaitingText;
	UPROPERTY() TObjectPtr<UTextBlock> LookText;
	UPROPERTY() TObjectPtr<UBorder> LookSwatch;
	UPROPERTY() TObjectPtr<UButton> StartButton;
	UPROPERTY() TObjectPtr<UProgressBar> MicMeter;
	UPROPERTY() TObjectPtr<UTextBlock> SettingsText;
	UPROPERTY() TObjectPtr<UComboBoxString> WindowModeBox;
	UPROPERTY() TObjectPtr<UComboBoxString> ResolutionBox;
	UPROPERTY() TObjectPtr<UVerticalBox> ReelList;

	// Panel tinted with Fill (Bone = untinted white panel).
	UBorder* Card(UPanelWidget* Parent, ETTAColor Fill = ETTAColor::Bone);
	UTextBlock* Label(UPanelWidget* Parent, const FString& Text, ETTAFont Size, ETTAColor Colour = ETTAColor::Ink);
	UButton* MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind);
	UButton* MenuButton(UPanelWidget* Parent, const FString& Text, ETTAButtonKind Kind, float Width);
	UVerticalBox* ScreenPanel(float Width);
	void SettingRow(UPanelWidget* Parent, const FString& Name, UWidget* Control);
	USlider* SettingSlider(UPanelWidget* Parent, const FString& Name, float Value01);
	UComboBoxString* SettingCombo(UPanelWidget* Parent, const FString& Name);
	void RefreshVideo();
	void RefreshLook();
	void RefreshSettings();

	UFUNCTION() void OnHostClicked() { Host(); }
	UFUNCTION() void OnPlaySoloClicked() { PlaySolo(); }
	UFUNCTION() void OnPlayClicked() { Open(tta::MenuScreen::Join); }
	UFUNCTION() void OnJoinClicked() { SubmitCodeOrAddress(CodeBox->GetText().ToString()); }
	UFUNCTION() void OnReelsClicked() { Open(tta::MenuScreen::Reels); }
	UFUNCTION() void OnSettingsClicked() { Open(tta::MenuScreen::Settings); }
	UFUNCTION() void OnQuitClicked() { OnQuit.ExecuteIfBound(); }
	UFUNCTION() void OnBackClicked() { Back(); }
	UFUNCTION() void OnCodeCommitted(const FText& Text, ETextCommit::Type Method);
	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnColourNext() { CycleLook(tta::LookPart::Colour, 1); }
	UFUNCTION() void OnHatNext() { CycleLook(tta::LookPart::Hat, 1); }
	UFUNCTION() void OnFaceNext() { CycleLook(tta::LookPart::Face, 1); }
	UFUNCTION() void OnMasterChanged(float V) { Settings.master = FMath::RoundToInt(V * 100.f); RefreshSettings(); }
	UFUNCTION() void OnMusicChanged(float V) { Settings.music = FMath::RoundToInt(V * 100.f); RefreshSettings(); }
	UFUNCTION() void OnVoChanged(float V) { Settings.vo = FMath::RoundToInt(V * 100.f); RefreshSettings(); }
	UFUNCTION() void OnSfxChanged(float V) { Settings.sfx = FMath::RoundToInt(V * 100.f); RefreshSettings(); }
	UFUNCTION() void OnWindowModeChanged(FString Item, ESelectInfo::Type Select);
	UFUNCTION() void OnResolutionChanged(FString Item, ESelectInfo::Type Select);
	UFUNCTION() void OnSensitivityChanged(float V) { Settings.sensitivity = FMath::RoundToInt(V * 500.f); RefreshSettings(); }
};
