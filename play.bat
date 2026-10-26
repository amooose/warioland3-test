@echo off
rem Start the PC port. Arguments are passed on to wl3.exe, for example
rem   play.bat --record port\tests\inputs\my_run.input
rem Edit MSYS2_DIR if MSYS2 is installed somewhere else.
setlocal
set "MSYS2_DIR=C:\msys64"

rem wl3.exe needs SDL2.dll; the build copies it next to the exe, and
rem MSYS2's folder is added to the path in case anything else is missing.
set "PATH=%MSYS2_DIR%\ucrt64\bin;%PATH%"

set "WL3=%~dp0port\build\wl3.exe"
if not exist "%WL3%" (
    echo port\build\wl3.exe does not exist yet: run build.bat first.
    pause
    exit /b 1
)

cd /d "%~dp0"
"%WL3%" %*
rem Keep the window open if something went wrong, so the error can be read.
if errorlevel 1 pause
endlocal
