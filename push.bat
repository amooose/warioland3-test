@echo off
rem Push the current branch to https://github.com/amooose/warioland3-test.
rem Commit first: this only pushes commits, not uncommitted changes.
rem   push.bat
setlocal
set "REMOTE=https://github.com/amooose/warioland3-test.git"

set "PAUSE_AT_END="
echo %cmdcmdline% | find /i "/c" >nul && set "PAUSE_AT_END=1"

cd /d "%~dp0"

rem safe.directory: git refuses a repo owned by another user (for example
rem Administrators) without it.
rem http.sslBackend: Git for Windows may be set to an OpenSSL backend it
rem can't load; schannel is the Windows one and always works.
set "GIT=git -c safe.directory=* -c http.sslBackend=schannel"

for /f "delims=" %%b in ('%GIT% rev-parse --abbrev-ref HEAD') do set "BRANCH=%%b"
if not defined BRANCH goto failed
if "%BRANCH%"=="HEAD" (
    echo Not on a branch, nothing to push.
    goto failed
)

rem Mention uncommitted changes, which aren't pushed.
for /f %%s in ('%GIT% status --porcelain --untracked-files^=no') do (
    echo Note: there are uncommitted changes; they won't be pushed.
    goto push
)

:push
echo Pushing %BRANCH% to %REMOTE%
%GIT% push "%REMOTE%" "%BRANCH%"
if errorlevel 1 goto failed
if defined PAUSE_AT_END pause
endlocal
exit /b 0

:failed
echo.
echo Push failed, see the messages above.
if defined PAUSE_AT_END pause
endlocal
exit /b 1
