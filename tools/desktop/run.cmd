@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "BASE=%ROOT%\app\src\main\java\io\github\mrcalzon02\reverievr"
set "OUT=%ROOT%\build\desktop-workbench\classes"
if not exist "%OUT%" mkdir "%OUT%"
where javac >nul 2>&1
if errorlevel 1 (echo JDK 17 or newer is required. & exit /b 2)
set "SOURCES=%OUT%\editor-sources.txt"
> "%SOURCES%" (
 echo "%BASE%\HomeEnvironment.java"
 echo "%BASE%\GameModuleRegistry.java"
 echo "%BASE%\ObjMeshReader.java"
 echo "%ROOT%\tools\desktop\SceneDocument.java"
 echo "%ROOT%\tools\desktop\ReverieSceneProject.java"
 echo "%ROOT%\tools\desktop\LightingAuthoringPanel.java"
 echo "%ROOT%\tools\desktop\ReverieDesktop.java"
)
for %%F in ("%ROOT%\tools\editor-kit\upstream\src\main\java\org\beckenburg\editor\*.java") do >>"%SOURCES%" echo "%%~fF"
for %%F in ("%ROOT%\tools\editor-kit\upstream\lighting-module\src\main\java\dev\mrcalzon\lighting\*.java") do >>"%SOURCES%" echo "%%~fF"
javac --release 17 -encoding UTF-8 -d "%OUT%" @"%SOURCES%"
if errorlevel 1 exit /b 1
if "%~1"=="" (
 java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktop "%ROOT%"
) else (
 java -cp "%OUT%" io.github.mrcalzon02.reverievr.ReverieDesktop "%ROOT%" "%~1"
)
exit /b %errorlevel%
