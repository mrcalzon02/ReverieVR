# Sitka coastal editor, geodata and native environment adapter

**Date:** 2026-10-26  
**Project:** [Sitka](SITKA.md), the original peaceful Sitka Sound fishing/beachcombing game hosted by ReverieVR.  
**Status:** Authoring tools and a game-private GLES2 terrain renderer proof are in source, with synthetic CPU and fake-GL host checks. No real Sitka DEM/coastline tile, displayed terrain, compiled desktop editor run, NDK/APK acceptance, launcher entry, or Galaxy S9 acceptance is claimed.

## Native terrain proof — 2026-10-27

- `app/src/main/jni/native/sitka_terrain_mesh.h` strictly decodes the
  exporter `SITKA01\\0` header, little-endian 32-bit vertex/index counts,
  double projected origin, 15-byte XYZ/RGB vertices and uint32 triangles.
  It rejects bad magic, oversized counts, truncated/trailing bytes,
  nonfinite coordinates, invalid/duplicate indices and degenerate triangles.
- GLES2 core does not guarantee `GL_UNSIGNED_INT` indices. The terrain
  adapter remaps triangles into batches of at most 65,535 unique vertices
  with `GL_UNSIGNED_SHORT` indices, retaining triangle order and colors.
- `sitka_terrain_gl.h` uploads each batch to static VBO/EBO pairs **once
  per GL context**, uses the shared shader/matrix/GL-state guard and
  separately preserves the element-buffer binding absent from that guard.
  Each eye issues indexed draws with no CPU terrain rebuild or upload.
  Context release deletes buffers/program; recreation rebuilds them.
- `sitka_terrain_module.cpp` implements the actual native module ABI and
  reads only `sitka/coast/terrain.sitkam` through host `read_asset`.
  It is a **source-only ABI prototype**, deliberately absent from `Android.mk` and the playable host registry until real content and gameplay are available.
  No synthetic mesh is bundled as geography, and the host content
  allowlist has intentionally **not** been expanded to admit Sitka yet.
- `app/src/test/native/sitka_terrain_mesh_test.cpp` exercises the
  synthetic exporter-compatible fixture, malformed bytes and a 70,002
  vertex remapping split. Local GCC C++17 and Clang ASan/UBSan passed.
  A local fake-GLES2 smoke exercised two indexed eye draws, unchanged
  static upload count and release/rebuild; it does **not** prove driver
  rendering, NDK compilation, stereo optics or frame pacing.

## Reuse of the actual shared toolchain

- The **standalone desktop editor** in `tools/sitka/SitkaEditor.java` imports the Beckenburg editor-kit's real `EditorProject`/`EditorPalettePanel` and the MIT independent `WaterCanvas`, `WaterEditorModel`, and `WaterEditorPanel`, not a fabricated water inspector.
- It consumes Beckenburg's **shared Java 17 `WeatherModule` and `WeatherType`**. The Malefic Veil's `VeilWeather` is already an adapter around the same independent weather library; Sitka follows that integration boundary, not Malefic's reflection/JAR distribution mechanism.
- `tools/sitka/launch-editor.sh` compiles against an **existing checkout of Beckenburg**, leaving its source and the Malefic Veil untouched. Editor compilation and interactive capture are still pending where that checkout and desktop display are available.
- Desktop water output is an **authentic Beckenburg v1** UTF-8 `.water` document: `water/sitka.water`. Sitka weather is a narrow authored profile: `weather/sitka.properties` (`version=1`, `condition`, `hour`, `wind_mps`, `wind_heading_deg`). Saves use per-file atomic replacements.
- The game-private C++17 adapter `app/src/main/jni/native/sitka_environment.h` parses bounded water footprints/settings and the Sitka weather profile and generates a deterministic two-layer wave-height sample. Its weather preset values were matched to Beckenburg's `WeatherType`; this does **not** represent a 1:1 port of Java water optical rendering or GPU weather effects. No GPL/proprietary desktop renderer or proprietary art has been imported.
- Do not call the Java Swing editor an in-headset editor: it is an **asset authoring tool** intended to export content for a future Android GLES2 renderer.

## Run the editor in a desktop checkout

Requires JDK 17 or later (with `javac --release 17` support), a desktop display, the ReverieVR checkout, and an unmodified Beckenburg repository checkout.

