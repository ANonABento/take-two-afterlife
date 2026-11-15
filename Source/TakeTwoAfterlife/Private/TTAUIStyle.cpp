#include "TTAUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "tta/MenuFlow.h"

DEFINE_LOG_CATEGORY_STATIC(LogTTAUIStyle, Log, All);

namespace
{
const TCHAR* const kGeneratedDir = TEXT("/Game/UI/Generated/");
const TCHAR* const kAngelTexture = TEXT("/Game/Characters/Angel/Face.Face");
constexpr float kSpaceScale[] = {0.f, 4.f, 8.f, 16.f, 24.f, 32.f};

// Written by Scripts/build_ui_chrome.py: measured 9-slice margins in px per texture.
const TCHAR* const kSlicesFile = TEXT("Art/UI/T_UI_Slices.json");

// Fallback colour and whether the asset is 9-sliced (margins come from kSlicesFile).
struct FAssetSpec
{
	ETTAAsset Asset;
	ETTAColor Fallback;
	bool bSliced;
};
const FAssetSpec kAssets[] = {
	{ETTAAsset::Button, ETTAColor::Bone, true},
	{ETTAAsset::ButtonHover, ETTAColor::Sky, true},
	{ETTAAsset::ButtonPressed, ETTAColor::Blue, true},
	{ETTAAsset::ButtonPrimary, ETTAColor::Yellow, true},
	{ETTAAsset::Panel, ETTAColor::Bone, true},
	{ETTAAsset::CodePill, ETTAColor::Yellow, true},
	{ETTAAsset::Banner, ETTAColor::Bone, true},
	{ETTAAsset::SliderBar, ETTAColor::Sky, true},
	{ETTAAsset::SliderKnob, ETTAColor::Yellow, false},
	{ETTAAsset::TextField, ETTAColor::Bone, true},
	{ETTAAsset::IconMicOn, ETTAColor::Sky, false},
	{ETTAAsset::IconMicOff, ETTAColor::Ink, false},
	{ETTAAsset::IconCamFixed, ETTAColor::Sky, false},
	{ETTAAsset::IconCamPan, ETTAColor::Sky, false},
	{ETTAAsset::IconCamFollow, ETTAColor::Sky, false},
	{ETTAAsset::IconCamTrap, ETTAColor::Orange, false},
	{ETTAAsset::IconReady, ETTAColor::Grass, false},
	{ETTAAsset::TitleBackdrop, ETTAColor::Sky, false},
};

const FAssetSpec& Spec(ETTAAsset Asset)
{
	for (const FAssetSpec& S : kAssets)
	{
		if (S.Asset == Asset) return S;
	}
	checkNoEntry();
	return kAssets[0];
}

// Texture name -> slice margins as UV fractions, loaded once from kSlicesFile.
const TMap<FString, FMargin>& SliceMargins()
{
	static const TMap<FString, FMargin> Margins = []
	{
		TMap<FString, FMargin> Out;
		const FString File = FPaths::ProjectDir() / kSlicesFile;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		const TSharedPtr<FJsonObject>* Assets = nullptr;
		if (!FFileHelper::LoadFileToString(Text, *File) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root)
			|| !Root.IsValid() || !Root->TryGetObjectField(TEXT("assets"), Assets))
		{
			UE_LOG(LogTTAUIStyle, Error, TEXT("Cannot read slice margins from %s; run Scripts/build_ui_chrome.py"), *File);
			return Out;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Assets)->Values)
		{
			const TSharedPtr<FJsonObject> A = Pair.Value->AsObject();
			const double W = A->GetNumberField(TEXT("width"));
			const double H = A->GetNumberField(TEXT("height"));
			Out.Add(Pair.Key, FMargin(A->GetNumberField(TEXT("left")) / W, A->GetNumberField(TEXT("top")) / H,
				A->GetNumberField(TEXT("right")) / W, A->GetNumberField(TEXT("bottom")) / H));
		}
		return Out;
	}();
	return Margins;
}
}

