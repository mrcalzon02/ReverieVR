# DOS fullscreen comfortable optical fit — 2026-10-15

The user reports cropped/hard-to-see screen edges when viewing traditional
2D DOS games through Galaxy S9 + Daydream headset. Unlike a spatial 3D
scene, DOS games often require every pixel at the outermost edges for HUD,
inventory and input.

The fullscreen DOS image is identical in **both eye channels** and
head-fixed; it must retain its intended source display aspect, never crop,
stretch to fill an eye, or remove the shell's global recovery and FPS
overlays. The optical-safe viewport is now 70% of each eye's raw width and
height before `RetroDisplayMode.fittedWidth/fittedHeight` selects the
largest centered guest-aspect rectangle within it. The rest of each eye
stays black because the renderer already clears the entire eye first.
The normal DOS options room remains unchanged, as do native games and
video display modes. No DOS simulation or texture upload behavior changes.

A reference nominal Galaxy S9 eye of 1480x1440 pixels yields an inset of
1036x1008, and a full 4:3 guest image of 1036x777 centered pixels. This
leaves at least 222px at either side, 331px at top and bottom. Actual
visibility depends on headset optics and face positioning.

Acceptance: compile and run `RetroDisplayModeCatalogTest` under JUnit;
assert complete guest aspect-fit and safe centered margins for 4:3, 5:4,
3:2 and 16:9 on multiple eye shapes. Physical S9 checks must establish
that entire DOS frame edges (score, health, menu buttons, inventory) are
visible in both eyes, black letterbox margins are acceptable, pointer
coordinates coincide with displayed image, no unwanted shell options
appear, and Quick Menu/HUD remain accessible. Increase/decrease the safe
scale only after headset feedback. Avoid an independent VR-level frame-rate
change or adding a redundant second scale implementation.

Status: source change; local equivalent Java optical-fit smoke test
passed. Android APK build and headset validation not yet performed.