```bash
cd ReverieVR
BECKENBURG_ROOT=/absolute/path/to/The-Beckenburg-Project \
  bash tools/sitka/launch-editor.sh /absolute/path/to/sitka-workspace
```

The supplied workspace receives `water/sitka.water` and `weather/sitka.properties`. The water editor includes placement, footprint transform, selection, snapping, properties and bounded undo. The weather panel previews clear, overcast, drizzle, rain, heavy rain, fog, dense fog, snow and thunderstorm, clock position and wind. The standalone GUI currently has **no true coast terrain 3D preview**; do not mistake weather sky preview for Android rendering. Asset labels use ordinary Swing fonts; the eventual native app must use ReverieVR's actual licensed font atlas.

## Generate a low-poly coastal tile, using *verified* source datasets

`scripts/sitka/build_coast.py` takes a metric-projected GeoTIFF DEM, GeoJSON coastline/land polygons in EPSG:4326, and a provenance JSON manifest. Install the geospatial dependencies in your local authoring environment (Python 3, rasterio, shapely, pyproj and numpy). Example:

```bash
python3 scripts/sitka/build_coast.py \
  --dem /data/approved/elevation.tif \
  --coast /data/approved/land-polygons.geojson \
  --provenance /data/approved/sources.json \
  --output build/sitka/coast/cove.json \
  --step 12
```

Run `python3 scripts/sitka/build_coast.py --help` for the exact supported CLI. Do not infer this is an executable example until the actual approved source files exist. The exporter validates metadata presence and rejects explicitly Google-labelled sources (it does not independently prove that a declared license is valid), generates fixed triangles in a local metre-based scene and writes a binary `.sitkam` mesh and neighboring JSON description. **No approved Sitka geography was fetched, baked or staged during this milestone.**

Use public-domain USGS Alaska elevation tiles as available, NOAA shoreline/bathymetry where actually suitable, and OSM geographic vectors only with compatible ODbL attribution and derived-database obligations. The owner's Google Earth map screenshot is *conceptual* and must never be pixel-traced or converted to a mesh. Every terrain/geographic input must carry a provider, acquisition URL, retrieval date, coverage, license, CRS, original checksum and transformation metadata. Actual dataset coverage and vertical datum still need local verification. Terrain must eventually be clipped for dry shore, water mask, tidal level, boat route and collision rather than treating an authored water rectangle as navigable ocean.

## Geometry transfer contract (current source only)

- `scripts/sitka/build_coast.py` outputs a compact, deterministic `.sitkam` mesh with documented magic/header, origin, vertex positions/colors and triangle indices, alongside the provenance JSON.
- Runtime axes: east +X, up +Y, north +Z. Preserve each tile's projected coordinate origin and static edge samples for consistent neighboring chunks and LOD.
- The environment parser remains independent of GL. The game-private indexed terrain VBO/EBO source now exists, but real Android driver rendering and module admission are unverified. Water surface mesh, optical depth, sun/sky, cloud/rain particles, shoreline shaders, occlusion/collision and atlas staging are not integrated.
- Weather is a static saved preset profile at this milestone. Dynamic weather transitions, generated soundscape and 3D rainfall require separate real integration with the ReverieVR shell-owned audio/runtime services.

## Verified gates and outstanding work

**Passed locally:** `python3 scripts/sitka/test_build_coast.py` (one synthetic fixture suite, deterministic export and provenance/input rejection); strict GCC C++17 host build and execution of `app/src/test/native/sitka_environment_test.cpp`, plus Clang AddressSanitizer/UndefinedBehaviorSanitizer execution of the same C++ adapter regression.

**Not executed/accepted:** editor `javac` with full Beckenburg source present; displayed Swing water/sky interaction; USGS/NOAA Sitka source acquisition and triangulation; real coordinate/datum check; actual GL driver terrain render; Android NDK/Gradle build, headset frame-pacing and comfort, module admission/packaging/update acquisition; gameplay rod/crab/shell/boat progression.

**Next concrete milestone:** verify this exact source target with the real Android NDK and a real GLES2 context, then acquire approved, legally distributable Sitka DEM and shoreline vectors, bake a genuine cove, integrate authored water/weather, measure actual S9 stereo frame pacing, and only register Sitka after real fishing/shore activities work. Do not ship the synthetic fixture as geographic content or duplicate Beckenburg/Malefic assets.