static_assert(static_cast<size_t>(ETTAColor::Wood) + 1 == tta::kPaletteHex.size(), "ETTAColor must list every palette entry");

FLinearColor UTTAUIStyle::Color(ETTAColor Name)
{
	return Colour(static_cast<int32>(Name));
}

FSlateFontInfo UTTAUIStyle::Font(ETTAFont Name)
{
	switch (Name)
	{
	case ETTAFont::Title: return MakeFont(Get().LoadFont(true), true, kTitle);
	case ETTAFont::H1: return MakeFont(Get().LoadFont(true), true, kH1);
	case ETTAFont::H2: return MakeFont(Get().LoadFont(true), true, kH2);
	case ETTAFont::Body: return MakeFont(Get().LoadFont(false), false, kBody);
	case ETTAFont::Small: return MakeFont(Get().LoadFont(false), false, kSmall);
	}
	checkNoEntry();
	return MakeFont(nullptr, false, kBody);
}

float UTTAUIStyle::Space(int32 Step)
{
	check(Step >= 0 && Step < static_cast<int32>(UE_ARRAY_COUNT(kSpaceScale)));
	return kSpaceScale[Step];
}

FMargin UTTAUIStyle::PadAfter(EOrientation Box, float Gap)
{
	FMargin Pad;
	(Box == Orient_Vertical ? Pad.Bottom : Pad.Right) = Gap;
	return Pad;
}

FSlateBrush UTTAUIStyle::Brush(ETTAAsset Asset)
{
	const FAssetSpec& S = Spec(Asset);
	const FString Name = FString(TEXT("T_UI_")) + StaticEnum<ETTAAsset>()->GetNameStringByValue(static_cast<int64>(Asset));
	const FSoftObjectPath Path(FString(kGeneratedDir) + Name + TEXT(".") + Name);
	FSlateBrush Result;
	if (UTexture2D* Texture = Cast<UTexture2D>(Path.TryLoad()))
	{
		Result.SetResourceObject(Texture);
		Result.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Result.DrawAs = ESlateBrushDrawType::Image;
		if (S.bSliced)
		{
			if (const FMargin* Margin = SliceMargins().Find(Name))
			{
				Result.DrawAs = ESlateBrushDrawType::Box;
				Result.Margin = *Margin;
			}
			else
			{
				UE_LOG(LogTTAUIStyle, Error, TEXT("%s has no slice margins in %s; drawn unsliced"), *Name, kSlicesFile);
			}
		}
		return Result;
	}
	UE_LOG(LogTTAUIStyle, Warning, TEXT("UI texture %s not imported; using a flat palette colour"), *Path.ToString());
	Result.DrawAs = ESlateBrushDrawType::Box;
	Result.TintColor = FSlateColor(Color(S.Fallback));
	return Result;
}

UTextBlock* UTTAUIStyle::MakeLabel(UWidgetTree* Tree, const FText& Text, ETTAFont Size, ETTAColor Colour)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
	Block->SetText(Text);
	Block->SetFont(Font(Size));
	Block->SetColorAndOpacity(FSlateColor(Color(Colour)));
	return Block;
}

UButton* UTTAUIStyle::MakeButton(UWidgetTree* Tree, const FText& Text, ETTAButtonKind Kind)
{
	const bool bPrimary = Kind == ETTAButtonKind::Primary;
	const FMargin Padding(Space(3), Space(2));
	FButtonStyle Style;
	Style.SetNormal(Brush(bPrimary ? ETTAAsset::ButtonPrimary : ETTAAsset::Button));
	Style.SetHovered(Brush(bPrimary ? ETTAAsset::ButtonPrimary : ETTAAsset::ButtonHover));
	Style.SetPressed(Brush(ETTAAsset::ButtonPressed));
	Style.SetNormalPadding(Padding);
	Style.SetPressedPadding(Padding);
	UButton* Button = Tree->ConstructWidget<UButton>();
	Button->SetStyle(Style);
	Button->AddChild(MakeLabel(Tree, Text, ETTAFont::Body));
	return Button;
}

