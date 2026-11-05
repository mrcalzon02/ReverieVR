> **2026-10-30:** The internal desktop games manager now has a Sitka project selector, separate named region files, shared Beckenburg editor palette and transaction history, and direct access to the actual Sitka water/weather authoring application. The portable editor, water and weather source is bundled and version-pinned; it does **not** turn Sitka into a playable native game. See [internal game editor integration](INTERNAL_GAME_EDITOR_BECKENBURG.md).

# Sitka

## Corridor-authoritative walking junctions — 2026-11-05

The source-only `WalkSession` now owns its currently occupied authored
pedestrian edge. It stays on that segment even when another sourced path
crosses or overlaps in horizontal X/Z coordinates. Stepping onto a different
segment requires BOTH an actual shared source endpoint node and proximity to
that node; the existing 0.30 m elevation-step limit still applies. Sideways
sliding is constrained to the active corridor, not whichever crossed path
happens to be closest. No road, path, crossing, bridge approach or legal
public access is inferred by the runtime.

The existing `sitka_walk_session_test.cpp` regression now covers a
visually crossing but unconnected source corridor and a lawful shared-node
turn. This is a narrow source-level correctness repair, not installation
of current municipal footpaths, a playable Sitka map, validated physical
sidewalk collisions or Galaxy S9 device acceptance.



## Daydream touchpad mapped to sourced footpaths — 2026-11-04

The Sitka walking session's native joystick movement now has a
**Sitka-only Daydream-controller touchpad input path**. While an approved
Sitka module is running, `VrShellRenderer` sends its existing
`TouchpadLocomotionGate`-validated analog pad axes into
`NativeModuleRuntime.setSitkaWalkTouchpad`. A fresh finger release is
required before initial movement or after click, menu block, stale BLE
packet, module switch or controller interruption. Click remains select.
The native module's `WalkSession` still owns all elevation/path
constraints and actual world translation; there is **no** global
increase of the shell's 100 m room-travel bounds or noclip path.

A connected gamepad moving above the 0.2 analog threshold always has
priority over the Daydream pad. Sitka's input override clears on
module stop and is never applied to other games. The pure Java
`SitkaWalkTouchpadControlsTest` checks mapping/clamping, gamepad
precedence and touch re-arm safety; the C++ walking regression and
existing Cardboard/native tests remain separate acceptance gates.

This enables input routing **for future source-approved Sitka
sessions**, not a playable public Sitka APK: the optional native
module is still unregistered until real terrain, roads, waterfront
elevations and required content are staged and physically validated.


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

## Harbor deck plank texture, shared once, source outlines kept — 2026-11-04

In addition to the source-shaped float massing, Sitka's
`sitka_shore_deck_planes.h` triangulates **only the top of an explicitly
authored pier/pontoon polygon**, using UVs that repeat every 2.4 metres
instead of subdividing the deck into extra geometry. The 0.01 metre
overlay is for texture visibility; it makes no separate collision,
water buoyancy, navigable dock or building-interior claim. The existing
irregular polygon and per-feature identity remain unmodified.

The `sitka_shore_deck_gl.h` renderer now uses the APK's existing
`native/shared/v1/materials/coastal/dock_planks.png` (64 x 64) through
a **Sitka-only explicit catalog grant**. No copy of the texture goes into
another asset pack. It decodes the PNG once on module creation and uploads
**one texture + one compact vertex buffer per GL context**. The flat
textured tops share exact feature indices and the near/far visibility
tiers of `ShoreStreamPlanner`, so distant/hidden harbor docks issue
no extra draw calls; both VR eyes reuse the same GPU allocations.

The dedicated CPU geometry test enforces simple concave polygon
triangulation, UV repetition, determinism and rejection of broken
rings. The real GLES2/Mesa stereo test checks the shared texture,
flat-top rasterization, zero draw on hidden features, host GL state
restoration, context recreation, and invalid alpha rejection. If the
optional texture is absent, Sitka retains untextured source-shaped P0
harbor massing rather than inventing a dock or crashing.

**Geographic status:** this is source-backed *rendering capability*;
no claimed 2026 Crescent Harbor source polygon, float elevation,
photographic layout or installed playable scene is substituted by
the synthetic regression fixture. The boat workstream is untouched.

