@echo off
setlocal enabledelayedexpansion
set "SOURCE_URL=https://advsys.net/ken/klab/labfull.zip"
set "EXPECTED_SHA256=643aa0ca8d1a7d2c88afecd952da00f3b912be8b83c4c1d2f4ab9b50491fadfd"
set "ROOT=%~dp0.."
set "DEST=%ROOT%\third_party\kens-labyrinth\payload\default-content\labfull.zip"
set "TMP=%DEST%.tmp"
where powershell >nul 2>nul
if errorlevel 1 exit /b 127
if not exist "%ROOT%\third_party\kens-labyrinth\payload\default-content" (
    mkdir "%ROOT%\third_party\kens-labyrinth\payload\default-content"
)
if exist "%TMP%" del /f /q "%TMP%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -UseBasicParsing -Uri '%SOURCE_URL%' -OutFile '%TMP%'"
if errorlevel 1 exit /b 1
for /f "delims=" %%A in ('powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 '%TMP%').Hash.ToLowerInvariant()"') do set "ACTUAL=%%A"
for /f "delims=" %%A in ('powershell -NoProfile -Command "(Get-Item '%TMP%').Length"') do set "ACTUAL_BYTES=%%A"
if not "!ACTUAL_BYTES!"=="940242" (
    echo Ken's Labyrinth official ZIP size mismatch.
    del /f /q "%TMP%"
    exit /b 1
)
if /I not "!ACTUAL!"=="%EXPECTED_SHA256%" (
    echo Ken's Labyrinth SHA-256 mismatch.
    del /f /q "%TMP%"
    exit /b 1
)
move /y "%TMP%" "%DEST%" >nul
echo Verified Ken's Labyrinth v2.1 archive.
