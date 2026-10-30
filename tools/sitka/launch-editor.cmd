@echo off
setlocal
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "VENDOR=%ROOT%\tools\editor-kit\upstream"
set "OUT=%ROOT%\build\sitka\editor-classes"
if not exist "%OUT%" mkdir "%OUT%"
where javac >nul 2>&1
if errorlevel 1 (echo JDK 17 or newer is required. & exit /b 2)
set "SOURCES=%OUT%\sources.txt"
> "%SOURCES%" echo "%ROOT%\tools\sitka\SitkaEditor.java"
for %%F in ("%VENDOR%\src\main\java\org\beckenburg\editor\*.java") do >>"%SOURCES%" echo "%%~fF"
for %%F in ("%VENDOR%\water-module\src\main\java\dev\mrcalzon\water\*.java") do >>"%SOURCES%" echo "%%~fF"
for %%F in ("%VENDOR%\src\main\java\dev\mrcalzon\weather\*.java") do >>"%SOURCES%" echo "%%~fF"
javac --release 17 -encoding UTF-8 -d "%OUT%" @"%SOURCES%"
if errorlevel 1 exit /b 1
if "%~1"=="" (
 java -cp "%OUT%" SitkaEditor "%ROOT%\build\sitka\authoring"
) else (
 java -cp "%OUT%" SitkaEditor "%~1"
)
exit /b %errorlevel%
