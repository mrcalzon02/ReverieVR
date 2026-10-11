# Original (2015) Steam Controller mesh pipeline

## Input and licensing
Use **Valve's March 24, 2016** separate-part STL ZIP, linked from:
https://steamcommunity.com/app/353370/announcements/

Do **not** use the 2026 second-generation Steam Controller/Puck geometry.
The Steam announcement states Creative Commons terms but also says to contact
Valve before selling creations. Verify the precise license inside the archive
and the project's distribution rights before checking Valve geometry or
derived meshes into the public repository. Do not treat an archive download as
approval to bundle it.

Extract the ZIP into a local (untracked) folder. Inspect it for a license,
a part list, and the model's actual coordinate units. Preserve the original
ZIP and compute its SHA-256 in the project's asset-provenance record.

## Offline conversion

```bash
python tools/steam_controller/convert_stl_parts.py /path/to/extracted-stl /path/to/converted --target-faces 12000
```

The converter:
- reads ASCII or binary STL;
- preserves separate parts and their relative model coordinates;
- welds nearby vertices using deterministic grid quantization;
- removes degenerate and duplicate triangles;
- writes one OBJ per part and `conversion_manifest.json` containing source hashes,
  vertex/face counts and original bounding boxes.

**Limitation:** quantization is a first-pass LOD reduction, **not**
topology-aware decimation. It may introduce cracks or impair round trackpads
and tiny buttons. Preview before using assets; use a curvature-preserving
mesh simplifier for final runtime models. It does not generate UV textures,
normals, physically meaningful material colors or controller button mappings.
No render mesh is currently bundled with the app.

## Planned runtime link
Once license-reviewed source geometry and an optimized mesh are available,
build an explicit stable part-name mapping for:
left/right pad, pad click, thumbstick, face buttons, shoulders, triggers,
rear grips, shell and system buttons. Overlay input event visualization from
`HidInputInspector` and raw HID only after those events have been observed.
Never animate an input based on guesses about a part name.

## Verification gates
1. Source archive SHA-256, source license and part inventory captured.
2. Each part imported with correct orientation and relative coordinates.
3. Mesh continuity, face normals, part positions and silhouette visually checked.
4. Input part-map events validated on Galaxy S9.
5. S9 GLES2 stereo performance and APK-size impact measured.
6. Redistribution approved before vendoring converted Valve meshes.
