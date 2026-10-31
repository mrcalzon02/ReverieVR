# ReverieVR internal games manager — Beckenburg editor integration

**Implemented source checkpoint:** 2026-10-30. The upstream **owner-controlled** project is
[`mrcalzon02/The-Beckenburg-Project`](https://github.com/mrcalzon02/The-Beckenburg-Project)
and its editor source was imported from commit
`4637cce0a570c6c4d1b64c655ad610155618046a`.
The exact 36 Java 17 files and their Git blob hashes are pinned in
`tools/editor-kit/upstream-lock.json`.

## What is integrated today

The *portable* Beckenburg editor is part of the ReverieVR **desktop internal
games workbench**, reached through `tools/desktop/run.sh` (Linux/macOS) or
`tools/desktop/run.cmd` (Windows). The workspace has a live scene hierarchy,
editable primitive and licensed local OBJ blockout, Beckenburg's searchable
category-based palette, transactional history with failed-edit rollback,
transform inspector, region-aware document save/reload, and its existing
mouse-orbit wireframe viewport. The underlying documents remain the existing,
strict `REVSCENE\t1` source. The source adapter is
`tools/desktop/ReverieSceneProject.java`, which implements Beckenburg's
unmodified `EditorProject<String>` against ReverieVR's live document and
`EditorHistory`; there is no competing preview-only save model.

**Lighting is now included.** The additional three original Beckenburg
`dev.mrcalzon.lighting` classes (`LightModel`, `LightingModule`,
`LightingFrameProfiler`) are pinned and compiled alongside the editor, water
and weather modules. The internal games manager exposes a functioning Lighting
dock for each native/project scene; Sitka's separate environment editor also
includes that same dock. It offers presets, direction/beam shape, ambient,
source strength, reach, diffusion, flicker, cell size, cache/LOD flags and a
live preview calculated with the actual Beckenburg light falloff functions.
The editor saves an independently validated, atomic `.properties` file
under `build/desktop-workspace/lighting/<game-id>.properties`; Sitka's
lighting profile is shared between both editors under
`build/desktop-workspace/sitka-environment/lighting/sitka.properties`.
It intentionally does *not* change Beckenburg's global Java Preferences or
silently modify an Android game. These settings and the preview are
**desktop authoring only**, not authored light fixtures, occlusion, dynamic
shadows or renderer parity. Native light runtime adaptation is still a
separate verified milestone.

The Sitka project is available in the editor project selector even though it is
**not yet a playable game**. Sitka's region field creates separate safe,
area-named `build/desktop-workspace/scenes/sitka/<area>.rvscene` files. This
allows multiple local scene partitions instead of pretending 512 boxes equal
the entire city: **512 primitives per REVSCENE file** remain the bounded legacy
format. Each file is an authoring blockout, not assembled georeferenced terrain.
Choosing **Water and weather editor** for Sitka starts the existing real
`tools/sitka/SitkaEditor.java` (a desktop GUI) from inside the manager. That
editor now compiles *offline using the imported water and weather modules*
instead of depending on an adjacent Beckenburg checkout. Windows and Unix
launchers are supplied. Water/weather documents are authored under
`build/desktop-workspace/sitka-environment/`, independent of each region's
REVSCENE until the runtime-world georeferencing and authority bridge exists.

Red Ledger's editor profile includes bar/counter and stool *blockout* entries
using the same validated scene and history. Other native/classic game entries
reuse primitives/OBJ and history; they do not gain fictional game-specific
simulation inspectors. A palette selection makes an actual scene edit or opens
the existing OBJ importer; none is an inert button. The existing Test Chamber
**Stage for APK** action remains the only explicitly wired native scene staging
command and still requires user confirmation and an APK rebuild.

### Why this is not a castle renderer port

Beckenburg's full developer viewport currently relies on its game-specific
Java software renderer, castle geometry and collision. Its experimental PSX
suite design names runtime-adapter seams, but does not implement a ready-made
GLES2 Android engine. Therefore this integration **imports the portable kit,
water authoring and weather library**, not the Castle game or Blender assets,
and does **not** put Swing or Java 17 editor code into the Galaxy S9 APK.
There are no additional Android dependencies, APK assets or runtime memory/
thermal costs. The authoring tool requires Java 17+, including JDK for
compilation. Desktop binary size grows only with compiled Java authoring
classes and their Swing GUI; no heavy external dependency, network service
or separate executable binary is bundled. Source remains owned/controlled
by the same GitHub account, with exact upstream provenance retained.

### Keeping it up to date without breaking edited worlds

The vendored source copies under `tools/editor-kit/upstream/` are **verbatim**
and must not be locally patched. ReverieVR game-specific adapters live outside
that subtree. The script `tools/editor-kit/sync-upstream.py` supports:

- `python3 tools/editor-kit/sync-upstream.py`: offline integrity verification
  of the pinned 36 Git blobs (no network, no mutation);
- `--check-upstream`: query the current upstream `main` and list changed,
  new or removed portable/water/weather/lighting Java source files;
- `--apply`: **explicitly** fetch and SHA-verify changed/new files before
  atomically replacing *pristine* copies and advancing the pinned revision.
  It refuses local modifications, path conflicts, deleted upstream classes,
  truncated Git trees and checksum mismatches. This is an import, never
  a blind merge into native modules.

After any upstream refresh run `bash tools/desktop/test.sh`,
`bash tools/sitka/test-editor.sh` and
`bash scripts/verify-desktop-scene-bridge.sh`, review the complete source
diff, then commit verified compatible changes to `main`. Upstream changes
may need a **game-specific adapter** update; the sync tool deliberately
does not overwrite one. Neither a new CI/release gate nor a new GitHub
Action has been installed. Visual/art-style drift remains diagnostic; only
functional failures should block a release.

### Current real limits / next capability gates

An imported model in the wireframe preview is **not** an imported runtime
model. The Sitka GLES2 renderer remains source-only; it has no approved
geographic DEM/coastline asset or playable fishing. The Red Ledger runtime
currently consumes its existing authored native geometry, **not** editor
REVSCENE files. Multi-floor construction, arbitrary surface meshes, road
splines, terrain sculpting, props with physical collisions, real lighting
volumes, WYSIWYG game rendering, nav/crowd and simulation data inspectors,
prefab/scatter, world streaming and runtime playtest-in-place require proper
per-game adapters, runtime format migrations and verified game-side execution.
Do not treat this partial portable editor integration as delivery of those
unfinished features. Sitka also needs a georeferenced streaming/sector data
contract beyond the 512-part guide files before full-city production.

### Reproducible checks

`bash tools/desktop/test.sh` compiles the actual portable
`org.beckenburg.editor` classes and ReverieVR adapter and exercises scene
save/undo/redo/rollback, Sitka region isolation/path safety and Red Ledger
palette isolation, all without a GUI. `bash tools/sitka/test-editor.sh`
compiles the imported water/weather/lighting/editor classes and the actual Sitka GUI
source, then runs headless water placement persistence and weather snapshot
recovery. `bash scripts/verify-desktop-scene-bridge.sh` additionally tests
the existing Java/C++ REVSCENE exchange and bundled Test Chamber authoring
file. Passing Java compilation is not headset/graphics acceptance.

### Lighting verification notes

`tools/desktop/LightingAuthoringPanelTest.java` checks the real shared light beam and falloff math, performance preset visibility invariants, bounded frame profiler and the authoring panel’s atomic file save/reload/invalid-input rejection. Both the desktop Java 17 test scripts and the Sitka test scripts compile the actual imported lighting module and its shared Swing authoring panel. These are source/compiler checks, not Android lighting acceptance. The existing Test Chamber point-light shader is still separate until a native `LightModel` parity adapter is implemented and tested.

## Latest portable editor suite sync — 2026-10-30

**Upstream:** `mrcalzon02/The-Beckenburg-Project@2a16001ca76d47fdaaed6d927e5c02f81e11ef49` (`main` at intake).
ReverieVR now vendors **52 verbatim portable Java 17 sources** under `tools/editor-kit/upstream/`, pinned by individual Git blob hashes in `tools/editor-kit/upstream-lock.json`.
The refresh changed/added 17 files, including the revised palette; 35 previous source files were byte-identical. No upstream deletions were found.

The desktop workbench and the separate Sitka editor expose a shared **PSX authoring** menu:
- **Animation clips:** real keyframe/root-motion authoring, stepped/linear sampling, timing, preview and versioned save/load. No native game actor currently consumes clips.
- **Sound conversion:** local WAV PCM import and bounded mono 11,025/22,050-Hz 8/16-bit derivative export. It is not spatial sound emitter placement or an in-game mixer.
- **Billboard art:** source-preserving sprite/PNG conversion, palette/dither/size choices and compressed PNG output; no runtime billboards are automatically staged.
- **Campaign flow:** actual state/flag/dependency authoring and versioned save/load, without connecting to native mission state.
- **Mesh pipeline:** standalone OBJ/STL import, adjustable part-aware triangle reduction, warnings and explicit GLB export, not an automatic runtime asset installation.
- **PSX reference:** upstream offline authoring guide with optional links to the upstream wiki.

Five new `org.beckenburg.editor.ai` files are also imported as **game-agnostic, source-only AI rule inspection, decision replay and bounded aggregate telemetry**, but no ghost AI or Sitka NPC behaviors are wired to ReverieVR. The three previously imported lighting classes and existing water/weather classes remain unchanged; this refresh is not a new lighting renderer, spatial audio system or animation runtime.

Seven **upstream-derived functional test programs** are copied into `tools/editor-kit/upstream-tests/` and executed by `bash tools/editor-kit/test-ported-tools.sh` with offline lock integrity checking. The workbench and Sitka Java builds compile the new source and menu adapters. The existing game-content workflow runs these objective source/behavior checks; GUI display and real headset parity still require direct runtime acceptance.

**Boundaries:** this is desktop Java 17 authoring infrastructure, not packaged into the Galaxy S9 APK. Existing `.rvscene`, water, weather and lighting files are not auto-migrated. Exporting a WAV, GLB or animation does not imply the native Android/desktop engines consume it, and the editor viewport is not the live GLES2 renderer. Nothing from Castle maps, Beckenburg audio recordings, gameplay network logic or ghost assets was copied into Sitka. Actual Sitka terrain, lighting occlusion, authored spatial sound emitters, character animation clips, collision and runtime staging remain integration tasks.

**Imported test-fixture compatibility correction (2026-10-30):** `NpcAiTelemetryTest.java` from upstream `2a16001` reused `ignored` in an enclosing local and a nested catch; Java 17 compilation rejects the shadowing. The ReverieVR *test copy only* renames the catch-local to `ignoredCleanup`. All 52 pinned upstream production/editor source files remain byte-for-byte unchanged; test assertions, behavior and coverage are not weakened. CI run 41157258883 exposed the compile error.
