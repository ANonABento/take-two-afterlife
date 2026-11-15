# Windows equivalent of `make test` (no make needed).
# Usage: powershell -File Scripts/test.ps1     (from the repo root)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)

# pip-installed cmake lands in the per-user Scripts dir, which is often not on PATH.
$userScripts = python -c "import sysconfig; print(sysconfig.get_path('scripts', 'nt_user'))"
if (Test-Path $userScripts) { $env:PATH = "$userScripts;$env:PATH" }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'cmake not found: python -m pip install -r Scripts/requirements.txt'
}

cmake -S Core -B build -DCMAKE_BUILD_TYPE=Debug;               if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build --config Debug;                            if ($LASTEXITCODE) { exit $LASTEXITCODE }
ctest --test-dir build --output-on-failure --build-config Debug; if ($LASTEXITCODE) { exit $LASTEXITCODE }
python -m unittest discover -s Scripts/tests;                  exit $LASTEXITCODE
