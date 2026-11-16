@echo off
rem Usage: Scripts\package_win64.bat "C:\Program Files\Epic Games\UE_5.5" TeamName
setlocal
set "UE=%~1"
set "TEAM=%~2"
if "%UE%"=="" goto usage
if "%TEAM%"=="" goto usage
set "ROOT=%~dp0.."
set "PROJECT=%ROOT%\TakeTwoAfterlife.uproject"

python "%ROOT%\Scripts\release_checks.py" || exit /b 1

rem Title already in Content is the one to cook. create_title_map deletes that map, and headless
rem actor spawn has been crashing UnrealEditor-Cmd. Only build Title when it is missing.
if not exist "%ROOT%\Content\Maps\Title.umap" (
    "%UE%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT%" -run=pythonscript -script="%ROOT%\Scripts\create_title_map.py" -unattended -nop4 || exit /b 1
)

call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT%" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -compressed -prereqs -archive -archivedirectory="%ROOT%\Dist" -unattended -utf8output || exit /b 1

rem The game reads these as loose files from ProjectDir()/Art/UI (TTAUIStyle slice margins, TTALookIcons hat
rem pictures). The cook does not stage them, so copy them next to the packaged Content.
set "STAGED_UI=%ROOT%\Dist\Windows\TakeTwoAfterlife\Art\UI"
if not exist "%STAGED_UI%\Looks" mkdir "%STAGED_UI%\Looks" || exit /b 1
copy /y "%ROOT%\Art\UI\T_UI_Slices.json" "%STAGED_UI%\" >nul || exit /b 1
copy /y "%ROOT%\Art\UI\Looks\T_UI_Hat_*.png" "%STAGED_UI%\Looks\" >nul || exit /b 1

python "%ROOT%\Scripts\zip_package.py" "%ROOT%\Dist\Windows" "%TEAM%" --out "%ROOT%\Dist" || exit /b 1
exit /b 0

:usage
echo Usage: %~nx0 ^<UE install dir^> ^<TeamName^>
exit /b 2
