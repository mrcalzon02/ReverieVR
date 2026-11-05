> **Lighting module integrated (2026-10-30):** All three genuine Beckenburg Java 17 lighting classes are now pinned with the portable editor, water and weather code. The Sitka environment editor exposes a Lighting tab with native Beckenburg preset/falloff calculations, a live authoring preview, and separately saved `lighting/sitka.properties`. The internal games manager opens the same Sitka lighting profile in its Lighting dock, and offers per-game lighting profiles to Red Ledger and other titles. This is **desktop source integration only**; Sitka's Android GLSL lights and physical runtime fixture mapping have not been ported. See [internal editor integration](INTERNAL_GAME_EDITOR_BECKENBURG.md).

> **2026-10-30 integrated authoring milestone:** Sitka is now in ReverieVR's desktop **internal games workbench** as a non-playable game project with safe per-region blockout documents, actual Beckenburg portable editor palette/transaction history, and a launch action for the genuine water/weather editor. Exact 33 owner-source Java 17 files are pinned in `tools/editor-kit/upstream-lock.json`, so `tools/sitka/launch-editor.sh` or `tools/sitka/launch-editor.cmd` works without an adjacent Beckenburg checkout. The water/weather GUI still writes real authoring data rather than the playable game's assets; actual Sitka geography, cross-tile streaming, environment-runtime linkage, building/road collision, and fishing remain pending. [Integration contract](INTERNAL_GAME_EDITOR_BECKENBURG.md).

# Sitka coastal editor, geodata and native environment adapter

## Sitka path-constrained grounded walking session — 2026-11-04

`sitka_walk_session.h` now implements an actual source-graph-bound foot
position: native joystick movement takes up to 0.05 s of travel per
simulation frame at an authored real-world walking pace (1.45 m/s),
rejects walking off connected source sidewalk/shore path surfaces,
slides gently along existing corridors, follows the individually
authored footpath Y elevation (including measured bridge decks), and
never adds fake traversable shore. The session starts **only** at the
public source-authored S spawn; an absent walking graph does not create
a fictional player or fallback town.

The native Sitka module now applies the resulting 3D foot position to
a **copy** of the world-view matrix before rendering either Cardboard
eye (terrain, water, paths, shore, bridge, boat): no IPD or headset view
mutation, and no stereo-specific simulation advance. The controller's
fishing ray is offset by this same world position before the existing
surveyed-water target test; shore fishing still requires the separate
reachable, reviewed public stance tied to a real target. The walking
session is idle while the skiff is active or the fishing interaction is
under way. A GL-context rebuild retains the approved world foot pose,
with no second texture upload introduced.

This stage is accessible through the native module's *joystick axis*
input. The Daydream-only touchpad locomotion adapter, real world
collision against city objects and shore edges, district/page streaming
beyond one loaded network, height-matched camera comfort on slopes and
full playable Sitka registration still require completion and real
headset testing. The existing shell movement cap is 100 m for unrelated
native games and has not been relaxed globally. Do **not** claim a
finished city walking/fishing game based on these source-only tests.


## Full-city pedestrian graph and legal shore-fishing admission — 2026-11-04

**Objective:** eventually walk *all* accessible Sitka streets, paths and
shorelines at believable real-world scale; transition into shore fishing
without floating through working docks, private land or harbor water.
This milestone establishes the **native walking-network contract**,
**source-to-network geographic exporter**, and **shore-fishing access gate**.
It does **not** claim that a current complete Sitka city map has been
installed or that headset locomotion/collision follows the streets yet.

