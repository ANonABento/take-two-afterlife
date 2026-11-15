# UI style guide

Single reference for every screen (plan.md addendum R-S1..R-S3). Code: `UTTAUIStyle` (Source/TakeTwoAfterlife/Public/TTAUIStyle.h).
`Scripts/tests/test_ui_style_guide.py` checks this file against the code and `Art/UI/` both ways, so add a token or asset here and in TTAUIStyle together.

## Colours

`UTTAUIStyle::Color(ETTAColor)`. Same order and hex as the Core palette (`tta::kPaletteHex`, docs/art/palette.md).

| Token | Hex |
|---|---|
| `Ink` | #1E1B2E |
| `Bone` | #F4F1E8 |
| `Red` | #E5484D |
| `Orange` | #F2913D |
| `Yellow` | #F5D547 |
| `Grass` | #6CC24A |
| `Teal` | #2BB3A3 |
| `Sky` | #5BA8F5 |
| `Blue` | #3F5BD9 |
| `Purple` | #8E5CD9 |
| `Pink` | #F27BB0 |
| `Wood` | #9A6A43 |

## Fonts

`UTTAUIStyle::Font(ETTAFont)`. Sizes in px at 1080p. Display face Lilita One (3 px Ink outline), body face Nunito.
This is the only type scale: each row is `UTTAUIStyle::k<Token>` (Canvas HUD text uses the same constants). The countdown timer uses `H1`.

| Token | Size | Face |
|---|---|---|
| `Title` | 120 | Lilita One |
| `H1` | 96 | Lilita One |
| `H2` | 64 | Lilita One |
| `Body` | 32 | Nunito |
| `Small` | 24 | Nunito |

## Spacing

`UTTAUIStyle::Space(Step)`, px at 1080p. Slot gaps use `PadAfter(Orientation, Gap)`.

| Token | px |
|---|---|
| `Space(0)` | 0 |
| `Space(1)` | 4 |
| `Space(2)` | 8 |
| `Space(3)` | 16 |
| `Space(4)` | 24 |
| `Space(5)` | 32 |

Button padding: `Space(3)` x `Space(2)`. Panel padding: `Space(4)`. Code pill padding: `Space(4)` x `Space(2)`. Banner padding: `Space(5)` x `Space(2)`. Text field padding: `Space(3)` x `Space(2)`. Button gap in a row: `Space(1)`.

Corner radius: baked into the generated art (no code-drawn corners). Brushes keep the texture's pixel size as `ImageSize`.

## Assets

Source PNGs (with alpha) live in `Art/UI/`; `Scripts/import_ui_assets.py` imports them to `/Game/UI/Generated/` as UserInterface2D, no mips. `UTTAUIStyle::Brush(ETTAAsset)` loads `T_UI_<Name>`; when it fails to load, the brush is the fallback palette colour and a warning is logged.
Chrome (buttons, panel, pill, banner, text field, slider) is drawn by `Scripts/build_ui_chrome.py`: rounded rectangle, palette fill, 6 px Ink outline, 4 px Ink shadow below; all button states are 512x160. Sliced assets take their 9-slice margins from `Art/UI/T_UI_Slices.json`, written by the same script (corner radius + outline on each side, plus the shadow at the bottom). Unsliced assets are drawn whole.

| Asset | Sliced | Fallback | Use |
|---|---|---|---|
| `T_UI_Button` | yes | Bone | Secondary button, normal |
| `T_UI_ButtonHover` | yes | Sky | Secondary button, hovered |
| `T_UI_ButtonPressed` | yes | Blue | Any button, pressed |
| `T_UI_ButtonPrimary` | yes | Yellow | Primary (gold) button, normal and hovered |
| `T_UI_Panel` | yes | Bone | White rounded panel |
| `T_UI_CodePill` | yes | Yellow | Gold pill behind the lobby code |
| `T_UI_Banner` | yes | Bone | Timer banner |
| `T_UI_SliderBar` | yes | Sky | Slider track |
| `T_UI_SliderKnob` | no | Yellow | Slider thumb |
| `T_UI_TextField` | yes | Bone | Text field frame |
| `T_UI_IconMicOn` | no | Sky | Voice on |
| `T_UI_IconMicOff` | no | Ink | Voice muted |
| `T_UI_IconCamFixed` | no | Sky | Fixed camera |
| `T_UI_IconCamPan` | no | Sky | Panning camera |
| `T_UI_IconCamFollow` | no | Sky | Follow camera |
| `T_UI_IconCamTrap` | no | Orange | Trap camera |
| `T_UI_IconReady` | no | Grass | Ready tick |
| `T_UI_TitleBackdrop` | no | Sky | Title screen backdrop |

## Builders

All take the widget tree first; the caller adds the result to its parent.

| Builder | Makes |
|---|---|
| `Color` | Palette colour for a token |
| `Font` | Font for a type token |
| `Space` | Spacing step in px |
| `PadAfter` | Trailing-side slot margin |
| `Brush` | 9-slice brush for an asset, flat fallback |
| `MakeButton` | Button with a live text label, by `ETTAButtonKind` |
| `MakePanel` | Border with the panel or code-pill brush, by `ETTAPanelKind` |
| `MakeLabel` | Text block in a font and colour token (default Ink) |
| `MakeIconCount` | Icon + count label row |
| `MakeBanner` | Timer banner whose content is an H2 label |
| `MakeSlider` | Slider with bar + knob art |
| `MakeAngel` | Image of the angel placeholder (`/Game/Characters/Angel/Face`), panel brush fallback |
| `MakeTextField` | Editable text box with frame art, Body font |

## Component rules

| Kind | Rule |
|---|---|
| `Primary` | Gold button (`T_UI_ButtonPrimary`). One per screen: the main action. |
| `Secondary` | White button (`T_UI_Button`, hover `T_UI_ButtonHover`). Every other action. |
| `Panel` | White rounded panel (`T_UI_Panel`) behind grouped controls. |
| `CodePill` | Gold pill (`T_UI_CodePill`) for the lobby / join code only. |

- Banner = timer banner (`MakeBanner`), for the phase timer only.
- Text is always live UMG text, never baked into art, so it stays translatable.
- Widget .cpp files outside TTAUIStyle.cpp never contain `FLinearColor(`, `FSlateFontInfo(`, a numeric `FMargin(` or a texture path. Need something new? Add a token or builder to TTAUIStyle and a row here first.