## Sitka authentic harbor polygons and bridge draw-call culling — 2026-11-04

The optional `SITKA-SHORE-1` authoring now permits **real projected-metre
polygon outlines for piers and pontoons**, not just guessed rotated rectangles.
A licensed harbor GeoJSON Polygon goes through the same stable source-ID,
current existence, CRS, provenance, 32-point and non-self-intersection gates.
It supplies its own base elevation and deck thickness; neither is inferred
from a terrain DEM under water. The runtime extrudes the *source polygon*
with ear-clipped top and flat sides. The `sitka_shore_detail.h` decorator
never attaches generic building windows and awnings to a polygon float.
Existing explicit Point structures remain supported for independently
documented simple rectangular objects, but **Crescent Harbor's seven-float
arrangement must eventually use the actual official harbor footprints**.

The O'Connell Bridge's already cached, three-material GLES2 renderer now
uses a tiny CPU-only `BridgeVisibility` gate from the authored world-space
bridge mesh bounds. It draws when the listener is within 1,250 m of any
bridge part, retains visibility to 1,450 m to avoid threshold flicker, and
skips **all three draw calls in both eyes** beyond that band. Source geometry,
GPU buffers, materials and stereo-eye pose are unchanged by this culling.
Invalid listener coordinates leave the last validated state untouched.
The gate resets on GL context loss and recomputes before eye rendering.
New regressions cover irregular pontoon geometry, no invented float facade
and boundary hysteresis/coordinate-origin equivalence.

These are source/runtime **capability additions**, not a claim that the
current Crescent float polygons or O'Connell alignment have been installed:
the shoreline requires authorized current geographic footprints, calibrated
water levels and site-reviewed architectural imagery.

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

## O'Connell Bridge: native 3-material GLES2 textured rendering — 2026-11-04

The **simple-plane** bridge renderer is now implemented as
`sitka_bridge_gl.h` and hooked to the source-only `sitka_terrain_module.cpp`.
`sitka_bridge_layout.h` accepts an **optional**, strictly bounded
`SITKA-BRIDGE-1` layout under `sitka/structures/oconnell.bridge`. It
requires a reviewed source feature identifier, 64-hex SHA-256 attribution,
EPSG:32608 projection, origin agreement with the actual terrain mesh,
documented deck dimensions, and individually authored road-grade samples.
It rejects an absent/invalid identity or mismatched origin; it **never
generates bridge endpoints or pylons** from the LOC point locator.

For N grade segments, the native builder emits exactly **2 side sheets + N
road-top planes + N underside planes**, with UVs. The GLES2 layer builds
**three** cached material VBOs and textures (side/road/underside), and
draws them in the same stereo world coordinates, with alpha cutout on the
two billboard sides. It reuses the APK's existing 64×64 concrete PNGs
(`quay_concrete` on road and `reinforced_concrete` underneath) through
two narrowly scoped Sitka grants in the shared-asset resolver; no second
texture copies or new model-heavy material system are installed.

A **512×128 original PSX silhouette** can be generated once on the CPU as
an alpha texture from the publicly documented twin-pylon/harp-stay pattern
when the optional `sitka/structures/oconnell_side.png` is absent.
That is an explicitly *stylized approximation*, not a surveyed rendering
or an extracted/stolen photograph. A source-reviewed 512×128 replacement
PNG takes precedence later without changing the two-sided geometry.
Texture decoding occurs at module creation; GLES2 texture/VBO uploads
occur once per context, **never per eye or frame**. Malformed optional art
omits the bridge rather than producing opaque boxes or damaging the
existing terrain/seabed/water/boat.

`sitka_bridge_layout_test.cpp` exercises source/CRS/geometry fail-closed
admission. `sitka_terrain_egl_test.cpp` exercises the real GLES2 bridge
program, alpha mask and both eye views, host GL state preservation,
material slots and context recreation. The NDK cross-link gate compiles
the actual modified Sitka module for ARMv7 and ARM64. The bridge still
**does not appear in a playable Sitka game**: licensed/current projected
alignment, terrain-matched grades and an approved world asset are
pending. No automatic default bridge placement is permitted.

