# Sitka — Surveyed Seafloor, Bathymetric World and Boat Depth Finder

**Game:** Sitka, inside the single main ReverieVR Android APK.
**Decision (2026-11-04):** Model land and underwater terrain from real, provenance-checked datasets, using the existing deterministic native simulation and GLES2 terrain engine. The seabed and boat depth finder share one geographically anchored world. Real hydrography remains authoritative where known. Unmeasured ocean floor is filled with deterministic **PROCEDURAL** game geometry, never mislabeled as a surveyed sounding.

**Current state:** An offline bathymetry raster exporter, native tile parser/sampler, boat depth and skiff movement rules, a NoData-safe low-poly seabed triangulator and optional seafloor VBO rendering in the source-only Sitka terrain module. Synthetic-data native and real-Mesa regression fixtures exist. This remains **source-only**: Sitka is unregistered, without approved geographic data, a launched boat, an on-screen depth gauge, a legally sourced coastal scene or physical Galaxy S9 evidence.

## 1. Data authority for a multi-mile world

- **Land:** USGS Alaska IfSAR / 3DEP bare-earth 5 m DEM for broad coverage, with the higher-resolution Alaska DGGS/NOAA 2018 Sitka DTM where its real license and tile footprint overlap the selected world cells. Do not use the vegetation/building DSM as the bare-earth terrain.
- **First-choice seafloor:** NOAA NCEI National Ocean Service hydrographic survey grids (validated BAG and multibeam products) for surveyed harbors and coastal channels.
- **Secondary sourced seafloor:** NOAA NCEI coastal relief/CUDEM products, tagged COMPOSITE with source resolution and uncertainty.
- **Gameplay infill:** Deterministic globally seeded seafloor, influenced by proximal approved guidepoints and tagged PROCEDURAL where actual source data is unavailable.
- **City:** approved OpenStreetMap/municipal vectors for irregular roads, authentic footprint polygons, grade, trails and public waterfront, with attribution/license reviews.

References:
- NOAA bathymetric coverage/download: https://www.ncei.noaa.gov/maps/bathymetry/
- NOS survey archive: https://www.ncei.noaa.gov/products/nos-hydrographic-survey
- USGS Alaska IfSAR: https://www.usgs.gov/ngp-user-engagement-office/alaska-mapping-initiative
- Sitka DGGS lidar: https://dggs.alaska.gov/pubs/id/30531
- NOAA vertical-datum conversion: https://vdatum.noaa.gov/

Each approved source records provider, dataset/survey ID, download/acquisition date, version, licensed redistribution rights, source and output SHA-256, horizontal/vertical CRS/datum, tile coverage, sampling resolution, vertical-transform method/uncertainty and source-quality classification.

**Coordinates:** +X east, +Y up, +Z north. Original projected/metre anchors remain doubles; GPU positions use local, rebased floats. **Never mix MLLW, NAVD88 or other land/seabed/tide vertical datums silently.** The exporter requires the declared terrain and bathymetry datum to match exactly after independently validated preprocessing (VDatum where actually supported). There is no automatic datum conversion, Google data extraction or network access. Intentional seeded marine terrain is allowed as clearly labeled gameplay infill, not as factual geographic data.

## 2. Implemented source-only tile and conversion contract

Source scripts: scripts/sitka/build_bathymetry.py and app/src/main/jni/native/sitka_bathymetry.h.

The desktop converter consumes an approved, north-up, projected-metre one-band GeoTIFF and a separate source provenance JSON with provider, dataset, license, retrieved date, URL, matching terrain and seafloor vertical datum, explicit elevation encoding (elevation_up or positive_down_depth), and quality (surveyed or composite). It does not change CRS/datum, invent NoData values or download anything.

Binary SITKAB01 format is little-endian, with a 56-byte header:

| Byte offset | Type | Meaning |
| --- | --- | --- |
| 0 | ASCII[8] | SITKAB01 |
| 8 / 12 | uint32 | columns/rows (2..513) |
| 16 / 24 | float64 | first cell-center easting/northing in metres |
| 32 / 40 | float64 | positive east and south sample spacing |
| 48 | uint32 | FNV-1a canonical vertical-datum identifier |
| 52 | uint32 | reserved zero |
| 56 onward | 5 bytes/cell | float32 seabed elevation (metres, positive up), uint8 quality |

Quality codes: 0=NoData, 1=Survey, 2=Composite. NoData requires a zero stored value, and native bilinear queries require all four covered source pixels. Reject corrupt sizes, nonfinite samples, datum mismatch, invalid tile names, duplicate tiles, invalid quality and overlapping tile coverage. Read only from the host-scoped sitka/bathymetry/ asset prefix.

