# Take Two: Afterlife

## Build
- `make test` — Core C++ doctests + Python script tests.
- Content: `Scripts/fetch_assets.ps1`, then `Scripts/setup_content.ps1` (builds toon assets, Backyard, toon swap, Title into `Content/Maps`).
- Package: `Scripts/package_win64.bat`.

## Online (EOS)
Online defaults to the **NULL** subsystem (LAN/IP), so the editor, commandlets and packages run without Epic IDs.
To use EOS lobbies + P2P:
1. Epic Developer Portal -> your product -> note **ProductId, SandboxId, DeploymentId**, and a client's **ClientId / ClientSecret** (Product Settings -> Clients).
2. Editor: Project Settings -> Plugins -> **Epic Online Services** -> Artifacts -> add one and paste those five values plus an encryption key (64 hex chars). This writes `+Artifacts=(...)` under `[/Script/OnlineSubsystemEOS.EOSSettings]` in `Config/DefaultEngine.ini`.
3. In `Config/DefaultEngine.ini`, set `[OnlineSubsystem] DefaultPlatformService=EOS`.
Keep the ClientSecret out of git (paste it only on the build machine). With the IDs empty, leave it on NULL; `make test` fails if EOS is selected without IDs.
