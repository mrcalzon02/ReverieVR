@echo off
setlocal enabledelayedexpansion

set "REPO_URL=https://github.com/schellingb/dosbox-pure.git"
set "PIN=a4a0bab7f8931433588f2fcad9045c85b277373d"
set "ROOT=%~dp0.."
set "DEST=%ROOT%\third_party\dosbox-pure\src"

where git >nul 2>nul
if errorlevel 1 (
    echo git is required to fetch DOSBox Pure.
    exit /b 127
)

if exist "%DEST%\.git" (
    for /f "delims=" %%A in ('git -C "%DEST%" status --porcelain') do (
        echo Refusing to modify dirty DOSBox Pure checkout: %DEST%
        exit /b 1
    )
    git -C "%DEST%" fetch --tags origin
    if errorlevel 1 exit /b 1
) else (
    if not exist "%ROOT%\third_party\dosbox-pure" mkdir "%ROOT%\third_party\dosbox-pure"
    git clone --no-checkout "%REPO_URL%" "%DEST%"
    if errorlevel 1 exit /b 1
)

git -C "%DEST%" checkout --detach "%PIN%"
if errorlevel 1 exit /b 1

for /f "delims=" %%A in ('git -C "%DEST%" rev-parse HEAD') do set "ACTUAL=%%A"
if /I not "!ACTUAL!"=="%PIN%" (
    echo DOSBox Pure pin verification failed.
    echo Expected: %PIN%
    echo Actual:   !ACTUAL!
    exit /b 1
)

echo DOSBox Pure ready at !ACTUAL!
