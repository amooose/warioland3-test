@echo off
rem Pull the latest changes and build the PC port, using MSYS2.
rem   update.bat        pull and build
rem   update.bat run    pull, build and start the game
rem Edit the two paths below if MSYS2 or RGBDS are installed elsewhere.
setlocal
set "MSYS2_DIR=C:\msys64"
set "RGBDS=/c/rgbds/"

rem When double-clicked, keep the window open at the end so the output
rem can be read.
set "PAUSE_AT_END="
echo %cmdcmdline% | find /i "/c" >nul && set "PAUSE_AT_END=1"

if not exist "%MSYS2_DIR%\usr\bin\bash.exe" (
    echo MSYS2 was not found in %MSYS2_DIR%. Edit MSYS2_DIR at the top of update.bat.
    goto failed
)

rem Start the MSYS2 UCRT64 environment in this folder.
cd /d "%~dp0"
set MSYSTEM=UCRT64
set CHERE_INVOKING=1
"%MSYS2_DIR%\usr\bin\bash.exe" -lc "./port/update.sh %1"
if errorlevel 1 goto failed
if defined PAUSE_AT_END pause
endlocal
exit /b 0

:failed
echo.
echo Update failed, see the messages above.
if defined PAUSE_AT_END pause
endlocal
exit /b 1
