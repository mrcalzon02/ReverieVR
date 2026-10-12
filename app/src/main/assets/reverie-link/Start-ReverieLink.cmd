@echo off
setlocal
title ReverieVR Laptop Link
cd /d "%~dp0"
where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo Windows PowerShell could not be found.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File "%~dp0ReverieLink.ps1"
if errorlevel 1 (
  echo.
  echo ReverieLink exited with an error. No Windows settings were modified.
)
echo.
pause
endlocal
