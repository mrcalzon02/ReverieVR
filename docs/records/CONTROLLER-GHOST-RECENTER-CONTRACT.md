# Controller ghost recenter — implementation contract

Status: Runtime implementation committed; Galaxy S9 visual, audio, thermal and stereo proof pending.

## Interaction
On one confirmed sharp-shake gesture, play one short spring-like audio cue, show a translucent duplicate of the existing Daydream controller model at the corrected headset-relative pose, and draw a thin spring/tether between live and ghost positions. Do not use a target ball.

## Tracking invariants
- Preserve raw controller tracking and input throughout. Never animate or mutate sensor samples.
- Maintain independent tracked orientation; the ghost shows the same tracked orientation at the corrected position.
- Blend only the headset-relative calibration offset toward zero over three seconds using monotonic easing, no overshoot.
- Each frame recompute live and ghost world transforms from current headset pose. Never freeze the target in world space.
- Spring and ghost must render for both eyes with proper depth and stereo projection.
- If controller disconnects, tracking becomes invalid, or VR session exits, immediately clear the effect.
- New shake during active correction must not stack multiple animations or sounds; honor cooldown.
- Render ghost with depth-tested transparent material (~25% opacity), reuse controller mesh, and restore GL state afterward.

## Implementation
The torso-side anchor recalculates live and neutral positions from the same headset transform each frame. The ghost retains the tracked quaternion. A tinted transparent pass reuses the Daydream OBJ (or procedural fallback) without depth writes and restores prior depth/blend state. The coil is generated once per frame and reused for both eyes. A deterministic 460 ms PCM spring is generated once and replayed through a static AudioTrack, with Activity lifecycle handling.

## Diagnostics
Log shake accepted/rejected reason, correction start/end, offset length, and interrupted correction in DEV logging. Keep STANDARD logs to significant accepted events.

## Tests
1. Head yaw changes during correction: target stays headset-relative.
2. Controller translation and rotation remain responsive throughout.
3. Ghost orientation tracks live controller, never a fixed world orientation.
4. Left/right eye transforms and depth are consistent.
5. Consecutive impulses do not retrigger within cooldown.
6. Tracking loss and scene exit cleanly cancel.
7. No GL blend/depth state leakage to the stereo renderer.

## Related player rig
Future player-rig work should keep eye origins, headset geometry, collision proxy, and optional mirror-renderable avatar separate. Collider must not alter stereo eye calibration.

## Intermittent pose-stream visibility (2026-10-12)

The **input safety timeout remains 750 ms**. If a BLE notification is delayed
past this limit, the physical controller must stop supplying new pointer
rays, target selection, held input or guest actions. A transient loss of
freshness must not make the controller **mesh** blink out at 750 ms, however.
The shell may retain the last *still, noninteractive* model until 2.25 seconds
after the last valid sample; it removes this mesh immediately for actual
disconnection, input-provider switch, invalidated tracking or explicit Gaze
mode. The translucent recenter ghost/tether is **not** held: it clears
as soon as active pose tracking becomes stale. Once real samples resume,
normal tracked model/ray update is immediate.

The GL frame captures one immutable `ControllerSnapshot` for both its
quaternion-derived ray and its handset model/buttons so sensor callbacks
cannot interleave two different quaternions in one visible frame. A malformed
snapshot does not discard the last good packet before its natural timeout.
Development logging records notification intervals of 300 ms or longer,
the start/end of short visual holds, and malformed quaternion rejection.
These logs distinguish sensor-notification delays from present/render stalls;
a held mesh alone does **not** establish an underlying Bluetooth disconnect.

Deterministic JVM regression source:
`app/src/test/java/io/github/mrcalzon02/reverievr/ControllerPoseVisibilityTest.java`.
Unit checks cover 750 ms input gating, 2.25 s visual expiry, prompt
disconnect invalidation, invalid/future timestamps and immediate recovery.
Device-level S9 Bluetooth streaming, compositor timing and display stability
remain open acceptance items.
