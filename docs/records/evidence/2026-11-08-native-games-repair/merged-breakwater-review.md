# Breakwater merged audio and persistence review

## Result

The merged production source passes the strict audio gate, the recent-game
persistence gate and a fresh real EGL/GLES Breakwater render probe. This
review found no new production save-loss defect. The returned patch changes
only two test files and applies cleanly to the authoritative working tree.

The snapshot was copied from the authoritative checkout at HEAD
`3af395812fc062505e6afa05ff3a06063198189d`, including its 169 uncommitted
changed/new paths. `snapshot-manifest.json` records that copy. The exact
production hashes used for verification are in `production-hashes.json`;
all nine selected module, campaign, audio and shared ABI/queue hashes match
the authoritative tree. The Breakwater module hash is
`615069133b251a3d6bf7ccaa61c2daccbb88706c51a38a67d656d62b1fc26fba`.

## Fixture changes

`breakwater_audio_test.cpp` now supplies valid read/write services by
default, retains acknowledged bytes, and validates the actual checked
88-byte Shop/Victory format. Its explicit unavailable-service case still
removes the read callback and asserts failure, dirty retention and blocked
exit/wave launch. It no longer treats a save error as permission to leave
the summary or launch a wave. Terminal reset now asserts a separate audible
confirmation, held-button rejection and a second released press.

All weapon, impact, outcome, queue-budget, spatial-sound and lifecycle
assertions remain. Additional production-Update assertions verify:

- Failed purchases emit failure without a purchase or page-success cue.
- A successful automatic retry coinciding with both fresh action buttons
  acknowledges the pending save without buying again or starting a wave.
- A purchase first failing in the current frame prevents a simultaneous
  secondary launch and its sound.
- Failed debrief persistence retains the summary; recovery acknowledges
  only the save; a separate fresh action starts one audible wave.
- A reset whose replacement bytes became visible before an error remains
  pending and emits no false restart success; acknowledged retry clears once.
- Earned wave/victory sounds remain audible when their checkpoint fails,
  while progress stays dirty and ordinary exit remains blocked. A retry
  stores a checked terminal victory record before accepting reset input.

`breakwater_persistence_test.cpp` adds one `audio_frame.Flush(&host)` after
its direct `Interact` fixture call. The audio overhaul dispatches queued
feedback at the end of actual Update, so a direct helper call must supply
that explicit boundary before its existing host-feedback assertion. Every
previous persistence assertion remains intact, including short input,
invalid/read-failed slots, duplicate-purchase protection, same-frame
secondary launch protection, ambiguous resets and earned checkpoints.

## Executed evidence

| Gate | Result | Evidence |
| --- | --- | --- |
| `bash scripts/verify-breakwater-audio.sh` | Exit 0; **2,484 checks** | `evidence/audio-20261108T191639556697.log` |
| `bash scripts/verify-recent-game-persistence.sh` | Exit 0; all four actual-module/core programs pass | `evidence/persistence-20261108T191643736151.log` |
| Strict merged Breakwater GLES build | Exit 0 with `-O1 -Wall -Wextra -Wpedantic -Werror` | `evidence/gles/build.log` |
| Actual merged GLES run | Exit 0 | `evidence/gles/run.log` |

Initial failing fixtures are preserved: audio failed check 77 because the
old fixture omitted save services; persistence failed its immediate
feedback assertion because dispatch had moved to the audio frame boundary.
Those failures were repaired in tests without relaxing production guards.
The exact commands, environment, exit codes and log paths are in
`evidence/commands-and-results.jsonl`.

The new graphics proof uses Mesa OpenGL ES 3.2, actual packaged assets,
1280×800 framebuffers, a 77.32-degree vertical reference projection
(`projection[5] = 1.25`) and 64-mm eye separation. It loads 16 materials,
four approved horizons and five BWM1 models. Guide-on/off comparisons find
2,255–2,313 visible guide pixels in each eye across all three stations.
Immutable coast assertions, repeated-eye simulation checks and actual
context destruction/recreation pass. Fifteen PPM files and their lossless
PNG counterparts preserve six station eye views, six turned eye views and
three convenience aliases. Fresh console and battlefield images were
visually inspected; damage and guidance do not overlap.

These are source, host-queue and software GLES results. No physical audio
playback, Galaxy S9/controller session, Android package acceptance, sustained
thermal/performance session or real-device power-loss claim is made.

## Integration artifact

`merged-audio-fixtures.patch` contains only the audio test and the one-line
persistence fixture flush. Its SHA-256 is
`98de4a8d8166ba6f7f9b52d39fee20d1259c9050525cab7321c88aef461341f4`.
`git apply --check` against the authoritative working tree returned zero.
`patch-manifest.json` records before/after hashes. No repository commits,
staging, push, workflow execution or authoritative edits were performed by
this review lane.
