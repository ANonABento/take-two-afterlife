# Maps

Written from `Scripts/maps/house.py`, `sky.py` and `pirate.py`; `Scripts/tests/test_design_docs.py` checks this file still names every section and variant.

## Shared library (`Scripts/maplib/`)

Every map takes these from the library, not from its own script:

- **Grid and width**: 0.5 m grid, 8 m path width, read from `Core/include/tta/Placement.h`.
- **Budget**: build cells per layout <= kBudget (100).
- **Segment chain**: `generate(map, seed)` picks one variant per section and may mirror it; each exit pose is the next entry (<= 1 cm, <= 0.5°). Turns are multiples of 15°, steps up <= 1.5 m, total length 54–75 m (`check_layout`).
- **Kill plane**: one plane 8–12 m under the lowest floor, 10 m past the path bounds on every side; `WORLD_KILL_Z` −500 as the last catch.
- **Roles and credits**: floor/marker/dressing/gate roles and the CREDITS.md check.
- **Start and finish**: Start is 1 m into section 1 (z +0.1); the Finish gate is 1 m before the end of section 5.

## House (`house`)

**Route:** lawn → shed/drainpipe → bathroom window → counter/sink → bathtub.

**Start / finish:** Start in lawn, Finish gate in bathtub. Path length over seeds 0–99: 58.0–65.0 m.

**Kill zones:** shared kill plane, plus bathtub water (every bathtub variant); per-variant volumes below.

**Build zones:** 0.5 m grid cells per variant below, in each segment's local frame; a layout holds 11–13 cells (budget 100).

**Variants:**

| Section | Variant | Length | Exit turn | Exit rise | Kill volumes | Build cells |
|---|---|---|---|---|---|---|
| lawn | `lawn_hedge` | 12 m | 0° | +0 m | 0 | 3 |
| lawn | `lawn_sprinkler` | 13 m | 15° | +0 m | 0 | 3 |
| shed/drainpipe | `shed_roof` | 12 m | 0° | +1.5 m | 1 | 2 |
| shed/drainpipe | `drainpipe_climb` | 14 m | 30° | +1.5 m | 0 | 3 |
| bathroom window | `window_ledge` | 11 m | -15° | +1 m | 1 | 2 |
| bathroom window | `window_drop` | 12 m | 0° | -1.5 m | 0 | 2 |
| counter/sink | `counter_sink` | 11 m | 15° | +0 m | 1 | 2 |
| counter/sink | `counter_soap_slide` | 13 m | -30° | -1 m | 0 | 3 |
| bathtub | `tub_rim` | 12 m | 0° | +0 m | 1 | 2 |
| bathtub | `tub_duck_hop` | 13 m | 0° | +0.5 m | 1 | 2 |

**Asset packs (CC0):**

- [Kenney Furniture Kit](https://kenney.nl/assets/furniture-kit)
- [Kenney City Kit (Suburban)](https://kenney.nl/assets/city-kit-suburban)
- [Kenney Nature Kit](https://kenney.nl/assets/nature-kit)
- [KayKit Prototype Bits](https://kaylousberg.itch.io/prototype-bits)
- [Quaternius Ultimate Home Interior](https://quaternius.com/packs/ultimatehomeinterior.html)
- [Quaternius Furniture](https://quaternius.com/packs/furniture.html)

## Sky Construction (`sky`)

**Route:** crane jib → scaffolding → girders → hook ride → sandpit/dumpster.

**Start / finish:** Start in crane jib, Finish gate in sandpit/dumpster. Path length over seeds 0–99: 58.0–63.0 m.

**Kill zones:** shared kill plane, plus fall gaps (every hook-ride variant); per-variant volumes below.

**Build zones:** 0.5 m grid cells per variant below, in each segment's local frame; a layout holds 10–13 cells (budget 100).

**Variants:**

| Section | Variant | Length | Exit turn | Exit rise | Kill volumes | Build cells |
|---|---|---|---|---|---|---|
| crane jib | `jib_walk` | 12 m | 0° | +0 m | 1 | 2 |
| crane jib | `jib_counterweight` | 13 m | 15° | -1 m | 0 | 3 |
| scaffolding | `scaffold_ladder` | 12 m | 0° | +1.5 m | 0 | 3 |
| scaffolding | `scaffold_planks` | 11 m | -15° | +0.5 m | 1 | 2 |
| girders | `girder_beam` | 13 m | 30° | +0 m | 2 | 3 |
| girders | `girder_zigzag` | 12 m | -15° | +1 m | 1 | 2 |
| hook ride | `hook_swing` | 12 m | 0° | -1.5 m | 1 | 2 |
| hook ride | `hook_conveyor` | 13 m | 15° | -0.5 m | 1 | 2 |
| sandpit/dumpster | `sandpit` | 11 m | 0° | -1 m | 0 | 2 |
| sandpit/dumpster | `dumpster` | 12 m | 0° | +0 m | 1 | 2 |

**Asset packs (CC0):**

- [Kenney City Kit (Industrial)](https://kenney.nl/assets/city-kit-industrial)
- [Kenney City Kit (Suburban)](https://kenney.nl/assets/city-kit-suburban)
- [KayKit Prototype Bits](https://kaylousberg.itch.io/prototype-bits)
- [KayKit Restaurant Bits](https://kaylousberg.itch.io/restaurant-bits)
- [Quaternius Ultimate Modular Ruins](https://quaternius.com/packs/ultimatemodularruins.html)

## Pirate Dungeon (`pirate`)

**Route:** jail cell → torch corridor → sewer → dock planks → ship deck.

**Start / finish:** Start in jail cell, Finish gate in ship deck. Path length over seeds 0–99: 58.0–63.0 m.

**Kill zones:** shared kill plane, plus sewer water (every sewer variant); per-variant volumes below.

**Build zones:** 0.5 m grid cells per variant below, in each segment's local frame; a layout holds 10–13 cells (budget 100).

**Variants:**

| Section | Variant | Length | Exit turn | Exit rise | Kill volumes | Build cells |
|---|---|---|---|---|---|---|
| jail cell | `jail_cell_door` | 12 m | 0° | +0 m | 0 | 2 |
| jail cell | `jail_bars_squeeze` | 13 m | 15° | +0 m | 1 | 3 |
| torch corridor | `torch_stairs` | 12 m | 0° | +1.5 m | 0 | 3 |
| torch corridor | `torch_bend` | 11 m | -15° | +0.5 m | 1 | 2 |
| sewer | `sewer_ledges` | 13 m | 30° | -1 m | 1 | 3 |
| sewer | `sewer_grate_hop` | 12 m | -15° | -0.5 m | 1 | 2 |
| dock planks | `dock_planks` | 12 m | 0° | +0 m | 1 | 2 |
| dock planks | `dock_crates` | 13 m | 15° | +0.5 m | 0 | 2 |
| ship deck | `ship_gangplank` | 11 m | 0° | +1 m | 0 | 2 |
| ship deck | `ship_deck_mast` | 12 m | 0° | +0 m | 1 | 2 |

**Asset packs (CC0):**

- [Kenney Pirate Kit](https://kenney.nl/assets/pirate-kit)
- [Kenney Mini Dungeon](https://kenney.nl/assets/mini-dungeon)
- [Kenney Modular Dungeon Kit](https://kenney.nl/assets/modular-dungeon-kit)
- [KayKit Dungeon Remastered](https://github.com/KayKit-Game-Assets/KayKit-Dungeon-Remastered-1.0)
- [Quaternius Pirate Kit](https://quaternius.com/packs/piratekit.html)
- [Quaternius Modular Dungeon](https://quaternius.com/packs/modulardungeon.html)
