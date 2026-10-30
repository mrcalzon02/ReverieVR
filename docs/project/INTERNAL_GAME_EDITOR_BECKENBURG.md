# ReverieVR internal games manager — Beckenburg editor integration

**Implemented source checkpoint:** 2026-10-30. The upstream **owner-controlled** project is
[`mrcalzon02/The-Beckenburg-Project`](https://github.com/mrcalzon02/The-Beckenburg-Project)
and its editor source was imported from commit
`4637cce0a570c6c4d1b64c655ad610155618046a`.
The exact 33 Java 17 files and their Git blob hashes are pinned in
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
  of the pinned 33 Git blobs (no network, no mutation);
- `--check-upstream`: query the current upstream `main` and list changed,
  new or removed portable/water/weather Java source files;
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
compiles the imported water/weather/editor classes and the actual Sitka GUI
source, then runs headless water placement persistence and weather snapshot
recovery. `bash scripts/verify-desktop-scene-bridge.sh` additionally tests
the existing Java/C++ REVSCENE exchange and bundled Test Chamber authoring
file. Passing Java compilation is not headset/graphics acceptance.
