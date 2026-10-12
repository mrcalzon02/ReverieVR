# Phone Screen Panel — Android Display and Touch Proxy

**Status (2026-10-12):** a dedicated Home activation and adjustable static world-space phone housing exist. The original right rail retains six working targets (yaw left/right, pitch up/down, reset, X/Close); a second four-key rail adds actual panel size and distance adjustments. Closing restores the regular Home menu and releases any capture resources. The housing intentionally displays no Android pixels yet; MediaProjection-to-GL display and external touch dispatch remain outstanding. Opening the experimental panel deliberately does not request capture permission while captured frames would only be discarded.

## User experience

Inside the existing 3D Home environment (birch living room, beach or forest), the player can summon a spatially anchored Android screen panel without leaving Home or replacing the environment. It renders the current handset screen at its authentic aspect ratio and pixel content. The panel is a world-space object and may be repositioned independently of the headset; its controls do not turn the whole world or the Quick Menu. The controller pointer intersects the panel, shows a cursor, and the Daydream touchpad click acts as a touchscreen finger. Press-and-hold plus controller movement becomes a drag/scroll; release finishes the touch. Android Back/Home/Recent Apps and keyboard open through separately identified shell controls. The original right-hand control cluster retains yaw left/right, pitch up/down, reset and X/Close; the new left-hand four-key rail changes the entire panel assembly size (-/+) and distance (near/far) rather than changing a fake image. Portrait/landscape switching remains future work. These controls are VR-owned hit regions, outside the Android touch rectangle, and must consume pointer events without generating proxy taps. The regular ReverieVR Quick Menu always supersedes touch mode. Panel focus can be relinquished without losing the headset session.

## Non-negotiable screen privacy and transport boundary

**Never OCR, visually scan, recognize text or images, use computer vision,
perform AI visual inference, or programmatically inspect captured screen
content.** Future MediaProjection frames may travel *only* through the
on-device compositor to the locally displayed phone-panel texture. No
screen pixels, thumbnails, application text, inferred metadata or frame
extracts may be persisted in logs, used for telemetry, transmitted to a
controller peer, included in diagnostics, or uploaded. Frames may be
held transiently in GPU/decoder buffers for display only and destroyed
when closed/revoked. User-requested screenshot/video capture by Android
or scrcpy is outside ReverieVR's internal mirror function and must not
be silently triggered by ReverieVR.

The Android permission dialog uses system wording that can imply
"streaming" even when capture is entirely local. MediaProjection requires
Android user consent regardless of network behavior; **do not bypass this**
for a functional capture mode. Because this prototype currently does not
present pixels, it must not initiate consent from the Phone Screen open
button merely to count/discard images. Re-enable consent only with a
demonstrably working local GL display path, no recursion, a clear explanation
of the Android prompt, and independent user consent for any permitted
external-app gesture feature.

Outbound app-generated traffic remains limited to explicit controller
companion exchange, user-reviewed diagnostic submission, and optional
GitHub update-check/download requests. Network GETs for updates carry
ordinary protocol and app-version metadata but no captured screen data.
Logs, preferences, media references and saves may exist locally; these
are not cloud storage or permission to upload them. Future automatic
diagnostic transmission requires a separate design decision and explicit
consent; it is **not permitted** by this contract.

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

## Consented capture lifecycle slice

A consented `PhoneScreenCaptureSession` and its `VirtualDisplay`/`ImageReader` resource-lifecycle prototype remain in source for later integration, but **the Home Phone Screen activation no longer invokes Android's consent UI** while frames are not presented on the panel. When previously authorized capture is stopped, closing the panel or destroying the Activity still releases the virtual display, image reader and projection, with a revocation callback. The acquired frames are not yet rendered in OpenGL. Android external-app control requires separate explicit accessibility authorization and does not work yet. A future release that actually connects MediaProjection to the GL texture must restore explicit consent and validate supported Android versions/foreground-service restrictions; never bypass Android's prompt.

## Implemented activation slice

The existing first six Home navigation buttons remain unchanged. A seventh left-hand `Phone Screen` button is placed beneath `Exit to Phone` and opens a spatially anchored bezel and dark screen in the active Home environment. While displayed, ordinary Home menus are hidden to prevent their occlusion of the panel and their invisible hit regions are disabled. Android Back dismisses the panel and restores Home, rather than exiting ReverieVR. The Quick Menu remains available. The side control rail now has **six** individually ray-intersectable, visibly marked 3D buttons. Their actions are bounded yaw left/right, pitch up/down, orientation reset and **X/Close** (10 degrees per click; yaw limited to ±65 degrees and pitch to ±40 degrees). The bottom X has a distinct crossed-bars primitive marker that requires no color or texture. Closing restores the normal Home menus, clears UI hover state and releases capture resources without leaving VR. Intersections use the inverse of the same transformation that positions the panel and one shared key layout, so control targets rotate with the panel. These shell controls never dispatch OS touch events. The portrait/landscape toggle, live display capture and Android touch bridge are **still not implemented**. Panel size/depth changes are geometric only; they do not change Android capture resolution or imply that the dark panel is a screen. No phone display or proxy touch has been claimed.


## 2026-10-12 — Real size and distance rail (source, no live display yet)

The original right-side six controls and their X/Close index stay fixed. A
second left-side rail has four separate low-poly glyphs/targets: smaller,
larger, nearer, farther. Sizes are bounded to 0.75–1.15x in 0.10 steps,
and panel depth offsets to -0.30..+0.45 meters in 0.15-meter steps. The
original reset also returns size to 1x and depth to zero. The entire
bezel, dark glass and both control rails share one GLES2 model transform,
including the inverse transformation used for the controller/gaze ray.
No new textures, assets, background tasks, or permissions are introduced.
The raycast no longer constructs six scratch arrays per frame.

Source acceptance: deterministic plain-Java tests cover ten isolated
control positions, original Close identity, out-of-bounds/nonfinite hits,
adjustment clamps and invalid indices. Full Android compile/NDK packaging,
two-eye readability, physical reach, pointing and Galaxy S9 frame pacing
remain pending. No MediaProjection, accessibility dispatch or actual phone
pixels are newly enabled.