`sitka_walk_graph.h` accepts optional
`sitka/walk/paths.sitkaw` (maximum 256 KiB) with reviewed provenance
SHA-256, exact EPSG:32608 terrain origin, real georeferenced N nodes,
source-authored E pedestrian corridors, one public S start and individually
reviewed F fishing stances linked to actual fishing-spot IDs. Edge types
cover sidewalks, Sea Walk, crossing, trail, shore path, steps, pedestrian
bridge and approved public pier. Connectivity comes **only** from shared
surveyed endpoints: lines passing nearby or crossing in plan do not magically
connect, and isolated fishing access is rejected. The CPU graph provides
reachable-node checks, physical-length Dijkstra routing, local path-height
sampling and one **flat quad strip** per authored segment, rendered in a
separate static GLES2 terrain layer. Routes are simplified source lines,
not invented grid blocks.

`build_walkways.py` transforms separately licensed/currently reviewed
pedestrian GeoJSON LineStrings into the native graph in the **exact same
projection and terrain-origin frame** used for buildings/terrain. It samples
the source-matched DEM only for ground paths; bridge and public-pier routes
require independently authored per-vertex surface elevations. Public access
and present existence must be expressly approved per feature. A walk
start and each fishing stance must coincide with an **actual authored
route vertex** and each fishing stance must reference a surveyed fishing
site. The exporter writes a separate checksum/provenance manifest and
refuses invented widths, coordinates, implicit road intersections, broken
sources, private routes and unmeasured bridge grades.

`sitka_terrain_module.cpp` optionally loads this graph once. A walking
cast now needs both the original surveyed target-and-water ray hit **and**
a reachable, sourced, public bank/pier stance matching the target spot.
No walk graph or invalid listener pose means no arbitrary shore casting;
the boat's existing separate simulation is unchanged. Walking corridors
render from the same terrain-local coordinates. Once source data exists,
this avoids the old ability to cast from arbitrary water proximity.

`data/sitka/research/pedestrian-districts.json` lists eight genuine
areas/corridor families to acquire and verify, from the Lincoln Street
core and Sea Walk/Crescent Harbor through Katlian, Japonski, eastern
streets, Indian River and the Cross Trail. Its source links reference the
City and Borough of Sitka GIS and pedestrian traffic study and Visit
Sitka's pedestrian map, **not** a fictional city layout.

**Remaining blockers before playable full-city walking:** the Sitka
prototype is not yet registered as a playable native module, there is
no licensed current citywide pedestrian source package staged, the VR
shell uses a bounded movement rig rather than city-path collision /
continuous height-following and larger region-streaming, and actual legal
shore-access/angling sites require review. Don't label the eight research
districts or synthetic test routes as completed game geography.


## GIS-to-native O'Connell Bridge alignment baker — 2026-11-04

`scripts/sitka/build_oconnell_bridge.py` now completes the offline
source-to-runtime handoff for the **simple-textured** O'Connell Bridge.
It takes a licensed, reviewed GeoJSON LineString from a specific OSM way,
a source-hashed and individually authored deck-height profile, and the
**same checked projected terrain origin/CRS** used by the coast baker.
Only after validating site identity, source SHA-256, existence, approval,
line collinearity and measured grade does it write the optional
`sitka/structures/oconnell.bridge` file consumed by the real native
GLES2 bridge renderer. No arbitrary world coordinates, unmeasured road
heights, or guessed bridge endpoints are inferred. The resulting text
is bounded to 8 KiB and records complete source/provenance metadata
in a neighboring JSON manifest.

The exporter regression uses an explicitly **synthetic** three-point
fixture; it exercises a round trip through the actual C++17
`ParseBridgeLayout` and checks invalid CRS/terrain origin, missing
measured grade, stale source hashes, unreviewed identity and bent axes.
A real-source bridge layout **has not been exported yet** because an
approved, current OpenStreetMap/engineering alignment and individual
deck elevations have not been furnished. The module does not stage an
invented bridge into the VR scene or use Google imagery/geometry.


## Actual Sitka geographic source ingestion and photo comparison — 2026-11-04

