# Downloads the CC0 packs Scripts/backyard_layout.KITS uses (from choomfie-library/ref/assets-ue5.md) into
# <repo>/Downloads/<kit key>, unzipped. Downloads/ is gitignored. Re-running skips packs already there.
# A pack that fails to download is reported and skipped: build_backyard_map.py then uses placeholder blocks.
#   powershell -ExecutionPolicy Bypass -File Scripts/fetch_assets.ps1
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$Repo = Split-Path -Parent $PSScriptRoot
$Downloads = Join-Path $Repo "Downloads"
New-Item -ItemType Directory -Force -Path $Downloads | Out-Null

# kit key -> sources tried in order: a direct .zip, or a Kenney pack page (its .zip link is read from the page)
$Packs = [ordered]@{
    "KayKitPrototypeBits" = @("https://github.com/KayKit-Game-Assets/KayKit-Prototype-Bits-1.0/archive/refs/heads/main.zip",
                              "https://github.com/KayKit-Game-Assets/KayKit-Prototype-Bits-1.0/archive/refs/heads/master.zip")
    "KenneyCitySuburban"  = @("https://kenney.nl/assets/city-kit-suburban")
    "KenneyNature"        = @("https://kenney.nl/assets/nature-kit")
}

function Resolve-Zip([string]$url) {
    if ($url -like "*.zip") { return $url }
    $page = Invoke-WebRequest -UseBasicParsing -Uri $url
    $m = [regex]::Match($page.Content, 'https://kenney\.nl/media/pages/assets/[^"''\s]+?\.zip')
    if (-not $m.Success) { throw "no .zip link on $url" }
    return $m.Value
}

$failed = @()
foreach ($kit in $Packs.Keys) {
    $dest = Join-Path $Downloads $kit
    if ((Test-Path $dest) -and (Get-ChildItem $dest -Recurse -Filter *.fbx | Select-Object -First 1)) {
        Write-Host "$kit already in $dest"
        continue
    }
    $ok = $false
    foreach ($src in $Packs[$kit]) {
        try {
            $zipUrl = Resolve-Zip $src
            $zip = Join-Path $env:TEMP "$kit.zip"
            Write-Host "$kit <- $zipUrl"
            Invoke-WebRequest -UseBasicParsing -Uri $zipUrl -OutFile $zip
            if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
            Expand-Archive -Path $zip -DestinationPath $dest -Force
            Remove-Item $zip
            $ok = $true
            break
        } catch {
            Write-Warning "$kit from $src failed: $($_.Exception.Message)"
        }
    }
    if (-not $ok) { $failed += $kit }
}
if ($failed.Count) {
    Write-Warning ("Not downloaded (placeholders will be used): " + ($failed -join ", ") + ". Unzip by hand into $Downloads\<kit key>.")
} else {
    Write-Host "All packs in $Downloads"
}
