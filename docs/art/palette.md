# UI style sheet

The only colours UI code may use. Same order as `Scripts/toon_spec.py` `PALETTE` and Core
`tta::kPaletteHex`; `Scripts/tests/test_ui_style.py` keeps all three equal.

| Index | Name | Hex |
|---|---|---|
| 0 | Ink | #1E1B2E |
| 1 | Bone | #F4F1E8 |
| 2 | Red | #E5484D |
| 3 | Orange | #F2913D |
| 4 | Yellow | #F5D547 |
| 5 | Grass | #6CC24A |
| 6 | Teal | #2BB3A3 |
| 7 | Sky | #5BA8F5 |
| 8 | Blue | #3F5BD9 |
| 9 | Purple | #8E5CD9 |
| 10 | Pink | #F27BB0 |
| 11 | Wood | #9A6A43 |

## Type
- Headings, numbers, buttons: **Lilita One** (`/Game/UI/Fonts/LilitaOne`), Bone fill, 3 px Ink outline.
- Body: **Nunito** (`/Game/UI/Fonts/Nunito`), Ink on light cards, Bone on dark ones.
- Text sizes: see the Fonts table in Docs/design/ui-style.md (the one type scale).

## Layout
- Design size 1920x1080. Keep everything inside the safe area: 96 px left/right, 54 px top/bottom.
- Sticker cards: 6 px Ink outline around a palette fill, 24 px padding.
- HUD: phase + round top left (Purple), timer top centre (Ink, Red at 10 s or less), budget top right
  (Grass, Red when spent), build panel under the phase card.

## Where it lives
`UTTAUIStyle` (`Source/.../TTAUIStyle.h`), values in `Config/DefaultGame.ini`
`[/Script/TakeTwoAfterlife.TTAUIStyle]`. To swap a screen's fill for an image, set
`MenuBackground`, `HUDBackground`, `ResultsBackground` or `PodiumBackground` to a Texture2D path.
