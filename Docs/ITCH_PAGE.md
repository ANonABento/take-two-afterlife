# Take Two: Afterlife — itch.io page text

## Pitch
You died on a backyard obstacle course. Heaven gives you one more take: build the traps, film the wipeouts, cut the footage, and premiere your Director's Cut to the angels.

## Short description
A 2–4 player online party game: build a trap course, run it, then edit the camera footage of everyone's wipeouts into a 30-second Director's Cut and rate each other's films in a heavenly cinema.

## Long description
**Theme: Director's Cut.** In Take Two: Afterlife you are both the stunt victim and the director. Each round you place traps and cameras on a shared backyard course, run it as a human with a handheld camera, and then sit in the edit bay with the footage every camera recorded. Trim, reorder, switch angles, add slow-mo, zoom, stickers, captions and a "WASTED"-style card, and cut the funniest 30 seconds you can.

Then everyone floats up to a cloud cinema. Cuts play in random order, the narrator introduces the director and the victim, and the audience rates each cut on **Funny** (points to the editor) and **Evil** (points to the trap builder). Traps stay on the map between rounds, so the course gets nastier every take. Footage is recorded as replay data, so your impact frames, hits and mic audio come back in the edit and on the big screen.

Online with Epic Online Services join codes. Slapstick only, suitable for 13+.

## Controls
Keyboard + mouse (from the project's input mappings; gamepad: TODO).
- Move: W A S D · Look: mouse
- Jump: Space · Dive: Left Shift
- Raise handheld camera (Run): Right mouse button
- Build — place: Left mouse · remove: Right mouse · select: F · rotate 45°: R · next/previous item: mouse wheel
- Build — trap setting 1 down/up: 1 / 2 · setting 2 down/up: 3 / 4
- Editor, rating a cut (Funny / Evil 1–5), voice: TODO (confirm on-screen UI / push-to-talk in the packaged build)

## How to play
1. **Host or join** — the host creates a lobby and shares the join code. Players can join between rounds.
2. **Rounds** — each round runs Build → Run → Edit → Premiere. Number of rounds and timers are host settings.
3. **Traps (Build)** — spend a 100-point budget on Hammer, Red ball cannon, Spinner, Pusher wall, Trapdoor and Banana, each with two settings, plus up to 12 cameras of each type (fixed, pan, follow, trap-cam). Traps persist across rounds.
4. **Run** — reach the gate for +1. When your trap wipes someone out you score its cost. If everyone or nobody finishes, no trap points that round.
5. **Replay (Edit)** — 90 s to cut up to 30 s from the replay footage of your cameras and handheld. Hits are auto-marked; auto-edit kicks in if time runs out.
6. **Premiere** — watch every cut in the heaven cinema and rate others' cuts 1–5 on Funny and Evil. Ratings add to the score; the podium is crowned at the end.

## Tools used
- Unreal Engine 5.5 (C++, UMG, Python editor scripting, Automation)
- Epic Online Services (lobbies, P2P relay, voice)
- Cursor + Grok (Windows compile, link fixes, packaging, Automation)
- Other AI coding agents: Choomfie / Waymond (Claude) for code, scripts, tests and docs
- KayKit and Kenney asset packs (see CREDITS.md)
- Meshy assets: TODO — confirm none are used (plan says human-made assets only)
- ElevenLabs (narrator TTS), WeLoveIndies music, Freesound SFX, Blender, CMake + doctest, Python 3, OBS Studio
Full list and attributions: CREDITS.md.

## Team credits
- Kevin — TODO: role(s)
- TODO: other team members and roles
- Team name: TODO
