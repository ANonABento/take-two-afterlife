#include "TTAUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "tta/MenuFlow.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAUIStyle, Log, All);

FLinearColor UTTAUIStyle::Colour(int32 PaletteIndex)
{
	check(PaletteIndex >= 0 && PaletteIndex < static_cast<int32>(tta::kPaletteHex.size()));
	return FLinearColor(FColor::FromHex(FString(tta::kPaletteHex[PaletteIndex])));
}

FSlateFontInfo UTTAUIStyle::MakeFont(UObject* Face, bool bDisplay, int32 Size)
{
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bDisplay ? "Bold" : "Regular", Size);
	if (Face)
	{
		Font.FontObject = Face;
		Font.TypefaceFontName = NAME_None;
	}
	Font.OutlineSettings.OutlineSize = bDisplay ? 3 : 0;
	Font.OutlineSettings.OutlineColor = Colour(tta::kInk);
	return Font;
}

UObject* UTTAUIStyle::LoadFont(bool bDisplay) const
{
	const FSoftObjectPath& Path = bDisplay ? DisplayFont : BodyFont;
	UObject* Font = Path.TryLoad();
	if (!Font) UE_LOG(LogTTAUIStyle, Warning, TEXT("UI font %s not imported; using the engine font"), *Path.ToString());
	return Font;
}

UFont* UTTAUIStyle::LoadCanvasFont(bool bDisplay) const
{
	if (UFont* Font = Cast<UFont>(LoadFont(bDisplay))) return Font;
	return GEngine->GetLargeFont();
}

const FSoftObjectPath& UTTAUIStyle::BackgroundPath(ETTAUIScreen Screen) const
{
	switch (Screen)
	{
	case ETTAUIScreen::Menu: return MenuBackground;
	case ETTAUIScreen::HUD: return HUDBackground;
	case ETTAUIScreen::Results: return ResultsBackground;
	case ETTAUIScreen::Podium: return PodiumBackground;
	}
	checkNoEntry();
	return MenuBackground;
}

UTexture2D* UTTAUIStyle::LoadBackground(ETTAUIScreen Screen) const
{
	return LoadSlotTexture(BackgroundPath(Screen));
}

UTexture2D* UTTAUIStyle::LoadSlotTexture(const FSoftObjectPath& Path) const
{
	if (Path.IsNull()) return nullptr;
	UTexture2D* Image = Cast<UTexture2D>(Path.TryLoad());
	if (!Image) UE_LOG(LogTTAUIStyle, Error, TEXT("UI image %s is set but is not a loadable Texture2D"), *Path.ToString());
	return Image;
}

void UTTAUIStyle::ApplyBackground(UBorder* Border, ETTAUIScreen Screen, int32 Fill, float Opacity) const
{
	if (UTexture2D* Image = LoadBackground(Screen))
	{
		Border->SetBrushFromTexture(Image);
		return;
	}
	Border->SetBrushColor(Colour(Fill).CopyWithNewOpacity(Opacity));
}

UBorder* UTTAUIStyle::AddCard(UWidgetTree* Tree, UPanelWidget* Into, int32 Fill)
{
	// Sticker card: Ink outline around a palette fill.
	UBorder* Outline = Tree->ConstructWidget<UBorder>();
	Outline->SetBrushColor(Colour(tta::kInk));
	Outline->SetPadding(FMargin(kOutline));
	UBorder* Face = Tree->ConstructWidget<UBorder>();
	Face->SetBrushColor(Colour(Fill));
	Face->SetPadding(FMargin(kCardPadding));
	Outline->SetContent(Face);
	Into->AddChild(Outline);
	return Face;
}

UTextBlock* UTTAUIStyle::AddLabel(UWidgetTree* Tree, UPanelWidget* Into, const FString& Text, int32 Size, bool bDisplay, int32 Colour)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
	Block->SetText(FText::FromString(Text));
	Block->SetFont(MakeFont(Get().LoadFont(bDisplay), bDisplay, Size));
	Block->SetColorAndOpacity(FSlateColor(UTTAUIStyle::Colour(Colour)));
	Into->AddChild(Block);
	return Block;
}