## O'Connell Bridge — strict simple-geometry budget (2026-11-04)

**Design authority: SIMPLE.** The real John O'Connell Memorial Bridge must
retain verified alignment, deck width, grade and recognizable appearance, but
*does not* require individually modeled girders, cable stays, rivets, trusses,
road grooves, or large prop counts. Use source-reviewed photographic/silhouette
art for two **single flat alpha-textured side panels** (one per side of a
verified straight span). The **road deck** is a sequence of flat textured
quads following sourced grade samples; the **underside** is a separate series
of flat textured quads. No subdivided geometry for the concrete pattern.
Plain UV repeats are cheaper and clearer.

A game-private CPU-only implementation in
`app/src/main/jni/native/sitka_bridge_planes.h` generates **2 + 2N quads**
for N road grade segments: exactly two side sheets, N road-top planes,
N underdeck planes. Quads have per-surface UV/material tags. Road and
underdeck reuse the two existing approved shared concrete PNG materials:
`quay_concrete.png` and `reinforced_concrete.png`. The O'Connell side
alpha sprite is deliberately **not** bundled or authorized yet because its
artwork must be checked against actual bridge images. The source-only
`sitka_bridge_planes_test.cpp` checks counts, UVs, material splits,
determinism and rejection of invented/bent/nonfinite input profiles.

**Reality check:** this is a source-only geometry/texture contract; it has
**not yet been connected to a runtime UV shader or placed into Sitka's
geographic scene**. The side image, sanctioned local bridge endpoints,
surface grade, and physical render/device acceptance are still pending.
It must not silently reuse generic `bridge_ramp` panels as the authentic
bridge or convert old HAER locator coordinates into fabricated pylons.

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

## Shoreline geographic authenticity correction — 2026-11-04

The source-only shoreline uses **real site identity and GIS footprint data**, not
an interchangeable coastal village kit. The new source-backed catalog
`data/sitka/shore-reality.json` records five separate municipal harbors,
Crescent Harbor's seven numbered floats, the distinctive **cable-stayed**
John O'Connell Memorial Bridge, surviving Katlian and Japonski heritage
structures, and historic sites which must **not** reappear as extant buildings.
Every catalog entry has verified *identity* sources, but no generated geometry:
all are marked `not_acquired` until legal source geometry and projection are
confirmed. In particular the original Sitka Cold Storage burned in 1973;
it must not be placed as a present-day cannery. See
`scripts/sitka/shore_reality.py` for the fail-closed import checks.

The shore exporter now requires a source feature ID, present-day existence,
and a site catalog; named structures cannot be exported until matching
source-checked GIS identifiers and CRS/hash evidence exist. Buildings lacking
a documented height or source levels are no longer assigned a generic number
of storeys. The runtime's existing illustrated P1 windows/awnings are now
**off by default**; without an explicit, audited per-site approval, the
renderer uses real-footprint P0 massing even nearby. This corrects source
fidelity without touching skiff, water, bathymetry or headset recovery code.

## Shore distance-tier rendering — 2026-11-04 (source integrated; device proof pending)

The optional licensed `sitka/shore/waterfront.sitkash` feature set now stays
separate from the base land mesh. `sitka_shore_stream.h` keeps only approved
shore metadata resident and builds geometry when a feature crosses a world-space
visibility threshold: near P1 stylized facades at 80 m entering / 112 m leaving;
P0 silhouette massing to 440 m entering / 490 m leaving; fully hidden outside.
Distance is to the actual irregular polygon or rotated block *bounds*, not an
invented town grid or the object's centre. A shore-only GLES2 overlay reuses
the existing transactional static VBO/EBO replacement, protecting terrain,
seabed and boat buffers. The optional `prepare_frame` hook commits an entire
new shore selection before either stereo eye renders, with 30-frame retry
backoff after invalid geometry or upload failure; GL-context restoration resets
LOD state. This is **in-memory source-geometry selection**, not geographic
asset paging, completed P2 landmarks, survey data installation, or measured
Galaxy S9 performance. A synthetic C++17 shore planner regression checks
near/far transitions, hysteresis, rollback, rotated bounds and deterministic
context rebuild. Full source/NDK/Mesa/real-device gates remain open.

