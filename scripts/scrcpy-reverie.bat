@echo off
where scrcpy >nul 2>nul
if errorlevel 1 (
    echo scrcpy was not found on PATH.
    echo Install scrcpy, enable USB debugging on the Android device, then retry.
    exit /b 127
)

echo ReverieVR scrcpy diagnostic session
echo   baseline: --stay-awake --no-audio
echo   NOTE: do not use --turn-screen-off while the phone is in the VR headset.
echo   NOTE: close scrcpy before formal performance/thermal acceptance runs.
echo   DOS HID: add --keyboard=uhid --mouse=uhid ^(or -KM^) for physical-HID semantics.

scrcpy --stay-awake --no-audio %*