The source-only authoring inventory now includes 96 **real, archived**
building polygons selected deterministically from the original public-domain
Sitka building-footprint import, within about 393 metres of the Library of
Congress HAER AK-27 bridge reference point. The original Git blob is pinned
in `data/sitka/research/bridge-downtown-2017.geojson` and its exact WGS84
vertices are preserved. **The archived dataset is dated; these polygons are
not accepted as the current 2026 city or automatically installed in the APK.**
The upstream import's README explicitly directs users to newer OpenStreetMap
data for up-to-date building footprints.

`scripts/sitka/prepare_osm_waterfront.py` now accepts a separately downloaded
OpenStreetMap XML source and a bounded WGS84 study area, and emits a
review-only GeoJSON with original source way IDs, sourced heights/levels when
present, exact input checksum, ODbL attribution, and unresolved presence/
appearance status. It ignores demolition-tagged buildings and does not crop
partly visible polygons or turn an unclassified building into a fictional
storefront. This feeds the existing stricter `build_waterfront.py`
source/license/height/DEM/active-status review gate; **no automatic promotion**
occurs.

A source-only measured landmark record
`data/sitka/research/oconnell-bridge-facts.json` preserves the real
John O'Connell Bridge length, main span, width, twin-tower sets and
harp-cable pattern from cited engineering sources, without pretending to
know surveyed endpoints, pylon XY or deck gradients. Photo/plan references
(including Sitka's five individual city harbor plans and Google Maps as
**visual-only**, non-extractable context) are linked rather than repackaged
at `data/sitka/research/photo-source-audit.json`. The Harbor Department's
seven-float Crescent map and the historic Sitka waterfront walking-tour map
remain separately identifiable factual references, not a generic harbor kit.

This milestone remains **nonplayable source-stage research and tooling**.
No archival 2017 footprint becomes verified 2026 building geometry, no
photo is copied into the APK, and no Google imagery or proprietary GIS
geometry is traced or rebaked. The source-content CI now exercises
the OSM converter and photos/archive source assertions alongside the
authoritative exporter regression and shoreline C++ LOD gates.
Geographic placement and device acceptance still depend on current
licensed data, real elevation/datum matching and photographed site review.


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


## Source-only GLES2 authored water preview — 2026-10-29

The unregistered Sitka prototype now requires actual bounded Beckenburg v1
water and Sitka weather assets through the host read_asset interface at
sitka/water/sitka.water and sitka/weather/sitka.properties. Missing or
malformed assets fail closed; no synthetic ocean is bundled. The prototype
remains absent from Android.mk, the playable registry and asset allowlist.

Game-private sitka_water_gl.h creates convex polygon water footprints for
SQUARE, OCTAGON, CIRCLE, OBLONG and PILL. One immutable VBO per footprint
is uploaded per context. A GLES2 vertex shader implements the two existing
deterministic WaveHeight layers with bounded wind amplitude; rain, fog and
time-of-day affect provisional opaque blue tint. Simulation time advances
once per Update, not per eye. The renderer preserves the caller's GL state,
uses depth testing after terrain, and releases buffers/program on context
loss. It does NOT yet implement true shoreline/dry-land clipping, optical
depth, refraction, surveyed tides, bathymetry, boat collision or real Sitka
coastal data.

The existing real Mesa EGL test now includes synthetic blue-water pixel
assertions in both eye viewports, state isolation, no per-eye static upload,
deterministic weather/wave sampling and fresh unshared context recreation.
The existing content-matrix also source-links ARMv7 and ARM64 via NDK.
These are new tests, NOT a passing-CI claim until an exact-commit run is
observed. The synthetic square is test-only. Galaxy S9 FPS, licensed
geography, editor display, fishing gameplay and game admission remain open.

## Beckenburg portable authoring suite refresh — 2026-10-30

Sitka's desktop environment editor now opens the shared **PSX authoring** menu with real upstream animation clips, WAV sound derivatives, bitmap billboard bakes, campaign flow authoring, OBJ/STL mesh reduction and GLB export, and the PSX reference viewer. Each is implemented through the pinned Java 17 editor source at `tools/editor-kit/upstream/` and a ReverieVR-specific `EditorToolLauncher` adapter, not a new parallel editor. The shared lighting/water/weather modules remain exactly the upstream implementations already imported.