## Source-only seafloor and boat instrument milestone — 2026-11-04

Sitka now has a **source-only, deterministic bathymetric tile converter and native
depth-finder/skiff model plus source-only triangulated seabed rendering**, in addition to its existing terrain, water/weather,
lighting and fishing building blocks. NOAA/USGS-derived real world cells still
have to be acquired and checked for horizontal/vertical datum, source rights
and coverage; no real Sitka depth reading is displayed yet. The prototype
requires an approved bathymetry index and native tile assets, but remains
**unregistered and unplayable**. See [Sitka bathymetry and skiff specification]
(SITKA_BATHYMETRY_AND_BOAT_SPEC.md) for binary schema, surveyed/composite
quality, explicit NoData, source provenance and the next on-boat HUD milestone.


> **Expanded whole-city specification (2026-10-28):** [Sitka — Complete City, Coast, World, and Gameplay Specification](SITKA_CITY_WORLD_DESIGN_SPEC.md) is the current authority for entire-city exploration, buildings, georeferenced roads, Malefic Veil city/terrain reuse, the actual Beckenburg shared water and weather modules, town/boat upgrades and unified ReverieVR APK delivery. This document retains the original coastal gameplay scope and evidence history. Earlier independent game-package or content-pack statements are superseded by the unified main APK policy.


**Official title:** Sitka (no subtitle).  
**Architecture:** Real irregular Sitka buildings and roads, geographic P0 placeholders upgraded to unique P2 architecture; [normative custom-building specification](SITKA_IRREGULAR_CITY_AND_CUSTOM_BUILDINGS.md).  
**Proposed project ID:** RV-GAME-SITKA; tentative module ID `sitka`.  
**Category:** ReverieVR > Native Apps / Projects, once functional.  
**Stage:** Source-only native terrain rendering proof, now tested with real Mesa EGL/GLES2 stereo pixel readback and context recreation; no real Sitka geographic content or playable VR game.  
**Platforms:** Inherits the full ReverieVR host targets (Android Daydream, native Windows/Linux/macOS desktop, and provisional OpenXR on Windows/Linux); Daydream Galaxy S9 remains the 30 FPS reference.  
**Reference:** Owner-supplied overview image of Sitka Sound, 2026-10-26.

## Archive discoverability — 2026-10-30

**Sitka is explicitly listed in ReverieVR's combined Classic + Native Home gallery as an inspectable in-development project.** Its entry is a clearly labeled procedural coastal identity illustration, **not** an actual captured Sitka geographic landscape; clicking shows the planned coastal gameplay, current terrain/editor milestone and reasons it is not yet playable. The inspector stays inside the normal Home gallery and uses the existing dismissal action without hiding Home controls. Sitka is not entered in the 15 native game-module identity matrix or the screenshot archive as a fabricated playable game: the existing terrain-proof module still depends on approved real coastal mesh, water and weather assets and has no fishing loop. Gallery discoverability does **not** confer gameplay or geo-accuracy acceptance. The entry should transition to an actual selectable native game only when it can launch a working Sitka Sound shoreline experience, with screenshot evidence from that experience.

## Current authoring milestone — 2026-10-26

**Source integrated on ReverieVR `main`, not a playable game:** A separate desktop [Sitka editor and terrain pipeline](SITKA_EDITOR_PIPELINE.md) has been added. The editor uses the genuine Beckenburg Java 17 editor palette/water controls and the same independent weather module consumed by Malefic Veil. A deterministic offline DEM+shoreline-to-low-poly terrain baker requires independently supplied, appropriately licensed geographic datasets; the Python synthetic-fixture regression passed. A game-private C++17 water/weather data adapter and host regression passed GCC/Clang sanitizer checks. Real Sitka geographic tiles, desktop GUI compile/preview, *geographic* terrain/water rendering, native game registration, playable APK and S9 headset execution remain **unverified and unimplemented**. Do not equate authored profiles with visual rendering, or expose an inert app icon.

## Terrain rendering source proof — 2026-10-27

