# ADR-0009 — scrcpy / ADB Diagnostic Compatibility

**Status:** Accepted as development requirement  
**Date:** 2026-10-05  
**Affected target:** RV-0006

## Context

ReverieVR is physically awkward to debug because the reference phone is also the VR display inside a Daydream View headset. Repeatedly removing the Galaxy S9 merely to inspect Stage A, observe Stage B, capture failures, or reproduce controller behavior would make hardware iteration needlessly slow.

scrcpy provides low-friction Android screen mirroring and control over ADB without becoming part of the shipped APK.

## Decision

scrcpy compatibility is a first-class development requirement but **not** a runtime dependency.

ReverieVR must remain fully functional when scrcpy is absent.

### Stage A

The ordinary touchscreen setup/recovery surface should remain operable through scrcpy's normal injected mouse/keyboard/touch control.

No Stage A workflow may require a physical touch solely because the app is being mirrored.

### Stage B

The Cardboard-rendered stereoscopic output must remain mirrorable/recordable through scrcpy while the phone display itself remains illuminated in the headset.

Daydream BLE remains the authoritative in-headset control path. scrcpy input may be useful for Android/system recovery, but Stage B must not depend on injected desktop input.

### USB and window policy

The reference runtime must not claim the USB data path in a way that disables ordinary ADB/scrcpy use.

ReverieVR normal windows must not introduce a blanket `FLAG_SECURE` policy that prevents legitimate local developer capture/mirroring.

A future feature with an independently justified protected-content requirement would need its own scoped decision rather than changing the platform default.

### Screen power

Do **not** use scrcpy's `--turn-screen-off` option during headset use. The Galaxy S9 panel is the physical VR display.

For wired development, `--stay-awake` is the preferred baseline.

### Audio on the Galaxy S9 reference path

Current scrcpy supports audio forwarding only on Android 11 or newer.

Therefore, if the Galaxy S9 reference installation is Android 10 or earlier, the host scrcpy window is expected to be silent. ReverieVR audio must be validated from the phone/headphones and host-audio absence must not be filed as an application playback defect.

The project launch helpers therefore use `--no-audio` by default.

### Performance evidence

scrcpy requires screen capture, video encoding, USB transfer, and host decoding. Those workloads can alter CPU/GPU/encoder contention and thermals.

Accordingly:

- scrcpy is allowed for functional testing, defect reproduction, recordings, UI review, controller debugging, and calibration observation;
- scrcpy-assisted measurements may be retained as diagnostic evidence if clearly labeled;
- formal sustained frame-pacing, thermal, battery-drain, and performance acceptance runs must be repeated with scrcpy closed.

## Default launch command

    scrcpy --stay-awake --no-audio

Repository launchers pass any extra user arguments through unchanged.

## Acceptance

RV-0006 remains draft until the real Galaxy S9 demonstrates:

1. USB debugging and scrcpy connection;
2. Stage A remote operation;
3. Stage B mirroring with the phone display on;
4. concurrent Daydream BLE controller operation;
5. representative media playback observation/recording;
6. clean return to an unmirrored performance run.
