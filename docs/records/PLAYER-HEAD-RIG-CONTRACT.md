# Player headset rig — active implementation contract

**Status:** shell-owned geometry/collision and white-room diagnostic mirror implemented; full scene reflections, collision response and Galaxy S9 validation pending.

## Pose and optics

The rig updates once per GL frame from `VrShellRenderer.adjustedHeadView`, after the same yaw calibration, bounded inertial translation and optional native locomotion applied to the stereo eyes. The rigid center-head view is inverted without a per-frame allocation. Calibrated user IPD separates left and right eye markers along the headset's local X axis. The rig never writes Cardboard's per-eye view, perspective, viewport or scissor state.

This is a 3DoF pose proxy: the phone does not supply true positional head tracking. Artificial shell offsets are not physical room movement.

## Geometry and collision

`PlayerHeadRig` exposes 36 world-space line vertices: twelve oriented headset-box edges and a three-axis cross at each eye. The approximate headset collision box has half extents 0.110m × 0.085m × 0.085m, offset 0.020m behind the eye midpoint. A sphere-vs-oriented-box query supports exact geometric overlap against a point or small tool sphere. The shell currently probes the controller proxy with a 0.045m radius and logs enter/exit transitions in Development mode only.

The collider is **not** a user safety boundary, full-body physics collider, room-scale tracking, or game obstacle response. It must not suppress live controller input or silently move the headset. No user-facing toggle is exposed.

## Rendering contract

The wireframe is intentionally **not drawn into the user's own near-eye view**; that would place near-plane geometry on top of the camera and impair comfort. In the white home room, a small vanity panel at world X=2.14m (to the player's right) now displays a **diagnostic mirror of the headset/eye wireframe and active controller proxy**. `HeadsetMirrorProjection` reflects world-space proxy vertices across the panel, intersects sightlines from each separately calibrated eye, clips the projected line segments to the mirror rectangle and draws them on the visible panel. Both eyes see a consistent stereo scene from their own viewpoints. The controller proxy includes a position cross and a directional arrow derived from the active tracked/virtual pointer ray, so changes in aim are visible rather than showing only position. The clipped mirror lines are expanded to 14mm-wide triangles on the panel; GLES2 wide-line support is not required. All geometry uses reused per-eye scratch buffers with no frame allocations. The mirror is not a full environment reflection, does not render a face/body avatar, and does not change Cardboard projection. It is not shown in native games or other home environments. The panel is visible by looking to the right in the white room. Headset testing is still needed for comfort, depth and visibility.

## Acceptance gates

JVM tests verify identity/yaw/translation poses, eye marker separation, box-sphere intersection, deterministic finite line geometry and rejection of invalid transforms. Phone-test CI checks Android compilation and integration. A real S9 headset run must still verify world scale, calibration, the diagnostic mirror's visibility and stereo parallax, and absence of eye/view regressions.
