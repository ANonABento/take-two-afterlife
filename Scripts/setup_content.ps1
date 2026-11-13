# Builds the generated content headless, in order:
#   create_toon_assets.py -> build_backyard_map.py (Downloads/, placeholders for missing packs)
#   -> swap_to_toon.py (/Game) -> create_title_map.py
# Stops at the first script whose log has a Python Traceback or that exits non-zero, then checks that
# Content/Maps holds Title and Backyard. Run Scripts/fetch_assets.ps1 first for the real meshes.
#   powershell -ExecutionPolicy Bypass -File Scripts/setup_content.ps1 [-Editor "C:\...\UnrealEditor-Cmd.exe"]
param([string]$Editor = $env:UE_EDITOR_CMD)
$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Repo "TakeTwoAfterlife.uproject"

if (-not $Editor) {
    $roots = @()
    if ($env:UE_ROOT) { $roots += $env:UE_ROOT }
    $roots += Get-ChildItem "C:\Program Files\Epic Games" -Directory -Filter "UE_5*" -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | ForEach-Object { $_.FullName }
    foreach ($r in $roots) {
        $c = Join-Path $r "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
        if (Test-Path $c) { $Editor = $c; break }
    }
}
if (-not $Editor -or -not (Test-Path $Editor)) {
    throw "UnrealEditor-Cmd.exe not found: pass -Editor <path> or set UE_EDITOR_CMD / UE_ROOT"
}

$Scripts = @("create_toon_assets.py", "build_backyard_map.py", "swap_to_toon.py", "create_title_map.py")
$LogDir = Join-Path $Repo "Saved\SetupContent"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

foreach ($s in $Scripts) {
    $path = (Join-Path $PSScriptRoot $s) -replace '\\', '/'
    $log = Join-Path $LogDir ($s -replace '\.py$', '.log')
    Write-Host "== $s"
    & $Editor $Project -run=pythonscript "-script=$path" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput 2>&1 |
        Out-File -FilePath $log -Encoding utf8
    $code = $LASTEXITCODE
    if (Select-String -Path $log -Pattern "Traceback" -Quiet) {
        Select-String -Path $log -Pattern "Traceback" -Context 0, 15 | ForEach-Object { $_.ToString() } | Write-Host
        throw "${s} raised a Python Traceback (full log: $log)"
    }
    if ($code -ne 0) { throw "${s}: UnrealEditor-Cmd exited $code (log: $log)" }
}

$Maps = Join-Path $Repo "Content\Maps"
foreach ($m in @("Title", "Backyard")) {
    if (-not (Test-Path (Join-Path $Maps "$m.umap"))) { throw "Content/Maps/$m.umap missing after setup" }
}
Write-Host "Content ready: Content/Maps holds Title and Backyard. Logs in $LogDir"
