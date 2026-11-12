# Take Two Afterlife — Dev Log

Built for the Epic AI MegaJam 2026 (theme: Director's Cut). Each entry: the prompt, what the agent did, what failed, what was fixed by hand.

## Task 4 — Core scaffold + state machine
- **Prompt:** engine-free C++17 `Core/` with CMake + doctest, root `make test`, match state machine (Lobby → Build → Run → Edit → Premiere → next round or Podium) with rounds/timers from settings JSON.
- **Agent did:** added `Core/` (library + doctest tests), vendored doctest 2.4.11, a small flat-JSON settings parser, the state machine, and the `Makefile`.
- **Failed:** first build — doctest can't print `enum class Phase`; fixed with a `StringMaker<Phase>` in the tests. The sandbox also needed `pip install cmake`.
- **Fixed by hand:** nothing yet.

## Task 5 — Placement + budget model
- **Prompt:** trap catalogue, 0.5 m snap, 45° rotation, corridor bounds, budget 100, camera cap 12, hidden flag, JSON save/load.
- **Agent did:** added `Placement.h/.cpp` and doctest cases for snap, bounds, over-budget, camera cap, settings, hiding and JSON round-trip.
- **Failed:** two bounds tests used points that snap back inside the corridor (60.2 → 60.0); moved them further out.
- **Fixed by hand:** nothing yet.

## Task 6 — Deterministic trap timelines
- **Prompt:** pure `pose(trapType, settings, t)` for all 6 traps; `make test` with determinism and period tests per trap.
- **Agent did:** added `TrapTimeline.h/.cpp` (integer-ms phase, no randomness) and doctest cases covering every setting combination.
- **Failed:** nothing.
- **Fixed by hand:** nothing yet.

## Task 7 — Scoring
- **Prompt:** gate +1, trap wipeout = cost (min 1) to owner, all/none-finish voids trap points, round and match totals.
- **Agent did:** added `Scoring.h/.cpp` and doctest cases for every rule, including all-finish, none-finish, own-trap, unknown players and match totals/podium.
- **Failed:** nothing.
- **Fixed by hand:** nothing yet.

## Task 8 — Replay recorder
- **Prompt:** multi-player sampler (root + 15 bones at 20 Hz), hit/wipeout/smite events with trap id and owner, binary serialize, interpolated lookup; `make test` incl. 3-min 4-player size < 8 MB and interpolation error < 1 cm.
- **Agent did:** added `Replay.h/.cpp` (float32 positions, int16 rotations, Catmull-Rom position lookup, nlerp rotations) and doctest cases for size, interpolation error, round-trip, clamping and bad input.
- **Failed:** first build had 3.4 cm error in the first/last segment (edge sample repeated as the Catmull-Rom neighbour); fixed by extrapolating it. First check ran without cmake installed.
- **Fixed by hand:** nothing yet.
