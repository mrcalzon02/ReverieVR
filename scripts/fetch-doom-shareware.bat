@echo off
setlocal enabledelayedexpansion

set "SOURCE_URL=https://www.gamers.org/pub/idgames/idstuff/doom/doom19s.zip"
set "EXPECTED_SHA256=cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6"
set "ROOT=%~dp0.."
set "DEST=%ROOT%\third_party\doom-shareware\payload\default-content\doom19s.zip"
set "TMP=%DEST%.tmp"

where powershell >nul 2>nul
if errorlevel 1 (
    echo PowerShell is required to fetch DOOM Shareware.
    exit /b 127
)

if not exist "%ROOT%\third_party\doom-shareware\payload\default-content" (
    mkdir "%ROOT%\third_party\doom-shareware\payload\default-content"
)

if exist "%TMP%" del /f /q "%TMP%"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -UseBasicParsing -Uri '%SOURCE_URL%' -OutFile '%TMP%'"
if errorlevel 1 exit /b 1

for /f "delims=" %%A in ('powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 '%TMP%').Hash.ToLowerInvariant()"') do set "ACTUAL=%%A"

if /I not "!ACTUAL!"=="%EXPECTED_SHA256%" (
    echo DOOM Shareware checksum verification failed.
    echo Expected: %EXPECTED_SHA256%
    echo Actual:   !ACTUAL!
    del /f /q "%TMP%"
    exit /b 1
)

move /y "%TMP%" "%DEST%" >nul
echo Verified original DOOM Shareware v1.9 archive:
echo   %DEST%
echo   SHA-256 !ACTUAL!
