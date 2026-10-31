@echo off
rem Start the PC port. Arguments are passed on to wl3.exe, for example
rem   play.bat --record port\tests\inputs\my_run.input
rem It runs the game build (port\build-game, from build.bat). Recording,
rem replaying and the other development options need the development build
rem (port\build, from build.bat dev), which it uses for those instead.
rem Edit MSYS2_DIR if MSYS2 is installed somewhere else.
setlocal
set "MSYS2_DIR=C:\msys64"

rem wl3.exe needs SDL2.dll; the build copies it next to the exe, and
rem MSYS2's folder is added to the path in case anything else is missing.
set "PATH=%MSYS2_DIR%\ucrt64\bin;%PATH%"

set "WL3=%~dp0port\build-game\wl3.exe"
set "BUILD=build.bat"
echo.%* | findstr /i /c:"--record" /c:"--input" /c:"--hooks" /c:"--no-hooks" /c:"--difftest" /c:"--fuzz" /c:"--trace" /c:"--break" /c:"--coverage" /c:"--asm-profile" >nul && (
    set "WL3=%~dp0port\build\wl3.exe"
    set "BUILD=build.bat dev"
)
if not exist "%WL3%" (
    echo %WL3% does not exist yet: run %BUILD% first.
    pause
    exit /b 1
)

cd /d "%~dp0"
"%WL3%" %*
rem Keep the window open if something went wrong, so the error can be read.
if errorlevel 1 pause
endlocal
