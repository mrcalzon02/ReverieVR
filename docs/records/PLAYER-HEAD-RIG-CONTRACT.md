# Player headset rig — active implementation contract

**Status:** shell-owned geometry/collision foundation implemented; mirror display, scene collision response and Galaxy S9 validation pending.

## Pose and optics

The rig updates once per GL frame from `VrShellRenderer.adjustedHeadView`, after the same yaw calibration, bounded inertial translation and optional native locomotion applied to the stereo eyes. The rigid center-head view is inverted without a per-frame allocation. Calibrated user IPD separates left and right eye markers along the headset's local X axis. The rig never writes Cardboard's per-eye view, perspective, viewport or scissor state.

This is a 3DoF pose proxy: the phone does not supply true positional head tracking. Artificial shell offsets are not physical room movement.

## Geometry and collision

`PlayerHeadRig` exposes 36 world-space line vertices: twelve oriented headset-box edges and a three-axis cross at each eye. The approximate headset collision box has half extents 0.110m × 0.085m × 0.085m, offset 0.020m behind the eye midpoint. A sphere-vs-oriented-box query supports exact geometric overlap against a point or small tool sphere. The shell currently probes the controller proxy with a 0.045m radius and logs enter/exit transitions in Development mode only.

The collider is **not** a user safety boundary, full-body physics collider, room-scale tracking, or game obstacle response. It must not suppress live controller input or silently move the headset. No user-facing toggle is exposed.

## Rendering contract

The wireframe is intentionally **not drawn into the user's own near-eye view**; that would place opaque/near-plane geometry on top of the camera and impair comfort. A future mirror/third-person pass can draw this geometry with its own camera, correct occlusion and both-eye coherence. Do not claim the mirror exists before it is implemented and headset-tested.

## Acceptance gates

JVM tests verify identity/yaw/translation poses, eye marker separation, box-sphere intersection, deterministic finite line geometry and rejection of invalid transforms. Phone-test CI checks Android compilation and integration. A real S9 headset run must still verify world scale, calibration, mirror behavior once implemented, and absence of eye/view regressions.