Source revision and individual SHA locks: `tools/editor-kit/upstream-lock.json`, upstream `2a16001ca76d47fdaaed6d927e5c02f81e11ef49`. The seven source regression fixtures live in `tools/editor-kit/upstream-tests/` and compile headlessly. Neither these tools nor their output files are bound yet to Sitka's Android native fishing, weather audio, lighting shaders, or animation playback; new artifacts must be reviewed and staged through a future validated runtime contract. No synthetic geographic data or licensed game media were added.

## Authored ambient intensity → source-only GLES2 terrain — 2026-10-31

The genuine desktop `LightingAuthoringPanel` writes `lighting/sitka.properties`
using Java `Properties.store` (`schema=1`, `preset`, `beam`, `ambient`,
`source`, `reach`, `diffusion`, `flicker`, `cache`, `cell`,
`outdoor`, `changed`, `reuse`). The game-private
`sitka_lighting.h` validates the **complete version-1 profile**, including
Beckenburg's exact numeric ranges and canonical named presets. Missing,
duplicate, out-of-range, noninteger, unknown-beam, or malformed settings are
rejected. The source-only native module now requires the authored file at
`sitka/lighting/sitka.properties` through its bounded host asset reader;
it does not invent a runtime default when the required asset is missing.

**Only the authored `ambient` percentage is applied**: 50–150 maps to a
0.5–1.5 fragment-shader multiplier on the existing vertex-color terrain.
It is uniform across both eyes, clamped to valid display color, and has no
per-eye upload or simulation work. The other validated settings remain
**unimplemented in native terrain lighting**: there are no authored
directional sources, beam cones, attenuation/falloff, occlusion, shadows,
flicker, or cache scheduling. The desktop Java lighting preview remains
distinct from the Android renderer.

`sitka_lighting_test.cpp` is the deterministic CPU parser/negative-input
contract. The existing real Mesa `sitka_terrain_egl_test.cpp` now renders
100% and 50% ambient profiles and compares actual stereo framebuffer
pixels, host GL-state restoration, static buffer uploads and a second
unshared context. These are **synthetic test triangles**, not licensed Sitka
geography. CI status must be checked at the exact commit before claiming
the tests pass. Sitka remains unregistered and unplayable; no Galaxy S9,
geographic, real source-light or fishing-loop acceptance is implied.

## Bathymetric source-to-boat adapter — 2026-11-04

The offline `scripts/sitka/build_bathymetry.py` converter creates checked
SITKAB01 projected-metre bottom-elevation tiles, an index of host-scoped
filenames, and a detailed source SHA-256 manifest from an *already vetted*
GeoTIFF. It does not download NOAA data, fix an unknown vertical datum,
auto-fill missing coverage or create simulated Sitka geographic assets.
Signed depth rasters need explicit positive-down or elevation-up metadata;
the source and terrain vertical datums must already agree before export.

The C++17 `sitka_bathymetry.h` parser rejects corrupt/oversized tiles,
unreconciled datum IDs, NoData interpolation and overlapping tiles. The
source-only, unregistered `sitka_terrain_module.cpp` now requires an
authored index and all referenced bathymetry tiles in addition to terrain,
water, weather, lighting and fishing-site sources. This enforces fail-closed
content loading; there are still **no licensed Sitka files** in the package.

The CPU-only `sitka_boat_navigation.h` computes actual game-water-to-bed
depth, virtual keel clearance, source quality and quarter-second ping state.
Fixed 30 Hz skiff rules stop at unapproved water, shallow draft clearance
or bathymetry gaps, without moving the seated head camera. These are tested
simulation components, **not yet a boat object, underwater GL renderer,
Lato depth display, playable launcher or real on-device sonar**.

Follow the [bathymetry/boat milestone](SITKA_BATHYMETRY_AND_BOAT_SPEC.md)
before installing assets or calling this geographically accepted.

