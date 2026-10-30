> **Lighting module import (2026-10-30):** Beckenburg's lighting settings, falloff model and frame profiler are now pinned locally with the editor/water/weather Java sources. The internal game manager has an actual Lighting inspector and per-project authoring profile persistence; the separate Sitka editor has the same Lighting tab. Neither claims to drive Android game illumination yet. See [Beckenburg editor integration](../../docs/project/INTERNAL_GAME_EDITOR_BECKENBURG.md).

> **Beckenburg shared editor integrated (2026-10-30):** The internal games workbench now builds against a pinned, locally bundled Java 17 Beckenburg portable editor kit. Its actual search/category palette, transactional undo/redo with rollback, object duplication, snap-to-grid and safe per-project scene files are wired into the workbench. Sitka has a named-region authoring selector and a launcher for the real bundled water/weather editor; Red Ledger has game-aware primitive blockout palette entries. No neighboring Beckenburg checkout is required. This remains a desktop authoring preview, not an in-headset editor or automatic native game renderer. Complete implementation/limitations: [internal games manager integration](../../docs/project/INTERNAL_GAME_EDITOR_BECKENBURG.md). Windows: `tools\\desktop\\run.cmd`, Linux/macOS: `bash tools/desktop/run.sh`.

# ReverieVR desktop tooling

**Two separate executable desktop tools now exist:**

- **Playable Home prototype:** `tools/desktop/player/run.sh` or `tools\\desktop\\player\\run.cmd`. Offline Java 17 Swing Home room, in-room Native Games panel, movement and keyboard, and working Pong/Snake/Breakout. See `player/README.md`. Other games and full parity remain unported.
- **Scene-authoring workbench:** `tools/desktop/run.sh` or `tools\\desktop\\run.cmd`. The existing mesh/scene editor and Test Chamber staging pipeline described below.

## Existing scene-authoring workbench — functional slice

**This is a real standalone desktop scene-authoring program**, not yet the full desktop ReverieVR player or a mirror of its Android renderer. It uses Swing/Java 17, has no external editor dependencies, does not run Android, and compiles against **the authoritative** existing `HomeEnvironment`, `GameModuleRegistry` and `ObjMeshReader` from the ReverieVR source tree. No duplicate game catalog is maintained.

## Run

Checkout `mrcalzon02/ReverieVR`, install a **JDK 17+**, and from the repo:

- Windows: double-click `tools\desktop\run.cmd` or run it from Command Prompt.
- Linux/macOS: `bash tools/desktop/run.sh`.
- To use a custom workspace: `run.cmd C:\my-reverie-work` or `bash tools/desktop/run.sh /path/to/workspace`.
- Test the portable scene codec and compilation: `bash tools/desktop/test.sh` (or run equivalent `javac --release 17` + `SceneDocumentTest` on Windows).

All builds and documents are inside `build/`, which is already excluded from version control. No network, Steam, headset, Beckenburg source checkout or Android SDK is required for the desktop workbench.

## Actually working

- Select the Home or any *real registered* native/classic game from the canonical source catalog. This does **not** launch the game or claim its desktop module is built.
- Choose one of the three real canonical Home environment IDs.
- Place boxes and cylinders as editable construction guides. Select objects in the scene list or by clicking their projected centers; adjust X/Y/Z/yaw/scale with the inspector; drag to orbit and scroll to zoom. Delete objects, undo/redo, reload or save.
- Import repository-local **Wavefront OBJ** files into the scene and preview their triangles with the repository's bounded `ObjMeshReader`. External files must be placed in the source checkout first; path traversal, symlink escapes and oversized meshes are rejected. Importing is not a license grant and doesn't automatically redistribute the mesh.
- Per-project scene files are atomically saved as `build/desktop-workspace/scenes/<module-id>.rvscene` by default. A strict, bounded `REVSCENE\t1` text codec gives deterministic serialization and rejects malformed/object-count/path/transform input. Changes are not silently applied to APK content.

## What it cannot do yet

The preview draws original OBJ triangles using a lightweight Java2D wireframe viewer, **not** the Android GLES renderer or the actual native game visuals. Saved scene authoring data are **staging assets**, not loaded by the Android native modules yet; playable desktop games, emulator/DOS runtime, Home visual parity, packaged installers, OpenXR and SteamVR are later milestones. No misleading Play buttons or false availability indicators are exposed.

The next integration step is a shared native `REVSCENE` reader and explicit module resource binding, followed by a desktop window/graphics host that can execute the existing ABI v1 modules on Windows/Linux/macOS without compromising Android. The complete target is defined by `docs/project/DESKTOP_PLATFORM_EXPANSION.md`.

## Native Test Chamber authoring pipeline (2026-10-28)

Select **Procedural Test Chamber** in the project dropdown. Edit a box/cylinder layout, then choose **Stage Test Chamber for APK**. The command validates and saves your workspace scene, prompts before replacing existing bundled content and writes the same `.rvscene` into `app/src/main/assets/native/test-chamber/scene.rvscene`. After rebuilding/reinstalling the **entire ReverieVR APK**, the native Test Chamber reads the asset through an exact scoped host callback, generates real GLES2-ready meshes once per context and draws them in both eyes. This is not a live link; it does not update an already-installed APK.

The native runtime intentionally rejects OBJ-based scenes because desktop OBJ wireframe preview is not yet a licensed game-time OBJ mesh loader. The importer continues to work inside the desktop editor, but the Stage button will explain why such a scene cannot currently be used in Test Chamber. No fake primitives are substituted. See [native scene bridge](../../docs/native/SCENE_BRIDGE.md) and verify it without running the Android app using `bash scripts/verify-desktop-scene-bridge.sh`.