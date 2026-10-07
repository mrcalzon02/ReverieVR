# ADR-0021 — Bounded Headset-Relative Touchpad Locomotion

**Status:** Implemented for first-party native 3D modules; S9 validation pending
**Date:** 2026-10-06
**Scope:** RV-0221

## Contract

Daydream touchpad contact drives continuous horizontal camera movement
relative to the headset's current yaw, not the controller's quaternion.
Touchpad up/down means forward/back; left/right means strafe. A 0.28 radial
deadzone, normalized diagonals, 0.70 m/s maximum and 50 ms frame delta cap
keep movement predictable. There is no artificial camera rotation, vertical
motion, acceleration-based coasting, or snap-to-center. Releasing touch
stops travel immediately.

The shell owns one world-space camera translation shared across both
Cardboard eye views and the inverse-view controller/pointer anchor. Touchpad
click remains Select and pauses movement; the Quick Menu also pauses
movement without changing the player's location. BLE pose older than 250 ms,
invalid/absent touch and unknown modules cannot move the camera.

Module-specific virtual envelopes: Test Chamber ±1.55 m X/Z; Red Ledger
±0.70 m lateral and ±0.32 m depth to stay behind the counter. Movement
resets on module transitions. The Test Chamber's former duplicate module
camera translation and touchpad-to-joystick mappings are removed. The
native ABI, DOS controls, media and shell menus are not modified.

This is a virtual scene boundary, **not** a real-world guardian or
collision-mesh guarantee; the headset is still 3DoF. Physical Galaxy S9
tests must verify touchpad polarity, stereo alignment, pointer reach,
room geometry intersections and comfort before expanding locomotion.
Pure-JVM tests cover directions, deadzone, diagonal normalization,
bounds, release stop, frame gaps and binding ownership.
