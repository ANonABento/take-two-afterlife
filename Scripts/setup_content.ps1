# Builds the generated content headless, in order:
#   create_toon_assets.py -> build_backyard_map.py (Downloads/, placeholders for missing packs)
#   -> swap_to_toon.py (/Game) -> create_title_map.py
# Stops at the first script whose log has a Traceback or a 'LogPython: Error', or lacks the script's own
# success marker line, then checks that
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

# Run order -> regex for the success line each script logs last. UnrealEditor-Cmd's exit code is not used:
# it is 1 whenever any subsystem logs an error (e.g. EOS with empty Dev Portal IDs) even if the script worked.
$Markers = [ordered]@{
    "create_toon_assets.py" = 'LogPython: toon assets built'
    "build_backyard_map.py" = 'LogPython: built \S+: \d+ props'
    "swap_to_toon.py"       = 'LogPython: swapped \d+ material slots'
    "create_title_map.py"   = 'LogPython: (created \S+|\S+ already exists)'
}
$LogDir = Join-Path $Repo "Saved\SetupContent"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

foreach ($s in $Markers.Keys) {
    $path = (Join-Path $PSScriptRoot $s) -replace '\\', '/'
    $log = Join-Path $LogDir ($s -replace '\.py$', '.log')
    Write-Host "== $s"
    & $Editor $Project -run=pythonscript "-script=$path" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput 2>&1 |
        Out-File -FilePath $log -Encoding utf8
    $code = $LASTEXITCODE
    $bad = Select-String -Path $log -Pattern "Traceback", "LogPython: Error"
    if ($bad) {
        Select-String -Path $log -Pattern "Traceback", "LogPython: Error" -Context 0, 15 |
            ForEach-Object { $_.ToString() } | Write-Host
        throw "${s} failed in Python (full log: $log)"
    }
    if (-not (Select-String -Path $log -Pattern $Markers[$s] -Quiet)) {
        throw "${s}: success line /$($Markers[$s])/ missing (editor exit $code, log: $log)"
    }
    if ($code -ne 0) { Write-Host "   editor exited $code; script succeeded, other errors are in $log" }
}

$Maps = Join-Path $Repo "Content\Maps"
foreach ($m in @("Title", "Backyard")) {
    if (-not (Test-Path (Join-Path $Maps "$m.umap"))) { throw "Content/Maps/$m.umap missing after setup" }
}
Write-Host "Content ready: Content/Maps holds Title and Backyard. Logs in $LogDir"
