# Controller ghost recenter — implementation contract

Status: Planned; detector threshold tightening committed separately. No claim of runtime completion.

## Interaction
On one confirmed sharp-shake gesture, play one short spring-like audio cue, show a translucent duplicate of the existing Daydream controller model at the corrected headset-relative pose, and draw a thin spring/tether between live and ghost positions. Do not use a target ball.

## Tracking invariants
- Preserve raw controller tracking and input throughout. Never animate or mutate sensor samples.
- Maintain independent tracked orientation; the ghost shows the same tracked orientation at the corrected position.
- Blend only the headset-relative calibration offset toward zero over ~1.5 seconds using monotonic easing, no overshoot.
- Each frame recompute live and ghost world transforms from current headset pose. Never freeze the target in world space.
- Spring and ghost must render for both eyes with proper depth and stereo projection.
- If controller disconnects, tracking becomes invalid, or VR session exits, immediately clear the effect.
- New shake during active correction must not stack multiple animations or sounds; honor cooldown.
- Render ghost with depth-tested transparent material (~25% opacity), reuse controller mesh, and restore GL state afterward.

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
