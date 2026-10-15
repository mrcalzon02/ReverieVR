# 2026-10-14 — Doom fullscreen optical fit, battery position, horizontal cursor recovery

**State:** authoritative source correction; Android APK + Galaxy S9 / Daydream acceptance pending.

Three narrowly scoped user-reported corrections, with no headset pose, lens profiles,
IPD, power-artwork scale, shell input routing, or unrelated game changes:

1. **DOS fullscreen optical fit.** `VrShellRenderer` previously used content
   scale 1.0 for full-screen DOS, presenting the game directly against each
   Cardboard eye's lens-visible limit. Preserve the independent complete raw
   per-eye clear and identity NDC full-screen guest image, with no VR room,
   menus or world panel; render that fullscreen guest inside an 80% centered
   sub-viewport of each raw eye, leaving a 10% black margin per side.
   Native games/video/model viewer still use full raw eyes; ordinary room
   content retains its 73.8% scale. Screen is NOT moved back into the room.

2. **Charge HUD translation only.** Both battery arcs, their percentage labels
   and any look-up reveal move together by 15% of full-eye width to the left
   and 15% of full-eye height upward: x -0.30 NDC; y +0.30 NDC.
   Extra downward NDC offset changes 0.40 -> 0.10 while the prior 0.20
   base down remains. Inward NDC offset changes 0 -> 0.30.
   No scale, texture crop, arc artwork, colors, percentages or adaptive mode
   logic changed.

3. **Doom and DOS cursor horizontal guest polarity.** The 2026-10-14
   `48475333` edit reversed `HEAD_YAW_DELTA` from +650 to -650 for the
   Doom-shareware and DOS cursor profiles; device feedback reports left/right
   inverted again. Restore +650 only for those two. Keep the working
   Doom held up/down keyboard pitch binding and every other input profile
   unchanged; no Cardboard physical view-matrix change.

Follow-up correction (same date, separate source commit): The fullscreen
renderer initially centered the 80% inset but still stretched the DOS texture
to that inset's pixel aspect. The shared `RetroDisplayMode` fit calculation
now centers the recorded DOS display aspect inside the inset without
discarding any source pixels or reallocating per eye. The 320x200 Doom guest
uses 4:3 and other DOS modes use their own catalog aspect. Added focused
host-side fit regressions; device validation is still required.

Camera-driven DOS input follow-up: the 3D scene already receives a
world-to-eye matrix from Cardboard with correct orientation. The DOS
controller previously used Cardboard's separate Euler yaw/pitch values,
which can couple near vertical gaze; earlier patches flipped profile
signs instead of using the known-good camera basis. The shell now emits
right-positive yaw from the 3D camera's projected right vector and
up-positive pitch from its forward Y basis, including when looking
straight up or down. The same consistent +X/-Y mapping is used by all
built-in headset mouse profiles; Doom retains its absolute pitch-driven
Up/Down keys because original DOS Doom has no free vertical look.
Neither optical view transforms nor 3D scene orientation were changed.
Added synthetic 3D camera pose coverage across yaw/pitch combinations
including vertical poles plus profile input polarity tests. Device test
remains the final arbiter.

Validation evidence: synchronized existing JVM test assertions for Doom pitch,
horizontal-only sign, DOS touch+head relative mouse, and HUD geometry.
Source read-back and commit verification only until an actual Gradle compile,
signed APK and S9 controller/stereo run prove runtime behavior. A physical
test should confirm entire Doom top HUD/face and edges are comfortably visible
while no in-room DOS menu overlays immersive display, lateral turns follow
head direction, vertical navigation stays correct, and charge bars remain visible.
