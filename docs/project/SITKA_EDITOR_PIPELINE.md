# Sitka coastal editor, geodata and native environment adapter

**Date:** 2026-10-26  
**Project:** [Sitka](SITKA.md), the original peaceful Sitka Sound fishing/beachcombing game hosted by ReverieVR.  
**Status:** First environment **source milestone**. Authoring-tool source and independent terrain generation are committed, with synthetic-fixture and native-adapter host tests. No real Sitka DEM/coastline tile, rendered coast scene, compiled desktop editor run, Android native module, launcher entry, APK, or Galaxy S9 acceptance is claimed.

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
  --output build/sitka/coast/cove.sitkam \
  --step 12
```

Run `python3 scripts/sitka/build_coast.py --help` for the exact supported CLI. Do not infer this is an executable example until the actual approved source files exist. The exporter validates the inputs, generates fixed triangles in a local metre-based scene and writes a binary `.sitkam` mesh and neighboring JSON description. **No approved Sitka geography was fetched, baked or staged during this milestone.**

Use public-domain USGS Alaska elevation tiles as available, NOAA shoreline/bathymetry where actually suitable, and OSM geographic vectors only with compatible ODbL attribution and derived-database obligations. The owner's Google Earth map screenshot is *conceptual* and must never be pixel-traced or converted to a mesh. Every terrain/geographic input must carry a provider, acquisition URL, retrieval date, coverage, license, CRS, original checksum and transformation metadata. Actual dataset coverage and vertical datum still need local verification. Terrain must eventually be clipped for dry shore, water mask, tidal level, boat route and collision rather than treating an authored water rectangle as navigable ocean.

## Geometry transfer contract (current source only)

- `scripts/sitka/build_coast.py` outputs a compact, deterministic `.sitkam` mesh with documented magic/header, origin, vertex positions/colors and triangle indices, alongside the provenance JSON.
- Runtime axes: east +X, up +Y, north +Z. Preserve each tile's projected coordinate origin and static edge samples for consistent neighboring chunks and LOD.
- C++ environment parser is independent of GL so fixed-update/dual-eye render state remains stable. Actual Android VBO uploads, water surface mesh, optical depth, sun/sky, cloud/rain particles, shoreline shaders, occlusion/collision and atlas staging **are not integrated** yet.
- Weather is a static saved preset profile at this milestone. Dynamic weather transitions, generated soundscape and 3D rainfall require separate real integration with the ReverieVR shell-owned audio/runtime services.

## Verified gates and outstanding work

**Passed locally:** `python3 scripts/sitka/test_build_coast.py` (one synthetic fixture suite, deterministic export and provenance/input rejection); strict GCC C++17 host build and execution of `app/src/test/native/sitka_environment_test.cpp`, plus Clang AddressSanitizer/UndefinedBehaviorSanitizer execution of the same C++ adapter regression.

**Not executed/accepted:** editor `javac` with full Beckenburg source present; displayed Swing water/sky interaction; USGS/NOAA Sitka source acquisition and triangulation; real coordinate/datum check; actual terrain render; GLES2 shader/vertex-buffer implementation; Android NDK/Gradle build, headset frame-pacing and comfort, module packaging/update acquisition; gameplay rod/crab/shell/boat progression.

**Next concrete milestone:** obtain verified, legally distributable Sitka DEM and shoreline vectors; bake one genuinely accurate cove; use current Sitka editor to place water/weather against the real coast; implement GLES2 cached terrain/water rendering in an actual Sitka native module; run host, NDK, APK and device tests; only then add a functioning Native Apps entry. Do not duplicate or overwrite Beckenburg/Malefic assets.