Each bake emits a manifest.json for provenance and SHA-256 audit, index.txt for approved tile filenames, and the native SITKAB01 data. The current Atlas is a bounded **16 resident tiles**, with explicit failure beyond that limit. A 257×257 tile is approximately 330 KiB of raw file payload, not a measured S9 memory cost. Streaming/LOD for arbitrarily large Sitka Sound regions is a subsequent phase.

## 3. Deterministic skiff sounder

Source: app/src/main/jni/native/sitka_boat_navigation.h, combined with the source-only bathymetry reader.

For a valid boat sample within an authored water footprint:
- depth below the game surface = source-derived water-surface elevation minus source-derived seabed elevation;
- clearance below the vessel keel = that depth minus the virtual vessel's draft;
- status = safe, shallow/under-keel, dry, outside water or NO DATA;
- source quality = SURVEY or COMPOSITE.

The water-surface level must come from an authored game tide model in the *same vertical datum*, **not a claimed real-time Sitka tide prediction**. The depth finder refreshes every 0.25 seconds on the simulation update, never during either-eye drawing. Unmeasured seabed beneath an approved water footprint receives a separately flagged PROCEDURAL depth. Outside water, invalid datum/configuration and unavailable gameplay coverage remain NO DATA.

The skiff model is fixed 30 Hz, gently accelerates with a 4 m/s target cap, responds to bounded throttle/rudder, blocks movement into off-water, unsafe-depth or NoData zones, and never forces head/camera roll or yaw. The vessel draft is a gameplay attribute, not a factual real-boat measurement.

**Next visible HUD — designed, not implemented:** Lato text inside the virtual console: DEPTH 8.4 m / KEEL 8.0 m / SAFE / SURVEY (or COMPOSITE), with NO DATA for gaps, and SIMULATED — NOT FOR NAVIGATION underneath. No actual phone sonar sensor is implied. The measurements are geographically sourced *game simulation*, never suitable for real-world marine navigation.

## 4. Exact development order and acceptance

1. Acquire approved downtown / harbor land DEM and NOAA bathymetric survey/composite files; audit actual coverage and vertical datum; hash source and conversion metadata.
2. Bake and validate a real harbor chunk, plus an aligned public shoreline, water footprint and small navigable boat launch area. Synthetic CI fixtures cannot be staged as geography.
3. Validate and optimize the **implemented source-only underwater mesh** against actual licensed seabed/water masks. The C++ geometry baker refuses triangles spanning NoData, the terrain GLES2 renderer uploads shared 16-bit batches once per context, and the unregistered prototype has a conservative 300,000-vertex visual ceiling until streaming/visibility culling exists. Prove water/land clipping and measured Galaxy S9 performance.
4. Bind a real skiff model, launch/dock, input actions, collision, fixed-step water movement and the 4 Hz depth model in the actual native game, not as an inert launcher item.
5. Implement depth/clearance HUD using the licensed Lato glyph atlas, with clear missing/estimated labels. Verify water-surface and sea-floor datum, tide changes, and high/low-grounding cases.
6. Expand to continuous multi-mile coastal cells, ferry/boat corridors and inshore/offshore fishing. Stream adjacent cells with stable projected origins, seamless geographic boundaries, LOD and versioned save identities.
7. Author non-grid downtown roads and P0 polygon-accurate buildings, refine P1/P2 individual architecture and selected P3 public interiors, with no street displacement or pretend private access.
8. Package approved source assets with attribution into the single ReverieVR APK and verify offline launch, two-eye parity, saves, real hardware inputs, comfort and sustained 30 FPS on the Galaxy S9.

Native regression: deterministic C++ parse/sampling/draft/NoData/index/datum/tick/boat tests under GCC and Clang, plus Python GIS fixture for projected GeoTIFF export and source/license rejection. No code-stage pass alone means playable Sitka, actual bathymetric coverage, installed APK or device acceptance.

## Source-only visual seabed renderer milestone — 2026-11-04

The native `sitka_seafloor_mesh.h` baker now converts genuine tile-sample
elevations into low-poly terrain coordinates rebased against the existing
land mesh origin. It chooses a bounded LOD stride; a decimated quad is
emitted only if **all** covered source pixels are valid, so it cannot
draw a solid fabricated floor across a bathymetry gap. Color is a tiny
source-derived PSX palette, not a downloaded texture.

