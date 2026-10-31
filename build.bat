@echo off
rem Build the ROM and the PC port, using MSYS2.
rem   build.bat            build the game (no interpreter), in port\build-game
rem   build.bat run        build the game and start it
rem   build.bat dev        build the development build (interpreter, tests,
rem                        recording), in port\build
rem   build.bat dev run    build that and start it
rem Edit the two paths below if MSYS2 or RGBDS are installed elsewhere.
setlocal
set "MSYS2_DIR=C:\msys64"
set "RGBDS=/c/rgbds/"

rem When double-clicked, keep the window open at the end so the output
rem can be read.
set "PAUSE_AT_END="
echo %cmdcmdline% | find /i "/c" >nul && set "PAUSE_AT_END=1"

if not exist "%MSYS2_DIR%\usr\bin\bash.exe" (
    echo MSYS2 was not found in %MSYS2_DIR%. Edit MSYS2_DIR at the top of build.bat.
    goto failed
)

rem A running game locks wl3.exe, and the build can't replace it.
tasklist /fi "imagename eq wl3.exe" 2>nul | find /i "wl3.exe" >nul && (
    echo The game is running. Close it, then build again.
    goto failed
)

rem The MSYS2 login shell starts in its home folder, so pass this folder
rem as an absolute path and cd there.
set "REPO=%~dp0"
set MSYSTEM=UCRT64
"%MSYS2_DIR%\usr\bin\bash.exe" -lc "cd \"$(cygpath -u \"$REPO\")\" && ./port/build.sh %1 %2"
if errorlevel 1 goto failed
if defined PAUSE_AT_END pause
endlocal
exit /b 0

:failed
echo.
echo Build failed, see the messages above.
if defined PAUSE_AT_END pause
endlocal
exit /b 1
