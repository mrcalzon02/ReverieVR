# Desktop authoring -> native Test Chamber scene bridge

**Status (2026-10-28):** Source-integrated portable parser and GLES2-ready primitive geometry, real Java/C++ cross-language checks. **Android APK build, headset rendering, and native desktop game hosting are not yet device-verified.** This does not replace the shell, ABI v1, assets catalog or existing module renderers.

## Producer and staging

- `tools/desktop/ReverieDesktop.java` edits `workspace/scenes/<module-id>.rvscene` through the same strict `SceneDocument.java` codec.
- Select **Procedural Test Chamber** in the project dropdown, place boxes/cylinders, then use **Stage Test Chamber for APK**. The editor saves the workspace copy and, after explicit overwrite confirmation, atomically copies the same scene document to `app/src/main/assets/native/test-chamber/scene.rvscene`. Staging does **not** install/update an existing APK or publish a release; the next complete APK build includes it.
- OBJ files are available as desktop-editor previews only; the staging action refuses scenes containing OBJ until a correctly licensed portable native OBJ pipeline is implemented. Do not silently substitute boxes for OBJ geometry or include third-party assets from a developer filesystem.
- The default bundled Test Chamber scene contains two small hand-authored primitives so that the reader has a real APK asset to consume without user export; it is explicitly a development-room prop layout, not the Daydream Home scene.

## Consumer and contract

- Host `reverie_native_asset_scope.h` grants **only** the `procedural-test-chamber` module access to exactly `native/test-chamber/scene.rvscene`, with maximum 150,000 bytes. No arbitrary source files, paths, other module resources or phone storage are exposed.
- During module creation, `test_chamber_module.cpp` reads that optional, allowlisted asset using the existing v1 `read_asset` callback. A missing asset keeps the original Test Chamber working. A present invalid/unsupported asset must fail with a diagnostic rather than render a fake approximation.
- `reverie_scene_document.h` accepts canonical `REVSCENE\t1` with a required valid environment and 0..512 pieces (ID/kind/asset and five numeric transform fields). Transform values are finite, scale is positive and bounded. Paths must be repository-relative `.obj` references with no traversal, though native geometry rejects `obj` until implemented. No user-facing file paths cross the ABI.
- Primitive mesh baking happens only on GL-context initialization/creation from parsed state, not inside each eye. Box geometry measures ±1 horizontal and [0..2] vertically before scale+yaw; cylinders have sixteen sides with caps. Interleaved `XYZUV` is appended **after** existing room/avatar/wardrobe buffer slices, and each eye draws the same cached range. The existing GL state-restore, texture and shell-owned interaction paths remain authoritative.
- This is a proof of an actual **game-side authored scene asset consumer**, not general native modules applying arbitrary editor modifications yet. Other games must opt in deliberately and pass their own performance/source/license gates.

## Tests and acceptance

Run `bash scripts/verify-desktop-scene-bridge.sh`. It compiles real repository Java classes and the workbench with JDK 17, runs Java codec tests and writes a fixture, compiles native tests with a C++17 compiler, tests that the exact Java output parses and tessellates natively, rejects bad input, reads the bundled sample, and checks host asset isolation.

Still open: Android NDK compile/link, physically render authoring edits on Galaxy S9, ensure actual GLES2 frame pacing/GL-state preservation, desktop player runtime, and full library/editor integration. Native-module ABI layout is unchanged.

## Integrated headset Workshop — 2026-10-30 implementation slice

The existing `procedural-test-chamber` hosted native module now owns a bounded
in-headset **VR Workshop** editing mode, accessible from Home's **TOOLS** shelf
as **VR Workshop / Editor** (and its existing Test Chamber entry). This
preserves the 15-game identity contract and uses the existing trusted JNI
host instead of introducing downloaded native execution or Java Swing on
Android. The `SECONDARY` action toggles editing and live native scene
preview; the shell retains Quick Menu, recenter, exit and recovery ownership.

