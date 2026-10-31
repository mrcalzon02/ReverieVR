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
