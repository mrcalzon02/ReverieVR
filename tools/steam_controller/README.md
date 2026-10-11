# Original (2015) Steam Controller mesh pipeline

Use Valve's March 2016 original-controller separate-parts STL archive, not
2026 Steam Controller/Puck geometry. See
[`STEAM-CONTROLLER-MESH-PROVENANCE.md`](../../docs/records/STEAM-CONTROLLER-MESH-PROVENANCE.md)
for exact URLs, SHA256, licensing and real-asset verification.

## License and ownership

The inspected `LICENSE.TXT` specifies **CC BY-NC-SA 4.0**, copyright 2016 Valve
Corporation. Attribution, NonCommercial and ShareAlike apply to derivatives;
commercial products require contacting Valve. Preserve the full license,
source link and modification notice with shared packages. No Valve geometry
or derived meshes are vendored into ReverieVR Git. The independently authored
converter does not relicense assets or grant commercial permission.

## Explicit local importer

Python 3.12 and Node 24 were used. Install offline tooling locally; none of
these Python/Node/WASM dependencies run on Android:

```text
python -m pip install -r tools/steam_controller/requirements.txt
npm ci --prefix tools/steam_controller --ignore-scripts
python tools/steam_controller/prepare_local_assets.py original.zip .local/steam-controller/import --accept-noncommercial-sharealike --target-faces 12000
```

The importer verifies the pinned archive/license hashes, bounds extraction,
rejects traversal/symlinks, preserves an unchanged ZIP and CAD under `source/`,
and creates separate `converted/` and `assets/` directories. It does not enable
APK packaging or publish a release. Use a new empty output directory. Source
files are read-only; stale output directories are refused.

For already extracted CAD, use non-nested source/output directories:

```text
python tools/steam_controller/convert_stl_parts.py source/cad converted --target-faces 12000
python tools/steam_controller/preview_parts.py converted
python tools/steam_controller/verify_outputs.py converted
python -m unittest discover -s tools/steam_controller -p test_pipeline.py -v
```

`convert()` is callable without command-line globals. Binary STL is memory-mapped;
ASCII is also supported. Nonfinite/truncated data is rejected. Exact welding
and degenerate/duplicate-face cleanup precede topology-aware meshoptimizer QEM.
Minimum per-part allocations prevent dense thumbstick tessellation starving
small controls. Total output must fit the global budget. Original name stems
plus relative-path hashes prevent collisions.

When QEM cannot meet budget within 2% relative error or has boundary/nonmanifold
edges, the converter records a filled-voxel/marching-cubes approximation and a
second QEM pass. Resolution is recorded in mm. Sub-voxel details/interior cavities
may change. Remaining topology flags are reported, not declared repaired. Seven
parts in the inspected 12k candidate retain flags; use rendering and primitive
colliders, not manufacturing or watertight-volume operations.

## Outputs and coordinate contract

- Individual named OBJ files retain source mm coordinates and relative positions.
  A source file containing an assembly remains one named part.
- `conversion_manifest.json`: hashes, counts/budgets, source/output bounds,
  repair resolution, topology and empty input mappings.
- `controller.glb`: named assembly in meters, +X right, +Y up, -Z forward.
- `assembled.obj`: same meter-space assembly, usable in the existing OBJ viewer.
- `controller.rvmesh`: precomputed flat normals and interleaved positions/normals.
- Orthographic/isometric assembly and individual-part PNG previews.

Runtime transform: `(x,y,z) -> (x,z,-y)*0.001`, preserving handedness. Apply once.
Individual OBJ files intentionally remain source-space. Parts are not independently
centered. Actual hinge pivots require review and are not invented from bounds.
GLB colors distinguish parts and are not measured physical materials.

Binary format is little-endian: 8 bytes `RVSC0001`, uint32 part count; per part,
uint32 ASCII-name byte count, uint32 vertex count, name bytes, then vertex records
of six float32 values `(x,y,z,nx,ny,nz)`. Consecutive triples form triangles.
Suggested GLES2 hookup: one static interleaved VBO, 24-byte stride, position at
offset 0, normal at offset 12; retain named draw ranges for observed-input
highlights. Re-upload on context recreation and release GL objects at shutdown.
Offline packing is not proof of renderer/device integration.

## Input and handoff gates

Field observations describe right-pad mouse movement and a trigger-produced
mouse click, but do not identify exact Android reports, trigger side, dual pads,
gyro or grip semantics. `input_mappings` remains empty. Observe device identity
and Android events with `HidInputInspector` before mapping physical parts. Do
not map an anonymous mouse click to a guessed trigger or trackpad press.

At the user's 2026-10-10 direction, the simplified package is prepared for the
online instance to implement programming/hookup. Remaining: GLES2 lifecycle and
controller integration, observed Android reports, Android APK build and Galaxy
S9 stereo/interaction/sustained frame-time/thermal measurements. The local build
attempt stopped at Gradle configuration because no Android SDK was configured.
No APK, release or device acceptance was produced.
