# ReverieVR desktop development workbench — initial functional slice

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
