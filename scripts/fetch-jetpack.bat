@echo off
setlocal enabledelayedexpansion
set "URL=https://www.adeptsoftware.com/jetpack/jetpak15.zip"
set "EXPECTED_SHA=03f4979149bdeeb43091517e0f33f92b14cc2e5d7531e0c524749c3383fc9d4c"
set "ROOT=%~dp0.."
set "DIR=%ROOT%\third_party\jetpack\payload\default-content"
set "DEST=%DIR%\jetpak15.zip"
set "TMP=%DEST%.tmp"
where powershell >nul 2>nul
if errorlevel 1 exit /b 127
if not exist "%DIR%" mkdir "%DIR%"
if exist "%TMP%" del /f /q "%TMP%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -UseBasicParsing -Uri '%URL%' -OutFile '%TMP%'"
if errorlevel 1 exit /b 1
for /f "delims=" %%A in ('powershell -NoProfile -Command "(Get-Item '%TMP%').Length"') do set "BYTES=%%A"
if not "!BYTES!"=="494745" goto fail
for /f "delims=" %%A in ('powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 '%TMP%').Hash.ToLowerInvariant()"') do set "HASH=%%A"
if /I not "!HASH!"=="%EXPECTED_SHA%" goto fail
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "Add-Type -AssemblyName System.IO.Compression.FileSystem; $z=[IO.Compression.ZipFile]::OpenRead('%TMP%'); try { $names=@($z.Entries | ForEach-Object FullName); if (($names -cnotcontains 'jetpak15/JETPACK.EXE') -or ($names -cnotcontains 'jetpak15/SQUAREZ/SQUAREZ.EXE')) { exit 1 }; foreach ($e in $z.Entries) { if (!$e.FullName.EndsWith('/')) { $s=$e.Open(); try { $buf=New-Object byte[] 8192; while ($s.Read($buf,0,$buf.Length) -gt 0) {} } finally { $s.Dispose() } } } } finally { $z.Dispose() }"
if errorlevel 1 goto fail
move /y "%TMP%" "%DEST%" >nul
echo Verified Jetpack and Squarez original freeware archive.
exit /b 0
:fail
echo Author-hosted Jetpack original ZIP failed validation.
if exist "%TMP%" del /f /q "%TMP%"
exit /b 1
