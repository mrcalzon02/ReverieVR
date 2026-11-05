# Sitka shore: georeferenced waterfront exporter

**State:** Source-only authoring implementation. This is not evidence of actual Sitka data acquisition, authentic facades, playable shoreline, Android release, or Galaxy S9 validation.

This exporter connects the existing `build_coast.py` DEM/shoreline output to the optional native `sitka/shore/waterfront.sitkash` geometry reader. The native loader and terrain geometry renderer already consume `SITKA-SHORE-1`. The exporter supplies that exact format using separately acquired, legally cleared vector sources.

## Input contract

- `--terrain-manifest`: `sitka-static-terrain-1` JSON produced by the existing DEM/coast baker. The adjacent `.sitkam` checksum and projected-metre double-precision origin are independently checked.
- `--dem`: the **same** DEM used to bake that terrain, checked by SHA-256 and CRS. Buildings sample the DEM at every footprint vertex, reject missing elevations, and reject grade differences exceeding 2 m pending manually modeled foundations.
- `--buildings`: GeoJSON FeatureCollection containing source-identified **Polygon** building footprints. Preserve actual perimeter vertices, including concave geometry; no bounding-box substitution, hole removal, arbitrary reorientation, or guessed roads.
- `--blocks` (optional): separately authored/sourced GeoJSON Point records for maritime primitive placement: `rock`, `forest`, `pier`, `pontoon`, `pile`, `harbor_shack`, `bridge_deck`, `bridge_ramp`. Every block requires an explicit centre, width, depth, height, base elevation and yaw in radians. A ramp also requires rise.
- `--provenance`: JSON with nonempty `buildings` and, when blocks are present, `harbor_structures` records, each containing `provider`, `dataset`, `license`, `retrieved`, and `url`. Metadata is mandatory but **does not itself prove** redistribution rights. The operator must verify source terms and any ODbL derivative/database obligations. Google Maps/Earth-derived offline geometry is prohibited.

GeoJSON coordinates default to WGS84/EPSG:4326 unless its declared `crs.properties.name` explicitly identifies another recognized source CRS. The terrain's projected-metre CRS governs the output. The geometry is rendered relative to the same `terrain.sitkam` origin, with +X east, +Y up, +Z north.

Each feature requires a unique stable ASCII `feature_id` and a `sitka_kind` property. Buildings admit `storefront`, `cannery`, `heritage`, or `warehouse`. A known `height_m` is used as authored; otherwise `building:levels` becomes a clearly marked 3 m/level P0 estimate, or a class-height estimate is used. Estimated roof form, building detail, private access, and historical identity are **not** implied by the exported mass.

## Example

```bash
python3 scripts/sitka/build_waterfront.py \
  --terrain-manifest work/sitka/terrain_manifest.json \
  --dem work/sitka/source_dem.tif \
  --buildings work/sitka/licensed_building_polygons.geojson \
  --blocks work/sitka/authored_maritime_blocks.geojson \
  --provenance work/sitka/waterfront_sources.json \
  --output work/sitka/sitka/shore/waterfront.sitkash
```

A separate `waterfront.manifest.json` includes the geographic source checksums, feature counts, output checksum, source ledger, coordinate origin, per-building height quality and known limitations. The `.sitkash` parser is strictly bounded to **65,536 UTF-8 bytes, 256 features, 32 vertices per polygon**, and retains native feature identifiers. All lines are sorted by ID for deterministic output independent of input feature ordering. An invalid file fails before export rather than silently dropping buildings.

Run the deterministic synthetic-fixture regression:

```bash
python3 -m unittest discover -s scripts/sitka -p 'test_build_waterfront.py' -v
```

Authoring dependencies: `rasterio`, `pyproj`, `shapely`, and a local C++17 compiler for native parser round-trip verification. These are desktop/CI tools, **not APK runtime dependencies**. No remote service or network access is used.

## Explicit follow-on gates

1. Acquire and license-check Sitka Harbor, Crescent Harbor, Katlian and Lincoln Street source footprints, then validate datum alignment and heritage identities against independent permitted evidence.
2. Review P0 placement and DEM footprint grades, add authored retaining foundations rather than trusting a min-height massing base, and provide clean public-walkway/boat-dock collision boundaries.
3. Introduce subdivided shore cells/LOD before growing the footprint count above native limits. Do not pretend a global static mesh is lazy-loaded.
4. Upgrade individually sourced buildings to P1 facades and P2 bespoke structures; use P0 massing only as placeholder or far-LOD. Perform real stereo, device thermal and APK integration gates separately.

Reference: `docs/project/SITKA_IRREGULAR_CITY_AND_CUSTOM_BUILDINGS.md`, `docs/project/SITKA_CITY_WORLD_DESIGN_SPEC.md`, and the owner-provided waterfront blueprint. This authoring pass deliberately does not alter skiff, bathymetry, fishing, or helm state.

## Source-integrated P1 shoreline detail pass

`sitka_shore_detail.h` adds deterministic vertex-colored geometry **only for source-provided shore features**. It does not alter geographic footprints or invent city blocks. Building exteriors receive limited dark windows on existing source polygon edges; suitable storefronts receive a single low-poly rain canopy; piers/pontoons receive sparse top plank seams; bridge deck/ramp blocks receive light guardrails; authored harbor shacks receive a dark seaward-facing marker. These are **stylized visual estimates**, not surveyed architectural facts, functional doors, collision surfaces, or inferred public access.

The main `sitka_terrain_module.cpp` source-stage reader calls this once after `AppendShoreGeometry` and before GPU allocation. Its generated triangles enter the existing immutable terrain VBO batches; nothing is rebuilt per eye or per frame. `AppendShoreDetails` is transactional (failure leaves its input mesh unchanged) and caps details at 1,800 quads alongside the inherited native terrain vertex/triangle ceilings. Existing missing `waterfront.sitkash` still means **no** invented buildings.

Run: `g++ -std=c++17 -Wall -Wextra -Werror -Iapp/src/main/jni/native app/src/test/native/sitka_shore_detail_test.cpp -o /tmp/sitka-shore-detail-test && /tmp/sitka-shore-detail-test`. The regression checks reproducibility, facade and bridge/dock elements, authored polygon retention, packing and the absence of detail on natural rock. Real GLES2/S9 visuals and source building surveys remain unverified. No gameplay or boat code is introduced.
