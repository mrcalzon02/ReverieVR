# 2026-11-04 — Controller shake aims at live headset-forward direction

## Corrected requirement
A rapid, deliberate opposing-impulse shake of a pose-tracked Daydream
controller (or compatible controller reporting orientation and acceleration)
must align the controller's **3D aiming direction and visible orientation**
with the headset's **current viewing direction**. It must not aim at the
world-fixed Home/HUD forward vector or move/recenter the headset, scene or HUD.

## Implementation boundary
The existing shake detector, 65–350 ms opposing-impulse window, 3-second
cooldown, 160 ms recoil-settling period and three-second positional spring
return remain unchanged. The settled shake path now invokes the existing
`ControllerOrientationCalibration.compute(adjustedHeadView, quaternion)`
instead of `computeToHud(quaternion)`. The renderer computes the
`adjustedHeadView` once per frame, from live Cardboard head pose and the
shell's world yaw correction. Calibrated model and pointer ray share the
same matrix. A stale tracked pose is rejected, not retargeted to a virtual
gamepad or stale world axis. Recalibration is a one-time snap, not ongoing
gaze-following.

The separate manually activated **Reset Hand Position** path is
intentionally left pointing to the established HUD target. Generic HID
controllers without accelerometer and orientation telemetry cannot trigger
a physical shake; do not invent IMU data or call that case validated.

## Evidence / next gates
Source math was independently checked with yaw angles 0, +30, +60 and
-90 degrees: transformed calibrated pointer produced headset-view
forward (0, 0, -1). Two pure-Java/JUnit regression cases were added for
live headset yaw/pitch. These math checks are not an Android build, an
installed APK or a real headset test.

Before physical acceptance, run the existing Android unit test/build
flow, then on Galaxy S9 + Daydream controller: look 45–90 degrees
away from Home, shake without pushing buttons, verify the controller
and laser point exactly where the headset is looking while the
room/HUD do not turn, then move the headset and verify the controller
does not continue following gaze. Test pitched gaze, false-shake
rejection, spring return, disconnection/stale pose and manual reset.