The headset editor and desktop authoring share the real `REVSCENE\t1`
`reverie_scene_document.h` parser/primitive tessellator. The editor is
limited to 48 primitive boxes/cylinders per project, three isolated
native host save slots, 16 undo snapshots, live pointer-selected objects,
gaze/controller ray menu hits, and snapped pointer-drag Move X/Z, Rotate
and Scale operations. A changed scene is rebuilt in one VBO on edit
rather than being regenerated in each eye. Real Lato glyphs label the
tools, selection, project ID and unsaved state. The same scene is
rendered in both edit and playtest modes; playtest is a visual/runtime
scene preview, not general user-code execution or a full physics game.

Save and project switching use the existing host-scoped, atomically
written `read_save`/`write_save` services; malformed existing saves
refuse to load and never silently overwrite the previous project.
The first slot seeds from the allowlisted Test Chamber authored scene
when it has no save; later slots begin empty and can be edited in VR.
Pausing/exiting retries dirty saves; failed saves retain dirty state
and block project switches or leaving edit mode.

Input: `SECONDARY` enters Workshop from the chamber and saves/returns to
preview; click in-world menu entries to add box/cylinder, choose move,
rotate, scale, delete, undo, save, test and switch project; click an
existing object to select, then drag with the held primary action.
The shell locomotion and Quick Menu remain shell-owned; edit transforms
use the normalized pointer ray, not movement axes.

**Not yet delivered:** running arbitrary downloaded game scripts, full
Beckenburg desktop Swing UI within the headset, mesh/animation/sound
import into the on-device scene, generic cross-game asset sharing,
physics-backed playtest, Sitka terrain/world editing, multi-layer
prefabs, full game manager, export bundles, or a completed WYSIWYG
scene/renderer parity across other games. These require explicit
validated native adapters and content provenance. Actual S9
stereo/controller/thermal acceptance remains open until tested on device.

## Restricted in-VR object animation and single-slot project bundles (2026-11-04)

`app/src/main/jni/native/reverie_vr_project_motion.h` adds a versioned,
data-only `RVMOTION\t1` component codec. VR Workshop objects can be assigned
one **Spin**, **Bob**, or **Pulse** behavior with bounded speed and amplitude.
The native Test Chamber playback evaluates each binding on the existing
simulation clock and feeds the already-present GLES2 spin/offset/emissive
uniforms once per object per stereo eye. Editing shows unanimated transforms;
Play Test runs the composed scene with animation. This is real native motion,
not Java Swing or an uploaded scripting engine, and it does not yet import
Beckenburg's arbitrary keyframe animation clip format.

The second authoring menu page has the concrete controls **BOB, SPIN, PULSE,
CLEAR MOTION, SPEED +/-, AMPLITUDE +/-, NEXT PROJECT, SCENE TOOLS**.
The first menu page's tenth entry opens the second page; save and play-test
remain on the first page. The selected object HUD reports its actual binding.
Removing an object removes its binding in the same undoable transaction.
Undo restores both geometry and motion metadata; a completed drag is still
a single transform transaction, not one undo entry per frame.

Project saves are now a single, bounded `RVPROJECT\t1` envelope containing
the unchanged `REVSCENE\t1` document and a `RVMOTION\t1` document,
with strict byte-length sections, count limits and referential integrity.
Each slot is atomically written under its own `vr-workshop-project-N.rvproj`
host-scoped native save name (up to 65,536 bytes), never via arbitrary phone
storage paths. For compatibility, when an `.rvproj` is missing, the original
scene-only `.rvscene` slot is read as-is; a subsequent save writes `.rvproj`
without modifying or removing the legacy file. Present corrupt new-format
files fail closed rather than silently reverting to older files.

**Important limits:** the desktop `REVSCENE` parser cannot open an `.rvproj`
envelope directly; the embedded scene remains `REVSCENE\t1`. Export/import
bridges, fully authored keyframes, spatial audio, multi-light placement,
physics and general project scripts remain separate explicit integration
steps. Validate real controller bindings, text layout, performance and
two-eye parity on the Galaxy S9 before declaring headset usability complete.