The original `.sitkam` binary contract now has a strict native C++17 reader and
GLES2-safe 16-bit triangle batching. The game-private terrain renderer uses the
existing native shader/matrix/GL-state helpers, immutable indexed VBO/EBO uploads
once per GL context, and the same buffers for both eyes. A source-only
`sitka_terrain_module.cpp` ABI prototype consumes a bounded host `read_asset` mesh
and rejects missing/corrupt content. **It is deliberately not registered in
the host's playable module catalog and has no launcher control**: no verified
licensed Sitka mesh has been staged, and no fishing/shore gameplay exists.

The four-vertex synthetic `.sitkam` fixture and >65,535-vertex chunk test
passed GCC C++17 and Clang ASan/UBSan host execution; a fake-GLES2
lifecycle test exercised static uploads, two eye draws and context rebuild.
A separate real Mesa driver regression now also passes actual stereo pixel
readback, host state restoration and context recreation.
The subsequent [exact-commit NDK CI run](https://github.com/mrcalzon02/ReverieVR/actions/runs/40644268181)
**passed ARMv7 and ARM64 API 21 GLES2 shared-library link, ELF identity,
exported-entry and dynamic-dependency checks** with pinned NDK
`27.3.13750724`. The source-only libraries are retained as
[CI proof artifacts](https://github.com/mrcalzon02/ReverieVR/actions/runs/40644268181/artifacts/12796477836).
This is **not** a real GL driver, APK, S9, geographic or playable-game
acceptance. See [terrain pipeline](SITKA_EDITOR_PIPELINE.md) for exact
checksums, the earlier failed run and remaining acceptance gates.

## Verified real-driver rendering gate — 2026-10-28

The source-only `TerrainGl` now passes a real Mesa surfaceless EGL pbuffer
regression: an ES2-requested context rasterizes a synthetic red terrain
triangle into two separate eye viewports, preserves the host's GL state,
uploads the static VBO/EBO batch once per context, and survives release and
recreation of a second unshared context. Mesa reported an **OpenGL ES 3.2**
software driver, so this does not claim a strictly ES2-only device or headset
acceptance. The [exact-code six-job CI run](https://github.com/mrcalzon02/ReverieVR/actions/runs/40763852143)
passed, including both Android NDK source-only ABI links. See
[terrain pipeline](SITKA_EDITOR_PIPELINE.md) for the detailed verification
and remaining geographical, Android, comfort and gameplay gates.

## Identity

A gentle, relaxing VR fishing and beachcombing game along the real coast of Sitka, Alaska. Visit recognizable islands, beaches, harbors and coves. Fish, explore tidal pools, spot crabs and collect rare shells. Upgrade rods, reels, bait, tackle and boats to access new places. There is no combat, countdown, punitive failure, forced chores or grinding requirement. Quiet exploration must itself be enjoyable.

**Loop:** choose coastal destination on nautical-style chart -> travel by shore or boat -> explore from comfortable stationary/teleport viewpoints -> cast and reel, beachcomb, observe marine wildlife -> document discoveries in a field journal -> return to dock -> upgrade tackle or vessel -> explore farther.

The landscape is geographically grounded but stylized in a beautiful low-polygon PSX-inspired aesthetic using techniques learned from The Beckenburg Project and The Malefic Veil. Those projects' desktop renderers are **not** assumed compatible with ReverieVR GLES2; transfer suitable mesh, occlusion, material, LOD and caching techniques deliberately.

## Host platform inheritance

Sitka is a native game hosted **inside the complete ReverieVR platform**. Its Windows, Linux, macOS, Android Daydream and optional OpenXR target modes derive from the **whole-project** [desktop expansion](DESKTOP_PLATFORM_EXPANSION.md), not from a separately planned Sitka-only desktop application. Game-specific environmental and fishing rules remain here; hub/renderer/OS delivery responsibilities belong to the platform plan.

## Coastal-world mapping

Use real Sitka Sound geography, with high priority on coastline contours, relative island and headland positions, harbor silhouettes, mountain profiles, coves and forested slopes. Author varied shoreline pockets in the appropriate positions on a **regional chart**, rather than a fabricated island map. The player's supplied map is a geographic brief, not redistributable game terrain.

Potential initial destinations to survey: Crescent Bay, Starrigavan Bay, Halibut Point and the Indian River outlet, all identified as Sitka-area shoreline fishing locations by Alaska Department of Fish and Game. Evaluate geographic accuracy, public access, local data resolution and game suitability before selecting a first playable pair.

### Mapping source requirement and rights

Owner requested Google Earth accuracy. **Do not scrape, trace, reconstruct, extract or cache Google Earth/Maps imagery, meshes, terrain or geodata to create offline game assets**. Google prohibits using Earth output to rebuild 3D models or similar content, and Google's Map Tiles API prohibits geodata extraction and offline caching beyond its terms. Any optional live Google Maps Platform integration must be independently licensed, network-aware and attributed, and cannot be an offline low-poly data pipeline. Reference: https://about.google/brand-resource-center/products-and-services/geo-guidelines/ and https://developers.google.com/maps/documentation/tile/policies .

Build accurate offline game terrain using properly sourced downloadable data instead:

1. **USGS 5 m Alaska IfSAR/3DEP** elevation for ridgelines and headland shapes: https://www.usgs.gov/ngp-user-engagement-office/alaska-mapping-initiative
2. **NOAA coastal/shoreline and bathymetry products** for coastline, seabed and shore gradients where available, respecting source coverage and disclaimers: https://www.ncei.noaa.gov/products/bathymetry
3. **OpenStreetMap** geographic features where useful under ODbL attribution/database obligations: https://www.openstreetmap.org/copyright

For every ingested source record download source, version/date, attribution/license, tile coverage, CRS/vertical datum, checksum, generated recipe and derived outputs. Transform to bounded local coordinates, triangulate and decimate terrain offline, build independently licensed texture/mesh atlases, blend seam boundaries, and package local data for offline play. Validate coastline-to-elevation alignment and tide/shoreline approximation. This is artistic geographic reconstruction, **not** marine navigation.

## Activities

### Fishing
- 3DoF-friendly controller ray aims and click casts; reeling and drag/tension use a simple trackpad gesture or held action with accessibility alternatives. No tracked arm translation required.
- Realistic species candidates: salmon (king/coho/pink), Dolly Varden, halibut, lingcod, rockfish and flounder. Season, water depth, habitats, tide, bait/lure and weather mildly affect odds without harsh fail states. The ADF&G confirms such Sitka fish communities: https://www.adfg.alaska.gov/index.cfm?adfg=ByAreaSoutheastSitka.fishingOpps&fishery=Bottomfish
- Cast -> subtle nibble cue -> engage -> reel within forgiving tension -> land and identify -> decide to retain/release where appropriate -> journal entry, sale or upgrade reward.
- Deterministic seeded fishing simulation, limited active fish, rare encounters with meaningful discoverability; catalog includes species, size records, locations and favorite lure.
- Do not treat virtual harvest rules as actual regulatory advice. If featuring deepwater rockfish release, respect descending-device conservation guidance.

### Shores
- Walk/teleport between clearly accessible beach pockets; find unusual shells, beach stones, fossils, driftwood and discoveries in tide pools.
- See and optionally gently inspect crabs, kelp, starfish and shoreline animals. Ambient gulls, eagles and seals enliven the environment without chase objectives.
- Rare discoveries use regional biome and tide context, not arbitrary glitter collectibles. Shoreline collections and field-journal photos can be displayed at the home dock.

### Upgrades and boat
- Rods, reels, line, hooks, baits, lures, tackle storage and convenience gear have meaningful differences.
- Vessel candidates: starter small skiff or rowboat -> modest motor skiff -> compact cabin boat. Boats extend reachable chart destinations and provide safe/comfortable storage and calmer operation, not mandatory speedrunning.
- Dock has functional boat berth, chart, tackle rack, equipment store, aquarium/collection display (where appropriate), journal, departure and return controls.
- No progress loss for an unproductive outing.

## Comfort and presentation

- Start in seated comfort mode. Prefer destination selection, shore-point teleport and optional gentle boat transit. Do not impose head-bobbing, deck-to-camera bob, forced roll/yaw, surprise acceleration or weather-related nausea.
- Smooth sailing is optional, with snap turns, adjustable comfort vignette, motion intensity and instant return-to-dock.
- Audio is a major feature: swash/surf, light wind through conifers, birds, distant rain, rod and reel, footsteps over gravel, water droplets and skiff motor; support quiet mode and independently adjustable audio.
- Season, time of day, mist, rain and wind should change color, behavior and ambience; initial slice may use handcrafted presets rather than live weather.
- One simulation update and two eye renders, shell-owned input/recenter/Quick Menu/exit/HUD, GL context lifecycle, isolated versioned saves. Do not let a live controller occasionally activate headset gaze-select beams. Controller loss preserves recovery without accidental casts.
- Low-poly GPU terrain with immutable indexed static buffers, offline decimated terrain and LOD meshes, baked light/vertex color, limited alpha, atmospheric fog, sky and far-shore impostors, tiny animated surf strips, seeded compact wildlife, cached shader programs. No per-eye geometry rebuild, no per-frame terrain upload, no expensive water reflections or dynamic shadow cascades. Profile 30 FPS and thermal performance on the actual S9.
- All in-world signage, species names and menus must use real readable font assets/glyphs; no substitute pseudo-letter geometry.

## Phased delivery (future work, not authorized by this proposal)

**A — Geographic proof:** select one small cove; document legal source data, coordinate conversion, accurate coarse terrain/shoreline meshes, reproducible chunk/LOD generator.

**B — Environment proof:** generate two related exploration pockets; shoreline, foliage, atmospheric audio, sea surface, stable comfortable headset view and measured stereo/thermal baseline.

**C — Fishing:** one real rod, two or three species, cast/bite/reel/land, identification and persistence. No decorative fake fishing mechanics.

**D — Beach discoveries:** shells/rare finds, moving crab with pickup or examine, field journal and persistent collection.

**E — Boat and gear:** launch/return, two destinations, an actual upgrade purchase affecting play, persistence/save compatibility.

**F — Integration and acceptance:** Android module descriptor and versioned asset package, Native Apps / Projects registration **only when functioning**, native C++/NDK/APK checks, S9 physical launch, 30 FPS thermal/frame pacing, audio, shell/exit/reconnect, save/load and update-delivery behavior.

**Minimum honest playable acceptance (at each actually supported platform):** launch Sitka from ReverieVR; visit two recognizable Sitka shoreline destinations; cast and land a fish; inspect a crab or rare shell; upgrade tackle; depart and return; save/exit/relaunch to intact progress; hear environmental audio; no loss of shell recovery. If a feature cannot perform, it remains a design entry rather than an inert button.

## Project separation and scope

Sitka is an **independent native game concept** beside Breakwater Battery, Gumball Armada, Red Ledger, Omega Front and the other ReverieVR native modules—not a Breakwater mode, Classic Game or ROM. It inherits platform runtime/render/input/delivery and optional PC XR capabilities from the **ReverieVR hub as a whole**; it is not a separately scoped desktop product or SteamVR integration. Sitka requires its own module ID, state, assets, packaging and update manifest when operational. The project name remains **Sitka**.

This record specifies the Sitka game and documents its **source-integrated authoring pipeline only**. It does not claim an independent GitHub repository, a surveyed Sitka mesh, a working GLES2 renderer, an executable Sitka module, installed game, published APK or headset validation. Continue implementation only within the user-authorized environment milestone and explicit next work.

## Fishing mechanics foundation — 2026-10-30 (source only)

- Added game-private CPU fishing rules in `app/src/main/jni/native/sitka_fishing.h` and a deterministic host test in `app/src/test/native/sitka_fishing_test.cpp`. Local GCC/Clang C++17 and Clang ASan/UBSan tests passed. This is not headset or playable-runtime acceptance.
- Seeded waiting/bite/reel/land with Dolly Varden, coho, pink salmon and flounder; bounded updates, forgiving tension, catch journal, three tackle tiers and strict versioned journal parser. It is not surveyed species distribution or real-world harvest guidance.
- These rules require independently confirmed-water/depth inputs. A water rectangle alone cannot establish safe/correct fishing geography; casting must later be gated by verified shoreline/bathymetry/land masks.
- There is no native controller/pointer/visual or host-save integration yet. Terrain/water rendering remains an unregistered source-only module. Next: approved real Sitka geography, safe cast targeting, font-based UI feedback, host-scoped persistence, and Android/desktop playtests.

## Fishing host adapter and fishing-site targeting — 2026-10-30 (source only)

- Added game-private FishingSession for normalized input edges, bite/reel state, versioned durable journal read/write, guarded corruption handling and pause/exit retry. The existing host owns save slots; the prototype uses sitka-fishing-v1.sav. Invalid saves block creation instead of silently overwriting progress.
- Added the SITKA-FISH-SITES-1 authored-site parser and ray target gate. Each row names a site, existing water id, east/north centre, radius in metres and surveyed depth. Maximum 32 sites; bounded ray distances; invalid, unknown-water and out-of-water cases fail closed. The prototype now loads sitka/fishing/sites.txt through host scoped asset reading.
- This does NOT establish a certified marine bathymetry or shoreline mask. Survey provenance, dry-ground collision, and fish-site validity need geographic acceptance before any actual Play entry. No synthetic sites or map tiles are packaged.
- The source-only terrain module now executes fishing updates once per frame, provides host-owned activation/failure feedback, and saves the journal on meaningful changes. It remains absent from Android.mk and the playable gallery. Visual rod, line, fish, journal UI, audio, and purchases are not integrated.
- Local strict GCC/Clang C++17 and Clang sanitizer tests passed for both new CPU components. The existing content CI has objective functionality tests for them. Exact-commit CI and physical Galaxy S9 evidence remain separate.


**2026-10-31 source-only lighting bridge:** The editor's real `lighting/sitka.properties` schema now has a bounded native reader. Only validated authored **ambient intensity** modulates synthetic GLES2 terrain vertex colors; dynamic light beams, falloff and shadows are not implemented. See [terrain pipeline](SITKA_EDITOR_PIPELINE.md). This does not admit Sitka to the playable module catalog or establish geographic/S9 acceptance.

## Lazy seafloor sample tile cache (2026-11-04)

The [bathymetry specification](SITKA_BATHYMETRY_AND_BOAT_SPEC.md)
now covers a V2 geographic tile directory, a two-resident-tile CPU
cache and predictive boundary loading. Read-only sonar samples never
trigger disk IO, and missing data remains unknown. This is a
source-only precursor to multi-mile rendered land and underwater
streaming and an actual launchable boat, not proof of finished gameplay.

## Survey-first / generated-world seabed

The missing-bathymetry policy is now gameplay-oriented: where NOAA
measurements exist they are retained, but unmeasured ocean is
generated deterministically with proximal source guidance. Depth
readings identify PROCEDURAL terrain. See the
[bathymetry specification](SITKA_BATHYMETRY_AND_BOAT_SPEC.md).

## Skiff-authoritative bathymetry source stage

An optional, strictly-authored projected-world skiff berth now
allows a deterministic launch/dock prototype. When launched,
the skiff (not the headset) drives seabed tile selection and
four-hertz survey-first/procedural depth pings. Geographic
coordinates are correctly rebased for existing local water
footprints. This is integrated source code and regression tests,
not a playable game or a displayed boat HUD.


## Shore-side source development — 2026-11-04

The separate shore workstream now has a game-private `SITKA-SHORE-1` geometry importer and static massing baker for individually anchored polygon storefronts/canneries/warehouses, wharves, floating-pontoon proxies, pilings, volcanic shore rocks, forest backdrops, harbor shacks, bridge decks and sloped bridge approaches. This uses the **existing** terrain GLES2 batch renderer and preserves the existing independent seabed replacements, with no boat-system changes. It is **source-only**: the optional `sitka/shore/waterfront.sitkash` input has not been populated with approved real Sitka building polygons, structures, coastal heights or licenses; no new game launcher or APK content is implied. See `docs/native/PROCEDURAL_CONTENT_STANDARD.md` for schema/limits. Next: legally sourced harbor geometry and elevations, terrain/shore clipping, tile-level shore streaming/LOD, unique landmark exteriors, collision/nav, actual filtered or point-sampled approved PSX texture atlas, and in-headset/Daydream regression.