Existing `TerrainGl::CreateWithExtras` uploads land and seabed into the
**same** GLES2 shader and draw-batch path, preserving the old land-only
`Create` call for compatibility. Static VBO/EBO allocations occur once
per GL context, not per eye; the native source-only Sitka module now
loads bathymetry index+tiles and bakes additional meshes at context create.
Missing survey coverage is never rendered as a triangle. Full-atlas
visual admission has a conservative 300,000-vertex guard; the proper
multi-mile streaming/LOD renderer is still a separate integration phase.

This does not implement water transparency, underwater visibility/fog,
terrain-water shoreline masks, multi-mile continuous rendering, an
instrument drawn in the boat console or real Sitka geographic assets.
The existing water shader is provisionally **opaque**; the seabed will
therefore generally not be visible through the surface from above until
the water renderer gains appropriate depth/opacity treatment. Real
Mesa stereo/context-loss validation and on-device profiling are required.


## Geographic tile streaming and region transitions — 2026-11-04

**Source implemented (no S9 device acceptance):**

- The baker exports `SITKA-BATHY-INDEX-2`, a geographic directory carrying
  projected-metre bounds, grid resolution and tile filename without eagerly
  reading seabed samples. Up to 2,048 indexed source regions are supported
  per authored input. The old 16-tile V1 index is still readable for legacy
  authored source-only projects.
- `sitka_bathymetry_stream.h` loads at most **two CPU seabed tiles**:
  the tile beneath the boat/focus, plus one ahead when five seconds of
  bounded velocity predicts a known crossing. Read-only depth samples do
  not initiate I/O. Unknown cells return NoData and boat rules block passage.
- Tile headers are checked against actual indexed grid coordinates,
  dimensions, datum and spacing; mismatches fail closed. The loader issues
  at most one local read on each `Pump` update, with a 15-update failure
  retry gap. It tracks reads, loads, failures, evictions, resident tiles
  and **sample payload bytes**, not whole-process resident memory.
- The unregistered native Sitka prototype now recognizes V2 and uses the
  headset/editor position as a provisional focus for CPU streaming;
  a real skiff's projected position/velocity must replace that during
  full boat integration. A loaded tile may appear on GL context creation,
  but **in-motion GPU swapping is not implemented yet**. The host's
  `read_asset` is synchronous; no async or hitch-free boat IO is claimed.

**CI regression targets:** Synthetic 8 km / 40 tile continuous travel,
predictive prefetch, seam crossing, 120-frame boundary jitter, no redundant
IO, at-most-two resident cells, eviction, corrupted origin/datum, auto-retry
and boat sounder sampling directly from the same cache. Python GIS tests
exercise V2 directory export and more than 16 source tiles. The Action
workflow retains transition CSV evidence. These are test-only numeric grids.

**Further requirements:** Implement verified async data loading before
shipping a moving boat; keep GPU uploads in an explicit GL-safe
once-per-frame preparation stage outside either eye. Add distance-and
hysteresis-based LOD for actual terrain/seabed/roads/buildings; test
multi-mile adjacent tile seams and stable building IDs. Measure host
IO/decode p95/p99, dropped stereo frames, true RSS/GPU bytes and sustained
Galaxy S9 30 FPS before accepting performance. Never reuse stale seabed
depth as a current sonar reading across an unloaded region.

## Procedural proximal seafloor — gameplay policy (2026-11-04)

The survey-first world no longer ends at a missing NOAA sounding. New
`sitka_procedural_seabed.h` supplies deterministic world-coordinate noise
with a fixed world seed and gradual weighting from nearby real bathymetry
guidepoints, independent of tile load/eviction order. At shared generated
tile edges, positions and heights match exactly. Ocean coverage is
restricted to authored water polygons, not arbitrary land generation.

The approved GeoTIFF exporter writes `procedural-guides.txt` with at
most 256 fixed, source-derived guidepoints, source-datum ID, world seed,
game reference water level and proximal influence radius. The manifest
stores a SHA-256 of this guidance and marks it PROCEDURAL - GAME ONLY.

Loaded SOURCE cells always override generation. When one of their
pixels is missing, the gap-filler can replace it using the same
procedural height function, without modifying any surveyed cell.
Outside the source directory the same function generates a 256 m
world-aligned tile with 16 m samples for water-mask-approved locations.
Boat soundings must label these heights PROCEDURAL, never SURVEY.

These are video-game depths, not navigation-grade soundings. The
unregistered native Sitka prototype now has source+generated mesh
authoring logic but no actual commissioned NOAA tiles, launched
boat, on-screen depth finder or proven dynamic GL tile swap yet.
Device runtime latency/thermal and real source-guidance blending
are still unverified.

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

## Sitka-scoped authored asset admission (source stage)

