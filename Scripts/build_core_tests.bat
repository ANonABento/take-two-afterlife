@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set NINJA="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set CORE=%~dp0..\Core
%CMAKE% -S "%CORE%" -B "%CORE%\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=%NINJA% || exit /b 1
%CMAKE% --build "%CORE%\build" --config Release -j 4 || exit /b 1
"%CORE%\build\tta_core_tests.exe" || exit /b 1
echo Core tests PASSED
