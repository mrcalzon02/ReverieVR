# Sitka coastal editor, geodata and native environment adapter

**Date:** 2026-10-26  
**Project:** [Sitka](SITKA.md), the original peaceful Sitka Sound fishing/beachcombing game hosted by ReverieVR.  
**Status:** Authoring tools and a game-private GLES2 terrain renderer proof are in source, with synthetic CPU, fake-GL and now **real Mesa EGL/GLES2 pbuffer** checks. No real Sitka DEM/coastline tile, displayed *geographic* terrain, compiled desktop editor run, playable APK, launcher entry, or Galaxy S9 acceptance is claimed.

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
  A local fake-GLES2 smoke exercised two indexed eye draws and release/rebuild;
  a separate 2026-10-28 **real Mesa driver** gate now verifies rasterized
  pixels, static uploads and context recreation. Neither proves headset
  optics or frame pacing.

## Real EGL/GLES2 terrain driver acceptance — 2026-10-28

The unregistered Sitka terrain proof now has a separate **mandatory real-driver**
test, `app/src/test/native/sitka_terrain_egl_test.cpp`, in the existing
all-game content matrix. On Ubuntu 24.04, the test installs Mesa EGL/GLES2
development libraries, requests an ES2 pbuffer context, forces surfaceless
llvmpipe, compiles the actual `TerrainGl` class, uploads a synthetic triangle,
and reads back rasterized **red pixels in both separate eye viewports**. It
compares caller program/buffer/texture bindings, vertex attributes,
depth/blend/cull enablement and viewport before/after each draw; it checks the
successful static VBO/EBO batch upload count remains unchanged across eyes,
releases GPU objects, destroys the context, recreates a **new unshared**
context and repeats the complete test. Missing EGL or draw/readback failure
is a test failure, never a skip.

**Verified:** [exact-code commit `42ba8ea7`](https://github.com/mrcalzon02/ReverieVR/commit/42ba8ea769e2ad8c447ad25d620d58cde925b8cb)
passed [content-matrix run 40763852143](https://github.com/mrcalzon02/ReverieVR/actions/runs/40763852143)
with **all six jobs successful**, including the real EGL/GLES2 regression,
ARMv7+ARM64 NDK source-only link, and existing content checks. Independent
job-log readback records `OpenGL ES 3.2 Mesa 25.2.8` for both contexts
(the test **requested ES2** and uses only GLES2 calls), with two stereo
pixel/state/lifecycle PASS lines. This is genuine software-driver rendering,
**not** a physical Galaxy S9, a strictly version-2.0 driver, a displayed
geographic tile, an APK, a licensed dataset, or proof of headset frame pacing.
The prototype remains deliberately absent from `Android.mk` and the
playable catalog.

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

## Source-only Android NDK link gate — 2026-10-27

`scripts/sitka/verify-ndk-terrain.sh` is a real NDK cross-link check
for the deliberately unregistered `sitka_terrain_module.cpp`. The
`reverie-game-content-matrix-ci.yml` workflow now runs it in a separate
job whenever a Sitka terrain source, fixture regression or the probe itself
changes. It uses the build host's installed NDK, or installs pinned
NDK `27.2.12479018` through Android SDK Manager.

The script builds **ARMv7 API 21** and **ARM64 API 21** shared libraries
with GLES2, rejects undefined link symbols, verifies ELF32/ARM and
ELF64/AArch64 shared-object headers, checks the Android `libGLESv2.so`
dependency and requires the public `reverie_native_module_entry_v1`
symbol. It uses the **installed, exact NDK 27.3.13750724** on the pinned
Ubuntu 24.04 runner, not a network-installed or arbitrary NDK revision.

**Verified 2026-10-27 22:31–22:32 Sitka time:** exact source/CI commit
[`cca0f18aa5418c35e7c542bae277955e8d64a54c`](https://github.com/mrcalzon02/ReverieVR/commit/cca0f18aa5418c35e7c542bae277955e8d64a54c)
passed [all five content-matrix jobs](https://github.com/mrcalzon02/ReverieVR/actions/runs/40644268181),
including the real NDK job. The ARMv7 proof library SHA-256 was
`82eebb4c046de0f1a32bce1c91136a9c81b1a83bc794bdcda08cd9f08c6d98ff`;
ARM64 was
`f426d3cfdd5622d432aa064be6ec9abad3eb80ffe7da10187f0af4c13f4217f6`.
Both ELF files were uploaded as the explicitly
[source-only ABI artifact](https://github.com/mrcalzon02/ReverieVR/actions/runs/40644268181/artifacts/12796477836).
This repairs the earlier failing NDK gate at
[run 40563472230](https://github.com/mrcalzon02/ReverieVR/actions/runs/40563472230),
which exited 74 without diagnostic output.

These are temporary CI proof binaries only: **not** `Android.mk` targets,
not APK assets, not a new game identity, and not a real GLES2 display or
headset test. No Sitka geography, real GL driver, native module admission,
fishing gameplay or S9 performance was validated by this link-only gate.

## Verified gates and outstanding work

**Passed locally:** `python3 scripts/sitka/test_build_coast.py` (one synthetic fixture suite, deterministic export and provenance/input rejection); strict GCC C++17 host build and execution of `app/src/test/native/sitka_environment_test.cpp`, plus Clang AddressSanitizer/UndefinedBehaviorSanitizer execution of the same C++ adapter regression.

**Not executed/accepted:** editor `javac` with full Beckenburg source present; displayed Swing water/sky interaction; USGS/NOAA Sitka source acquisition and triangulation; real coordinate/datum check; real *geographic* terrain display; full Android NDK/Gradle playable APK build, headset frame-pacing and comfort, module admission/packaging/update acquisition; gameplay rod/crab/shell/boat progression. The source-only NDK link is verified separately above.

**Next concrete milestone:** acquire approved, legally distributable Sitka DEM and shoreline vectors with verified datum/rights, bake a genuine cove, integrate authored water/weather, measure actual S9 stereo frame pacing, and only register Sitka after real fishing/shore activities work. Do not ship the synthetic fixture as geographic content or duplicate Beckenburg/Malefic assets.
