@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "ROOT=%%~fI"
set "BASE=%ROOT%\app\src\main\java\io\github\mrcalzon02\reverievr"
set "SRC=%ROOT%\tools\desktop\player"
set "OUT=%ROOT%\build\desktop-player\classes"
if not exist "%OUT%" mkdir "%OUT%"
where javac >nul 2>&1
if errorlevel 1 (echo JDK 17 or newer is required. & exit /b 2)
javac --release 17 -encoding UTF-8 -d "%OUT%" "%BASE%\HomeEnvironment.java" "%BASE%\GameModuleRegistry.java" "%SRC%\DesktopHubState.java" "%SRC%\DesktopDeskKeyboard.java" "%SRC%\DesktopArcade.java" "%SRC%\ReverieDesktopPlayer.java"
if errorlevel 1 exit /b 1
if "%~1"=="" (
 java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktopPlayer "%ROOT%\build\desktop-workspace"
) else (
 java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktopPlayer "%~1"
)
endlocal