## First authored Beckenburg-parity point light in headset (2026-11-04)

`reverie_vr_project_lighting.h` introduces a bounded, project-owned
light attached by stable ID to **one existing primitive**. With the
new **Light Tools** submenu inside VR Workshop, the user can attach
or remove the light, tune source strength, reach and ambient, and
move the anchor by editing the original object. The authoring rig is
visible in both eyes and during playtest; if the anchor bobs, the light
follows the object's actual animated world-space position. Removing
the anchor deletes its light as the same undoable scene transaction.

The light contribution mirrors Beckenburg's `LightModel.diffusionFade`
and `LightModel.attenuation` (omnidirectional angle and diffuse factors
set to one). A single additional set of GLES2 uniforms selects
authored lighting and sets power, radius and edge; **no light**
preserves the previous Test Chamber shader falloff and demo modes.
The source is not blocked by scene geometry yet: proper line-of-sight
occlusion, multiple fixture types and shadow maps remain open tasks.
The native shader remains designed around the Galaxy S9 GLES2 budget.

Project saves now use **`RVPROJECT\t2`**, a single atomic, versioned
envelope holding unchanged REVSCENE v1, RVMOTION v1 and RVLIGHT v1
sections with strict size, count and referential integrity checks.
`RVPROJECT\t1` motion projects and original REVSCENE-only host saves
remain accepted, without silent deletion or replacement; subsequent
successful edits migrate to the current version. Malformed light
components or a missing anchor reject the project rather than drawing
an invented lamp. Existing test coverage checks lighting math,
linkage, animation-follow, undo, migration and failed writes.

**Limitations:** one point light per project, no occlusion or directional
beams, no native audio emitter or authored sound placement, no real
Beckenburg keyframe clip playback or arbitrary scripting. A valid
Android package is not proof of headset button, text legibility or
sustained frame-rate acceptance.

## Bounded positional audio emitters (2026-11-04)

The Workshop gains a fourth **SOUND TOOLS** editor menu, with
**ATTACH SOUND, REMOVE SOUND, NEXT/PREV CUE, INTERVAL +/-, RANGE +/-**,
project switching and a return to Light Tools. One emitter can be
anchored to a scene primitive with a deterministic periodic trigger;
it follows the primitive's authored Bob motion in live Test mode.
Only cues **1–21** from the existing original preloaded shell PCM bank
are selectable. No external files, arbitrary WAV samples, Android file
paths, downloaded executable code or network fetch is accepted.

The native runtime sends an append-only `request_spatial_sound` callback
containing a validated cue and bounded left/right gains. A distinct
two-event `ReverieNativeSpatialSoundQueue` keeps authored sounds separate
from existing game audio. The shell supplies actual headset/locomotion
listener position and horizontal right direction independently of the
controller pointer; the C++ authoring runtime computes distance and stereo
pan. The shell drains the queue **once after update**, not in either eye
render, and Android plays existing preloaded `AudioTrack` PCM at the
quantized stereo gains. No new waveforms are synthesized in the frame loop.

The project envelope becomes `RVPROJECT\t3`: one atomic payload
containing unchanged `REVSCENE\t1`, `RVMOTION\t1`, `RVLIGHT\t1`,
and `RVSOUND\t1` components. All previous project V1/V2 and scene-only
saves remain readable; successful saves upgrade to V3, without touching
the old original slot. Unknown cues, dangling emitters, invalid numbers,
oversize files and unexpected trailing bytes fail closed. Object deletion
removes emitter attachment in the same undoable transaction.

**Scope limits:** one emitter per project; finite authored periodic triggers
only; shell-generated cues, not imported WAV/MP3 content; basic distance
and stereo amplitude panning, not HRTF or convolution reverb. No physics
occlusion, general event graph or Beckenburg sound-lab WAV staging yet.
The optional append-only host/input tails do not alter existing native ABI
offsets, and existing modules keep their original sound queue.
Device speaker/headphone spatialization and controller feel remain
unverified until a physical headset run.
