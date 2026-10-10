# ReverieVR Internal 3D File Viewer

**Status:** OBJ import and stereo scene integrated into the Android shell source on `main`; build, emulator and Galaxy S9/Daydream acceptance remain unverified.

## Purpose and rendering contract

A 3D file opens as **geometry in a real stereoscopic world**. It is not a video on the media/DOS flat panel. The host passes separate Cardboard eye transforms to the GLES2 mesh renderer. Native applications and the 3D viewer use the **entire physical eye viewport** (scale 1.0), while ordinary shell/flat-media presentation retains its comfort-calibrated 0.738 eye-content scale. The shell power HUD remains an independent overlay.

The immersive viewer is presently a **shell-native renderer** (`ObjMeshViewerRenderer`), rather than a separately packaged ABI module, because it must integrate with Android's document picker and app-private imported files without broadening the native module sandbox or adding arbitrary filesystem access to frozen ABI v1.

## Working source-level path

1. On VR Home, choose **IMPORT / REPLACE OBJ MODEL** in the center panel. The shell requests the Android `ACTION_OPEN_DOCUMENT` picker by returning safely to the 2D Activity. Select a `.obj` file.
2. The 2D importer copies at most 8 MiB into temporary app-private storage, parses and validates the OBJ before atomically replacing the previous `model-viewer.obj`.
3. Re-enter VR and choose **OPEN 3D MODEL — IMMERSIVE VR**. The Open action is unavailable until a model has been imported; Import/Replace remains available after an import.
4. The OBJ opens as actual triangles (no billboard, screenshot or textured quad). The viewer recenters and normalizes the object to a visible size; face shades are computed once on loading. Move the Daydream touch surface to orbit yaw/pitch. The two rocker inputs zoom in/out within a bounded range. A select/click rotates by 30 degrees. Back returns Home; Quick Menu recovery remains shell-owned.

## Current geometry subset

- Wavefront OBJ `v` and `f` geometry, positive/negative vertex indices, fan triangulation for convex polygons, slash-delimited face indices.
- Ignores `vt`, `vn`, `mtllib`, `usemtl`, groups and comments; no texture/material loading yet.
- Fail-closed import on invalid geometry, missing indices, empty data, nonfinite coordinates or excessive data.
- Source caps: 8 MiB file, 8192 characters per line, 200000 vertices, 250000 triangles, 128 corners per face. Further device-budget refinements may tighten these numbers.

## Development and acceptance boundaries

- **Not yet implemented:** glTF 2.0/GLB, binary/ASCII STL, FBX, animations, textures/PBR/materials, hierarchy/scene graph, multi-file dependency resolution, object selection, measurement/grid, reset framing UI, pan, zoom from non-Daydream devices.
- **Not yet proven:** Android/Gradle compilation, import and Activity round trip, signed release, stable per-eye viewport coverage, controller comfort/gesture polarity, frame-rate and thermal performance on Galaxy S9.
- The separately reported Native Apps tab crash remains an open root-cause defect and is **not** fixed by the renderer viewport policy.

Never label a format supported or a control usable until it has an actual parser/renderer/input path and verification.
