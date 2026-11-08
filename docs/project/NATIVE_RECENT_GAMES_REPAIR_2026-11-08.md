# Recent native games: persistence, recovery and readability repair

## Scope and status

This repair covers the integrated native Breakwater Battery, Guess the Door
and Procedural Test Chamber modules. Source work was prepared against
`43a1626d40788067128915aa77deb356434ae9b3` in an isolated checkout for serial
integration by the repository owner. It does not change Sitka, companion
applications, admission status, deployment workflows or third-party assets.
These remain development projects pending the physical and package gates
below. The root host change supplies the append-only, size-gated
`prepare_exit` API used by all three modules.

## Implemented behavior

| Module | Failure closed by this change | Current save and recovery contract |
| --- | --- | --- |
| Breakwater | A failed read appeared to be an empty slot; failed writes still activated actions; earned wave/victory state waited for later interaction; failed reset could resurrect progress. | Distinct missing/invalid/I/O states. Valid legacy 80-byte Shop records migrate on the next change to a checksummed 88-byte schema-3 Shop/Victory record. Wave results are saved in Update. Writes remain dirty and visible until acknowledged. Retry consumes the action edge. Combat reload starts at the previous Shop/wave start. Two-confirmation reset retains clear-only intent after an ambiguous write. |
| Guess the Door | No persistence; one New Seed press also pulled a lever; clues and instructions were left-clipped. | Checked 36-byte schema-1 seed/progress record. Confirmed choices persist their deterministic safe outcome, including falls and victory, while animation/input state remains transient. Two-second dirty retry, lifecycle flush and safe exit. Invalid bytes are backed up before replacement; unreadable originals and occupied unrelated backups are retained. New Seed consumes its edge. |
| Test Chamber | Bad slot zero prevented launch; corrupt alternatives prevented reaching usable projects; editor text/hit layout was outside the initial view; pause could leave the display stale after save failure. | Bad default slot falls back to an existing valid alternative before a new slot. All bad slots produce an actionable read-only Retry Projects screen. Switching skips unreadable alternatives. Dirty current edits block switching, Play Test and ordinary exit until acknowledged. Quiescent retry and pause/exit flush update visible status. |

The three `prepare_exit` callbacks perform no GL work. They return zero
while their active save/read recovery is unresolved, retaining the instance
for a later bounded retry. A host write error may mean replacement bytes
are already visible but directory durability was not acknowledged. Tests
exercise that case. An explicit host exit override must not promise an
on-disk rollback; ordinary destruction remains guarded by the callback.

Breakwater's damage banner and guidance now occupy separate console rows.
Guess uses the real centred Lato text API with fitted widths. Test Chamber
renders every tool inside the forward view and shares menu constants with
ray hit testing. Unsupported arrow/semicolon glyphs were replaced with
supported text. All three copy only the supplied input prefix, and
Guess/Test Chamber require action-button release after lifecycle pause.

## Regression evidence

The source tests are checked in; the following local commands completed
with exit zero on the repair source:

```bash
bash scripts/verify-guess-the-door.sh
bash scripts/verify-recent-game-persistence.sh
g++ -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror \
  -Iapp/src/main/jni/native app/src/test/native/breakwater_campaign_test.cpp \
  app/src/main/jni/native/breakwater_campaign.cpp -o /tmp/breakwater-campaign
/tmp/breakwater-campaign
```

The existing `reverie_vr_workshop_test`, `reverie_vr_project_motion_test`,
`reverie_vr_project_lighting_test` and `reverie_vr_project_sound_test` were
also individually compiled with strict GCC diagnostics and passed. Their
malformed-slot assertions still check direct `LoadSlot` rejection without
partial replacement; project cycling now intentionally skips that slot.

The new tests exercise production module code, not reimplemented adapters:

- `breakwater_persistence_test.cpp`: invalid and I/O slots cannot be
  overwritten by selection; write failure prevents wave launch and duplicate
  purchase, including coincident automatic retry and fresh primary/secondary
  edges; pause and safe exit retry; actual Update earns and stores a wave
  result and final victory; legacy migration and checksum rejection; reset
  errors before and after replacement never resurrect the old campaign.
- `guess_the_door_save_test.cpp`: checked format, bounded fields, every byte
  corruption, truncation, safe phase normalization, falls and final victory.
- `guess_the_door_persistence_test.cpp`: actual primary input, save/recreate,
  New Seed consumption, pending reset after ambiguous error, lifecycle
  callback wiring, held input, read failure, invalid-byte backup, failed and
  full backup recovery, rocker selection with a pointer, and text bounds for
  64 deterministic seeds and inspection/recovery screens.
- `reverie_vr_workshop_recovery_test.cpp`: existing-valid-first fallback,
  missing-slot fallback, unchanged bad bytes, all-bad retry, dirty switching
  guard, applied-write error and every visible menu row's hit region.
- `test_chamber_persistence_test.cpp`: actual Create with packaged scene,
  tool input, failed pause/exit saves, applied-write error, quiescent retry,
  held-button guard, recreate, all-bad recovery and font bounds for all tool
  pages and the recovery screen.

Breakwater's actual module test also passed AddressSanitizer and
UndefinedBehaviorSanitizer using a heap input allocation exactly the size
of the pointer prefix, omitting the later listener tail. Base-only input is
also safely rejected by this pointer-driven title. LeakSanitizer cannot run under the
environment's tracing setup, so the successful memory-boundary run uses
`ASAN_OPTIONS=detect_leaks=0`; no leak-acceptance claim is made.

The new runner requires native GLES2 and zlib development libraries.
Guess's lifecycle binary uses `-O0` to avoid a pre-existing GCC
format-truncation warning in the shared GL diagnostic helper. Test Chamber
keeps only OpenKTG's existing anonymous-struct and unused-static-helper
warnings nonfatal. All other runner diagnostics remain errors. These
third-party/shared warning sources were not modified by this lane.

## Actual graphics evidence

Real EGL/GLES module draws were captured before and after the change with
Mesa software rendering (`OpenGL ES 3.2 Mesa 25.0.7-2+deb13u1`). The final
capture uses both eyes with a 64-mm stereo separation and actual packaged
assets. Guess and Test Chamber use a fixed 80-degree vertical reference
projection at 512 x 512; the existing Breakwater probe uses a 1.25 projection
scale (77.32-degree vertical field) at 1280 x 800. Guess captures
Ready, Victory and Recovery; Test Chamber captures Showcase, Workshop and
Recovery. Breakwater exercises all three stations, visible trajectory
pixels in both eyes, turned console/rack/shore views and context recreation.
Renderer return values and GL errors are checked. Both eyes consume one
logical state. The pictures are read-back framebuffers, not illustrations.

The work checkpoint retains exact compiler/runtime argument arrays,
exit codes, driver logs, PPM originals, lossless PNG conversions and labelled
before/after comparisons under `repair-work/recent/evidence`. These paths
are execution evidence outside the repository; the owner preserves them
with the integrated repair checkpoint.

## Remaining release gates

This lane proves the local source behavior and its graphics fixes. It does
not prove Android package integration with the host's new exit barrier,
real storage power-loss behavior, headset/controller usability, stereo
comfort, audio mixing, sustained frame timing or thermal/battery behavior.
Those gates still require the integrated package and the reference device.
Breakwater's intentional wave-start resume must be clear to players.
Guess's requested approved door art and licensed spoken line remain content
gates. Test Chamber's complete authoring workflow still needs a physical
controller session, including all tools, save/project switching and shell
recovery. The normal project opener uses its documented default/first valid
slot policy; it does not persist a separate last-selected-project index.
