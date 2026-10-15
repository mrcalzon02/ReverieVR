@echo off
setlocal enabledelayedexpansion
set "SOURCE_URL=https://advsys.net/ken/LABFULL.ZIP"
set "EXPECTED_SHA256=5baf67ed3795be0dff3f32819f0c20fa84d3a00d667215d7a1146a7971c81072"
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
if /I not "!ACTUAL!"=="%EXPECTED_SHA256%" (
    echo Ken's Labyrinth SHA-256 mismatch.
    del /f /q "%TMP%"
    exit /b 1
)
move /y "%TMP%" "%DEST%" >nul
echo Verified Ken's Labyrinth v2.1 archive.
