# Phone Screen Panel — Android Display and Touch Proxy

**Status:** dedicated Home activation and actual static world-space phone housing now integrated into the Home renderer, alongside the coordinate/gesture safety primitive. The housing intentionally displays no Android pixels yet; MediaProjection, permissioned touch dispatch, and functional upper-right orientation controls are still outstanding.

## User experience

Inside the existing 3D Home environment (birch living room, beach or forest), the player can summon a spatially anchored Android screen panel without leaving Home or replacing the environment. It renders the current handset screen at its authentic aspect ratio and pixel content. The panel is a world-space object and may be repositioned independently of the headset; its controls do not turn the whole world or the Quick Menu. The controller pointer intersects the panel, shows a cursor, and the Daydream touchpad click acts as a touchscreen finger. Press-and-hold plus controller movement becomes a drag/scroll; release finishes the touch. Android Back/Home/Recent Apps and keyboard open through separately identified shell controls. The panel has a distinct upper-right control cluster with orientation controls: yaw left/right, pitch up/down, recenter to headset forward, portrait/landscape presentation, and size/distance adjustment. These controls are VR-owned hit regions, outside the Android touch rectangle, and must consume pointer events without generating proxy taps. The regular ReverieVR Quick Menu always supersedes touch mode. Panel focus can be relinquished without losing the headset session.

## Android architecture and permissions

- Display capture: Android `MediaProjectionManager.createScreenCaptureIntent()` initiated explicitly through the 2D consent UI, then `MediaProjection` plus `VirtualDisplay` targeting a dedicated `SurfaceTexture`/OpenGL texture on the VR panel. Respect secure surfaces and protected content, which remain blank; never circumvent `FLAG_SECURE`.
- Remote touch in *other apps*: stock Android does **not** allow a normal application to inject unrestricted touches into unrelated applications using `dispatchTouchEvent`, `Instrumentation`, or hidden `InputManager.injectInputEvent`. An optional, user-enabled `AccessibilityService` with `canPerformGestures` can dispatch gestures where the OS permits; it must be opt-in via Android Settings, user-visible, minimized in scope, and disconnected/revoked cleanly. No root, elevated shell bridge, or hidden accessibility activation. Accessibility gesture dispatch limitations (multi-touch, gesture duration and Android version) must be tested on the actual Galaxy S9.
- To avoid self-capture feedback loops, screen capture must use a deliberate presentation mode and a user-approved transition between VR content and external Android applications. It must not assume that MediaProjection can capture another foreground application while an opaque VR Activity simultaneously monopolizes the phone's visible display. A mirrored virtual display is not a second independent desktop. A workable foreground/session strategy must be established experimentally before advertising unrestricted home-screen interaction.
- MediaProjection consent and Accessibility consent are independent, can be denied, and can be withdrawn at any time. Stopping either service must cleanly release any active touch gesture, VirtualDisplay, GL surface, and OS capture resource. Display and touch activation must never be silently conflated.

## Input math and state

`PhoneScreenTouchMapper` maps the rectangular VR panel's contact coordinates into the active display's pixel dimensions. It rejects nonfinite/outside coordinates, clamps the last valid pixel, enforces explicit authorization before initiating a gesture, and cancels on focus loss, permission revocation, or leaving the panel. This is an input safety primitive, **not** an OS input injector. Controller hover must never dispatch a click; press/release transitions must produce exactly one gesture lifecycle. Tests cover edges and cancellation.

## Acceptance tests

1. Galaxy S9 Android/API target verification and consent/revocation sequences.
2. Screen panel is embedded inside each of the birch living room, beach and forest 3D home scenes; virtual upper-right orientation controls visibly rotate, tilt, resize, move and recenter the panel while preserving the Android screen's proper portrait/landscape aspect and live pixel updates. The scene and shell HUD remain visible.
3. Controller hover is visual only. Click taps the correct pixel across center/corners, long press and drag scroll accurately, no input when closed, unfocused or unauthorized.
4. Separate supported Back/Home keyboard actions; shell Quick Menu always recovers control, including failed capture or BLE disconnect.
5. No recursion/capture feedback loops and no protected content exposure; honest user-facing inability to interact with apps/surfaces blocked by the OS.
6. Latency, thermal and battery profiling with both per-eye rendering and camera/VR activity running.

Do not describe the panel as available until capture + presentation + permissioned external touch have been implemented and verified end-to-end.

## Implemented activation slice

The existing first six Home navigation buttons remain unchanged. A seventh left-hand `Phone Screen` button is placed beneath `Exit to Phone` and opens a spatially anchored bezel and dark screen in the active Home environment. While displayed, ordinary Home menus are hidden to prevent their occlusion of the panel and their invisible hit regions are disabled. Android Back dismisses the panel and restores Home, rather than exiting ReverieVR. The Quick Menu remains available. The upper-right rail marks the reserved orientation-control area; it does **not yet** respond to controller input. No phone display or proxy touch has been claimed.
