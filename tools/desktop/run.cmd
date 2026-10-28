@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "BASE=%ROOT%\app\src\main\java\io\github\mrcalzon02\reverievr"
set "OUT=%ROOT%\build\desktop-workbench\classes"
if not exist "%OUT%" mkdir "%OUT%"
where javac >nul 2>&1
if errorlevel 1 (echo JDK 17 or newer is required. & exit /b 2)
javac --release 17 -encoding UTF-8 -d "%OUT%" "%BASE%\HomeEnvironment.java" "%BASE%\GameModuleRegistry.java" "%BASE%\ObjMeshReader.java" "%ROOT%\tools\desktop\SceneDocument.java" "%ROOT%\tools\desktop\ReverieDesktop.java"
if errorlevel 1 exit /b 1
if "%~1"=="" (
  java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktop "%ROOT%"
) else (
  java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktop "%ROOT%" "%~1"
)
endlocal