## NoData-safe low-poly seafloor geometry + GLES2 reuse — 2026-11-04

`sitka_seafloor_mesh.h` turns source-covered bathymetry rasters into
rebased terrain vertices and conservative low-poly triangles. Its
optional stride selection checks every underlying cell before emitting
a decimated quad, so no artificial floor bridges gaps. Existing
`TerrainGl::CreateWithExtras` uploads the additional indexed buffers
under the **same** land shader and GL-state guard; per-eye rendering
does not regenerate geometry or change simulation time. The source-only
Sitka module loads reviewed bathymetry assets and bakes the sea floor
once per GL context, with a 300,000-vertex visual cap pending streamer
and Daydream performance acceptance.

The GLES2 real-driver regression includes artificial seabed pixel
readbacks in both viewports, static upload counts, state restoration
and unshared context recreation. This is synthetic-only GL proof,
not a full coastal runtime: the provisional water shader stays opaque,
true coastline clipping/LOD streaming and visible boat gauge remain
open. No real Sitka geography or bathymetric dataset is distributed.

## Lazy bathymetry V2 tile directory (2026-11-04)

The source-only bathymetric converter now writes a coordinate-bearing V2
tile index. A bounded native CPU cache reads only the current geographic
seabed tile and one predicted neighbor, with explicit NoData behavior.
The unregistered native editor preview uses headset focus until a real
boat model is available. A 40-tile simulated route and CSV trace are
tested in GitHub Actions. The current shader still needs explicit
GL-safe in-motion tile switching, land/city streaming and actual verified
USGS/NOAA input; runtime mobile frame-time evidence does not yet exist.

## Proximal procedural seabed infill

Survey/composite bathymetry retains precedence. A new deterministic
seeded value-noise layer fills missing source pixels and ocean tiles,
blended by fixed source-derived guidepoints from the raster exporter.
Generated tiles use world-anchored 256 m regions and a 16 m sample
grid, so traversing and reloading them does not change shape.
Only authored water footprints receive geometry. Generated sonar
depth quality is shown as PROCEDURAL. Runtime GPU swap during boat
travel and real licensed Sitka data still remain outstanding.

## GL-safe underwater tile replacement primitive

`TerrainGl::ReplaceSeafloor` now supports a transactional seafloor-only
VBO/EBO change on the owning GL thread. It stages validated new
16-bit batches, verifies GL uploads, preserves immutable land buffers,
and restores the shell's GL bindings. Failed stages retain the last
committed seafloor. The Mesa regression tests valid tile swap,
malformed transition rollback, no-coverage clearing, return travel,
two-eye readback and unshared context recreation.

**Wiring boundary:** The prototype still lacks a persistent, explicit
once-per-frame GL preparation callback from the native shell. This
safe swap method is a tested primitive, not yet proof that boat travel
invokes it or that 30 FPS is sustainable. Never call it from either
`render_eye` callback or the non-render simulation update thread.

## Frame-boundary terrain streaming, LOD hysteresis (2026-11-04)

An optional append-only v1 module callback, `prepare_frame`, now
runs **once per headset frame**, after native simulation update
and before left/right stereo eye rendering. Old native modules
ignore the capability. The Sitka source-only native prototype
uses that stage to examine its CPU two-tile bathymetry cache,
select the region under the current development headset focus,
fill missing source cells with flagged deterministic seabed
only within authored water, generate a new 256 m region if
needed, and atomically replace seafloor-only GPU batches.

`SeafloorFramePlanner` prevents repeated uploads at the same
coordinates, treats real data arrival as an explicit source
upgrade, holds the old rendered tile until 8 metres inside
an adjacent same-quality region, and changes model detail
between stride 1/2/4 using 44/64 and 140/180 m hysteresis
bands. Large source tiles enforce a coarser minimum stride
to keep the one-frame upload around 17k vertices or less.
Failed GL stages keep the last committed floor and retry only
after 30 update frames. Stereo eye functions never open
assets or upload geometry.

