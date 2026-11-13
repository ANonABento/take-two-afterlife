# Take Two Afterlife — unified palette

Source of truth: `Scripts/toon_spec.py` → `PALETTE`, baked into `T_TTA_Palette`. Every mesh slot (Kenney, KayKit, SK_Angel, Heaven, hats) uses an `MI_TTA_Toon_Pal_<Name>` instance of `M_TTA_Toon`. Do not use any other colour.

| Name | Hex | Use |
|---|---|---|
| Ink | #1E1B2E | outlines, darkest shadow, screen bezel |
| Bone | #F4F1E8 | clouds, angel robe, highlights |
| Red | #E5484D | accents, warnings |
| Orange | #F2913D | warm props, hats |
| Yellow | #F5D547 | halo, sun |
| Grass | #6CC24A | backyard foliage |
| Teal | #2BB3A3 | water, secondary accents |
| Sky | #5BA8F5 | sky, Heaven backdrop |
| Blue | #3F5BD9 | cool accents, screen glow |
| Purple | #8E5CD9 | Heaven couch |
| Pink | #F27BB0 | angel cheeks, UI accents |
| Wood | #9A6A43 | fences, furniture legs, trunks |

World look (`MPC_TTA_World`): Saturation, Contrast, ShadowFloor, Bands, RimStrength, Warmth — shared by both maps. Outline: one post-process material, `M_TTA_PP_Outline`, so thickness is identical across all kits.