UBorder* UTTAUIStyle::MakePanel(UWidgetTree* Tree, ETTAPanelKind Kind)
{
	const bool bPill = Kind == ETTAPanelKind::CodePill;
	UBorder* Panel = Tree->ConstructWidget<UBorder>();
	Panel->SetBrush(Brush(bPill ? ETTAAsset::CodePill : ETTAAsset::Panel));
	Panel->SetPadding(bPill ? FMargin(Space(4), Space(2)) : FMargin(Space(4)));
	return Panel;
}

UHorizontalBox* UTTAUIStyle::MakeIconCount(UWidgetTree* Tree, ETTAAsset Icon, const FText& Text)
{
	UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
	UImage* Image = Tree->ConstructWidget<UImage>();
	Image->SetBrush(Brush(Icon));
	Image->SetDesiredSizeOverride(FVector2D(kBody + Space(2)));
	Row->AddChildToHorizontalBox(Image)->SetPadding(PadAfter(Orient_Horizontal, Space(2)));
	Row->AddChildToHorizontalBox(MakeLabel(Tree, Text, ETTAFont::Body))->SetVerticalAlignment(VAlign_Center);
	return Row;
}

UBorder* UTTAUIStyle::MakeBanner(UWidgetTree* Tree, const FText& Text)
{
	UBorder* Banner = Tree->ConstructWidget<UBorder>();
	Banner->SetBrush(Brush(ETTAAsset::Banner));
	Banner->SetPadding(FMargin(Space(5), Space(2)));
	Banner->SetHorizontalAlignment(HAlign_Center);
	Banner->SetContent(MakeLabel(Tree, Text, ETTAFont::H2));
	return Banner;
}

USlider* UTTAUIStyle::MakeSlider(UWidgetTree* Tree)
{
	FSliderStyle Style;
	Style.SetNormalBarImage(Brush(ETTAAsset::SliderBar));
	Style.SetHoveredBarImage(Brush(ETTAAsset::SliderBar));
	Style.SetNormalThumbImage(Brush(ETTAAsset::SliderKnob));
	Style.SetHoveredThumbImage(Brush(ETTAAsset::SliderKnob));
	USlider* Slider = Tree->ConstructWidget<USlider>();
	Slider->SetWidgetStyle(Style);
	return Slider;
}

UEditableTextBox* UTTAUIStyle::MakeTextField(UWidgetTree* Tree)
{
	UEditableTextBox* Field = Tree->ConstructWidget<UEditableTextBox>();
	// Set before the Slate widget is built (first paint), so no setter is needed.
	Field->WidgetStyle.SetBackgroundImageNormal(Brush(ETTAAsset::TextField));
	Field->WidgetStyle.SetBackgroundImageHovered(Brush(ETTAAsset::TextField));
	Field->WidgetStyle.SetBackgroundImageFocused(Brush(ETTAAsset::TextField));
	Field->WidgetStyle.SetPadding(FMargin(Space(3), Space(2)));
	Field->WidgetStyle.TextStyle.SetFont(Font(ETTAFont::Body));
	Field->WidgetStyle.SetForegroundColor(FSlateColor(Color(ETTAColor::Ink)));
	return Field;
}

UImage* UTTAUIStyle::MakeAngel(UWidgetTree* Tree)
{
	UImage* Image = Tree->ConstructWidget<UImage>();
	if (UTexture2D* Texture = Cast<UTexture2D>(FSoftObjectPath(kAngelTexture).TryLoad()))
	{
		Image->SetBrushFromTexture(Texture, true);
		return Image;
	}
	UE_LOG(LogTTAUIStyle, Warning, TEXT("Angel texture %s missing or not a texture; using the panel brush"), kAngelTexture);
	Image->SetBrush(Brush(ETTAAsset::Panel));
	return Image;
}

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