**Scope and limits:** The current input focus is the headset
position because a Sitka skiff is not yet instanced in the
registered game registry. Initial source tiles still load
via the synchronous read_asset host API on update, so a
real phone might hitch; asynchronous worker dispatch, real
model boat controls and GPU timing telemetry remain open.
No approved NOAA/USGS geography has been bundled. This
is a source-stage integration, not a playable release.

## LOD-first hybrid seafloor geometry: source tile CPU budget

The native source-only terrain renderer now uses
`BuildHybridSeafloorLod` for resident NOAA-derived source tiles.
Rather than filling potentially 513×513 unknown samples at
full resolution and immediately decimating, it evaluates the
16/32/64k or fewer displayed LOD vertices only, retaining
original surveyed cell elevations and procedural infill tags.
Every underlying cell in a coarse quad is still checked
against the authored water footprint, so a tiny island or
shoreline hole cannot be filled by a large underwater
triangle. An invalid source datum is rejected.

The synthetic 513×513, stride-4 CI fixture expects a
129×129 mesh (16,641 vertices; 32,768 triangles) and a
compact approximately 463 KiB GPU vertex/index payload.
That is a **calculated/CI-test budget**, not measured
Android VR memory or frame time. GCC/Clang optimized test
runs retain CSV-style metrics and CPU build microseconds
as an artifact; CI timing is diagnostic only and not a
30-FPS-device acceptance threshold.

The frame-preparation callback still builds a complete
new LOD mesh and uploads it during one GL-owned frame
boundary. Although bounded, that operation and the
CPU cache's synchronous source asset reads still require
an async/staged work budget and actual Galaxy S9 p95/p99
frame-time instrumentation before the feature is accepted
as hitch-free gameplay. Sitka remains unregistered.

## Boat-position authority, authored skiff berth and depth source (2026-11-04)

Sitka's unregistered native terrain module now supports an **optional**
projected-metre `sitka/boat/launch.txt` authored berth. The parser
`SITKA-SKIFF-LAUNCH-1` validates the vertical datum against the
active bathymetry directory, named water footprint, geographic
coordinate, heading, draft and allowed embark radius. A malformed
present berth refuses source-only module creation; an absent berth
leaves the existing fishing/editor preview intact. **No fictional
boat berth is silently inserted into the installed APK.**

At an authorized berth, a new provisional two-button chord
(primary + secondary, edge triggered) launches the deterministic
skiff. After launch, `SkiffNavigation` owns the world position:
joystick X controls rudder, joystick Y controls positive throttle,
30 Hz fixed movement is water-mask/keel-clearance limited, and
the native two-tile stream prefetch now follows the boat's actual
east/north and velocity rather than the user's headset transform.
Docking requires a return near the berth at low speed. Held chords
do not repeat an action, and boat mode suppresses simultaneous
fishing input instead of passing one trigger to two systems.

The **same survey-first/procedural fallback** supplies boat safety,
4 Hz sounder and render tile choice. Launch primes a valid initial
reading; labels retain `SURVEY`, `COMPOSITE` or `PROCEDURAL`
and the simulated-navigation disclaimer. A newly loaded shallow
survey overrides procedural depths and rejects unsafe launches.

**Coordinate correction:** Authored Beckenburg `.water` footprints
are LOCAL to the terrain origin, while NOAA tiles, boat poses
and source directories use projected world metres. The shared
`SitkaWorldWater` helper rebases before collision/water tests;
the earlier direct comparison of projected metres with local
float footprints could incorrectly report 'not water'.

**Remaining:** Boat model, physical head/camera seating, visible
gauge built from the licensed glyph atlas, real launch content,
controller onboarding, asynchronously preloaded local asset
reading, installed Galaxy S9 gameplay and thermal/frametime
verification are **not** completed by this source-stage hook.
Boat mode is not exposed as a fake shell gallery launch.