The host-owned read-only `MaximumBytes` classifier now recognizes
the **future** `sitka` module id for eight exact authored source
paths (terrain, water, lighting, weather, fishing, boat launch,
bathymetry V2 index and sparse procedural guides), plus only
strictly formatted `sitka/bathymetry/bathymetry-rNN-cNN.sitkab`
files capped at `56 + 513*513*5 = 1,315,901` bytes.
Other module IDs have no access to these paths; Sitka
cannot read foreign game paths, arbitrary user saves, URLs,
traversal, nested paths or alternate extensions.
These are path/size permissions, **not** registration of the Sitka
module or evidence that any real NOAA data ships in an APK.


## Modular physical helm foundation — 2026-11-04

An owner-provided 17-page boat bridge and upgrade design is being implemented in the existing Sitka native source. The first CPU-only implementation (sitka_helm.h) provides 16 physically anchored equipment sockets, four increasingly large walkable hull footprints, a steering-column origin that never shifts, a bare dinghy wheel and compass, gated depth/GPS/radar/overhead readouts, a reach-limited one- or two-hand wheel gesture with 10-degree haptic ticks, a separate fixed-30Hz rudder actuator, and boat-relative passenger coordinates that do not force the headset's orientation. Purchases accept the existing fishing-credit amount by reference. Equipment prices are provisional gameplay balances, not values supplied by the document.

Verification: sitka_helm_test.cpp is added to the existing GCC/Clang native gate. This is a CPU model and layout contract, not yet a finished physical model, marine screen GL texture, saving implementation or playable VR boat. Save integration, launch/interaction bindings, bought-part meshes, deck collision, readable physical screens, and comfort-tested camera transitions remain pending. Sitka remains source-only/unregistered pending approved geodata and headset evidence.

Persistence/runtime follow-up in the same development slice: hull and fitting ownership now use the existing fishing journal V2 (backward-compatible with V1), with purchases updating the exact fishing-credit balance and marking the journal dirty. The source-only Sitka boat session consumes the 30 Hz smoothed rudder output and updates gated instrument readings from the already sampled sounder after movement. These readings are still CPU-side; full VR-grip input binding, in-world screen mesh rendering and shop UI remain unimplemented.


## Procedural boat hull and boat-space GPU mesh (2026-11-04 continuation)

The previously CPU-only helm descriptors now have a **renderable, low-poly GLES2 boat mesh** using `sitka_boat_mesh.h`: four tapered hull shapes with open walkable deck surfaces and gunwale bands, a fixed helm wheel/column, brass compass casing, and optional equipment structures (physical screens are dark/unlit until proper type and live marine data displays are integrated). Its geometry is built at initial GL context creation from saved hull and purchased fittings, using the existing 16-bit TerrainGl batch path. Active launched vessels render at their projected bathymetric world pose using a local `u_Model` transform incorporated into the eye matrix; uploads occur once per context, not per stereo-eye. The local water footprint determines the base sea height. The renderer remains separate from headset transforms, keeping VR view orientation unchanged.

Acceptance contracts: native `sitka_boat_mesh_test.cpp` checks hull growth, upgrade visibility, 16-bit index batching, deterministic shape, model-matrix positioning and invalid inputs. The existing real Mesa/EGL stereo test now renders a physically transformed boat mesh under both eyes, checks GL state restoration and verifies one upload per context. These are *native source-only* improvements: approved real Sitka geography and an actual VR boat shop / fishing traversal / text-rendered electronic displays remain release gates. The fixed compass needle is intentionally absent until its card can track heading, rather than displaying a misleading direction.


## Real-font in-world boat instruments — source-only continuation

Purchased sonar, chartplotter, radar and overhead-bar equipment now produce CPU-side instrument text layouts in `sitka_boat_panels.h`, consumed by the shared licensed Lato Semibold alpha-atlas renderer on physical boat screen faces. Sonar uses the existing derived depth/keel/quality samples and carries `SIM - NOT FOR NAV`. Weather condition comes from the game profile; the radar explicitly says `RADAR UNAVAILABLE` until a sweep is implemented. Chartplotter says `NO MAP DATA` rather than pretending unsupported chart imagery exists. The overhead bar displays true simulated heading and commanded throttle, without inventing fuel level. The initial dinghy has **no digital text**. Labels update only when their content changes in the simulation tick, then draw with the boat model matrix and depth occlusion in both eyes. Font creation is a strict dependency, with Android -lz linked for the existing font atlas. Tests cover gating/provenance and real Mesa Lato draws with GL state preserved; headset readability remains unverified.

Aft-facing marine displays now mirror Lato vertex X about each panel's center (UV data stays unchanged), so text reads from the captain's position behind the helm; CPU and real Mesa tests exercise this correction.
