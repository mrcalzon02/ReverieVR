# Phone Screen Panel — Android Display and Touch Proxy

**Status:** design contract and standalone coordinate/gesture safety primitive committed; no screen capture, external input dispatch or visible panel is implemented yet.

## User experience

From the 3D Home, the player can summon a portrait Android screen panel into a comfortable position. It renders the current handset screen at its authentic aspect ratio and pixel content. The controller pointer intersects the panel, shows a cursor, and the Daydream touchpad click acts as a touchscreen finger. Press-and-hold plus controller movement becomes a drag/scroll; release finishes the touch. Android Back/Home/Recent Apps and keyboard open through separately identified shell controls. The regular ReverieVR Quick Menu always supersedes touch mode. Panel focus can be relinquished without losing the headset session.

## Android architecture and permissions

- Display capture: Android `MediaProjectionManager.createScreenCaptureIntent()` initiated explicitly through the 2D consent UI, then `MediaProjection` plus `VirtualDisplay` targeting a dedicated `SurfaceTexture`/OpenGL texture on the VR panel. Respect secure surfaces and protected content, which remain blank; never circumvent `FLAG_SECURE`.
- Remote touch in *other apps*: stock Android does **not** allow a normal application to inject unrestricted touches into unrelated applications using `dispatchTouchEvent`, `Instrumentation`, or hidden `InputManager.injectInputEvent`. An optional, user-enabled `AccessibilityService` with `canPerformGestures` can dispatch gestures where the OS permits; it must be opt-in via Android Settings, user-visible, minimized in scope, and disconnected/revoked cleanly. No root, elevated shell bridge, or hidden accessibility activation. Accessibility gesture dispatch limitations (multi-touch, gesture duration and Android version) must be tested on the actual Galaxy S9.
- To avoid self-capture feedback loops, screen capture must use a deliberate presentation mode and a user-approved transition between VR content and external Android applications. It must not assume that MediaProjection can capture another foreground application while an opaque VR Activity simultaneously monopolizes the phone's visible display. A mirrored virtual display is not a second independent desktop. A workable foreground/session strategy must be established experimentally before advertising unrestricted home-screen interaction.
- MediaProjection consent and Accessibility consent are independent, can be denied, and can be withdrawn at any time. Stopping either service must cleanly release any active touch gesture, VirtualDisplay, GL surface, and OS capture resource. Display and touch activation must never be silently conflated.

## Input math and state

`PhoneScreenTouchMapper` maps the rectangular VR panel's contact coordinates into the active display's pixel dimensions. It rejects nonfinite/outside coordinates, clamps the last valid pixel, enforces explicit authorization before initiating a gesture, and cancels on focus loss, permission revocation, or leaving the panel. This is an input safety primitive, **not** an OS input injector. Controller hover must never dispatch a click; press/release transitions must produce exactly one gesture lifecycle. Tests cover edges and cancellation.

## Acceptance tests

1. Galaxy S9 Android/API target verification and consent/revocation sequences.
2. Screen readable with preserved aspect ratio, portrait orientation changes, stereo comfort, and actual live pixel updates at headset frame rate.
3. Controller hover is visual only. Click taps the correct pixel across center/corners, long press and drag scroll accurately, no input when closed, unfocused or unauthorized.
4. Separate supported Back/Home keyboard actions; shell Quick Menu always recovers control, including failed capture or BLE disconnect.
5. No recursion/capture feedback loops and no protected content exposure; honest user-facing inability to interact with apps/surfaces blocked by the OS.
6. Latency, thermal and battery profiling with both per-eye rendering and camera/VR activity running.

Do not describe the panel as available until capture + presentation + permissioned external touch have been implemented and verified end-to-end.
