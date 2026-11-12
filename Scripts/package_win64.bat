@echo off
rem Usage: Scripts\package_win64.bat "C:\Program Files\Epic Games\UE_5.5" TeamName
setlocal
set "UE=%~1"
set "TEAM=%~2"
if "%UE%"=="" goto usage
if "%TEAM%"=="" goto usage
set "ROOT=%~dp0.."
set "PROJECT=%ROOT%\TakeTwoAfterlife.uproject"

"%UE%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT%" -run=pythonscript -script="%ROOT%\Scripts\create_title_map.py" -unattended -nop4 || exit /b 1

call "%UE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT%" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -compressed -prereqs -archive -archivedirectory="%ROOT%\Dist" -unattended -utf8output || exit /b 1

python "%ROOT%\Scripts\zip_package.py" "%ROOT%\Dist\Windows" "%TEAM%" --out "%ROOT%\Dist" || exit /b 1
exit /b 0

:usage
echo Usage: %~nx0 ^<UE install dir^> ^<TeamName^>
exit /b 2
