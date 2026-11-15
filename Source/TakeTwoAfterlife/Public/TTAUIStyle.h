#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "UObject/SoftObjectPath.h"
#include "TTAUIStyle.generated.h"

class UBorder;
class UFont;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class UWidgetTree;

UENUM()
enum class ETTAUIScreen : uint8
{
	Menu,
	HUD,
	Results,
	Podium,
};

// One style for every TTA screen (docs/art/palette.md): colours come only from the
// Core palette (tta::kPaletteHex), headings Lilita One, body Nunito, 1920x1080 safe area.
// Values live in DefaultGame.ini [/Script/TakeTwoAfterlife.TTAUIStyle]: set a screen's
// *Background to a texture path to swap its palette fill for an image, no code change.
UCLASS(Config = Game)
class TAKETWOAFTERLIFE_API UTTAUIStyle : public UObject
{
	GENERATED_BODY()

public:
	// Type sizes at 1080p. One declarator per line: MSVC gives C2487 on the 2nd+ name of a
	// comma list of static members in an exported (TAKETWOAFTERLIFE_API) class.
	static constexpr int32 kH1 = 120;
	static constexpr int32 kH2 = 64;
	static constexpr int32 kTimer = 96;
	static constexpr int32 kBody = 32;
	static constexpr int32 kSmall = 24;
	static constexpr float kDesignHeight = 1080.f;
	static constexpr float kOutline = 6.f;
	static constexpr float kCardPadding = 24.f;

	UPROPERTY(Config) FSoftObjectPath DisplayFont;
	UPROPERTY(Config) FSoftObjectPath BodyFont;
	// Inset from each screen edge at 1920x1080 (5% title-safe by default).
	UPROPERTY(Config) FMargin SafeArea = FMargin(96.f, 54.f);
	// Empty = palette fill.
	UPROPERTY(Config) FSoftObjectPath MenuBackground;
	UPROPERTY(Config) FSoftObjectPath HUDBackground;
	UPROPERTY(Config) FSoftObjectPath ResultsBackground;
	UPROPERTY(Config) FSoftObjectPath PodiumBackground;
	// Art slots for the title menu. Empty = text title / palette rounded buttons.
	UPROPERTY(Config) FSoftObjectPath MenuLogo;
	UPROPERTY(Config) FSoftObjectPath ButtonNormal;
	UPROPERTY(Config) FSoftObjectPath ButtonHovered;
	UPROPERTY(Config) FSoftObjectPath ButtonPressed;

	static const UTTAUIStyle& Get() { return *GetDefault<UTTAUIStyle>(); }
	// Palette index (tta::kInk ...) to colour. The only colour source for UI code.
	static FLinearColor Colour(int32 PaletteIndex);
	static FSlateFontInfo MakeFont(UObject* Face, bool bDisplay, int32 Size);

	// Font asset or null (logged) when it is not imported yet.
	UObject* LoadFont(bool bDisplay) const;
	// Canvas (AHUD) needs a UFont; falls back to the engine large font, logged.
	UFont* LoadCanvasFont(bool bDisplay) const;
	// Configured image for Screen, null when none is set. A set path that fails to load is logged.
	UTexture2D* LoadBackground(ETTAUIScreen Screen) const;
	// Texture at an art slot (MenuLogo, Button*), null when empty. A set path that fails to load is logged.
	UTexture2D* LoadSlotTexture(const FSoftObjectPath& Path) const;
	// Image when configured, else palette Fill at Opacity.
	void ApplyBackground(UBorder* Border, ETTAUIScreen Screen, int32 Fill, float Opacity) const;

	// UMG builders shared by the TTA widgets.
	static UBorder* AddCard(UWidgetTree* Tree, UPanelWidget* Into, int32 Fill);
	static UTextBlock* AddLabel(UWidgetTree* Tree, UPanelWidget* Into, const FString& Text, int32 Size, bool bDisplay, int32 Colour);

private:
	const FSoftObjectPath& BackgroundPath(ETTAUIScreen Screen) const;
};
