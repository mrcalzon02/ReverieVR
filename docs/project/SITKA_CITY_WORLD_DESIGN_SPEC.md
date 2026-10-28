# Sitka — Complete City, Coast, World, and Gameplay Specification

**Authority:** Game design specification, revision 1, 2026-10-28.  
**Canonical game name:** **Sitka**. Never add a subtitle.  
**Project:** ReverieVR native-game family, prospective game identifier \`sitka\`.  
**Related documents:** [Sitka canonical brief](SITKA.md), [Sitka environment and editor pipeline](SITKA_EDITOR_PIPELINE.md), [ReverieVR platform expansion](DESKTOP_PLATFORM_EXPANSION.md).  
**Delivery policy:** **One main ReverieVR Android APK**, with the finished native Sitka game and its licensed runtime data bundled into the host. Do not resurrect independently downloadable native-game APKs or mandatory content packs. Later desktop platforms ship their own complete platform-native packages.  
**Implementation state:** This document is a **design specification**, not a declaration that a city simulation or playable Sitka game exists. The existing editor imports Beckenburg's Java 17 weather/water/editor sources; the C++ source-only terrain, native environment adapter, and Mesa/NDK tests exist. Real surveyed city geometry, runtime weather/water rendering, geography, walks, boats, fisheries and reference-device acceptance remain pending.

## 1. Product vision and non-negotiable boundaries

**Sitka is both a peaceful coastal fishing game and a geographically recognizable, explorable 3D interpretation of Sitka, Alaska.** The entire developed city should eventually be traversable at human scale, with its real street network, shorelines, terrain slopes, harbor basins, public waterfront, bridges, trails, recognizable landmarks and buildings in defensible approximate positions. The city and surrounding Sitka Sound form a continuous geographic identity. Water activities are not an isolated minigame, and land activities are not a decorative dock menu.

Two equally legitimate ways to play:

1. **Ashore:** Walk the city and public paths, examine buildings and storefronts, visit useful interactive interior spaces, discover viewpoints and shore access, repair/customize a personal waterfront base, shop for provisions, photograph sights, beachcomb, meet fictional townspeople, and record local discoveries.
2. **Afloat:** Select a boat, depart a functional berth, travel through navigable waterways, fish tidal and offshore habitats, visit coves/islets, collect coastal discoveries, return with catches, and improve boat, equipment and harbor facilities.

Neither loop requires doing the other as punishment. Walking, looking and listening can be the entire session. The game has no combat, arbitrary survival timer, aggressive monetization, forced repetitive quests, punitive failure, compulsory real-time appointments or automated travel that removes the pleasure of a familiar place. Story is observational and community-centered, with optional simple requests from fictional residents.

The whole city is the **long-term coverage goal**, not a claim that every private home can be entered or every individual object is uniquely modeled. All publicly accessible street/sidewalk/trail/shore corridors within declared developed-area coverage must be traversable; richly interactive interiors are purposefully selected, and the rest have coherent exterior geometry and controlled entrances. Never invent a real person's private interior or advertise a hazardous/restricted real-world route as public access.

## 2. Source authority: the Sitka geography cannot become a fictional procedural city

Geographic data must be legally sourced and traceable. Use Google Maps/Earth **only where permitted for human-scale general orientation and comparison**; do not trace/digitize roads, roofs, facades, footprints, 3D meshes, elevations or other map content out of Google and do not scrape/cache its proprietary data as offline game assets. Google does not become an input dataset to the offline compiler. An optional genuinely licensed Google-hosted display is a separate clearly attributed online UI, never the terrain or building generator.

**Preferred geodata stack and source checks:**

| Layer | Candidate authoritative source | Construction rule |
| --- | --- | --- |
| Elevation, mountains, slopes | USGS 3DEP/Alaska IfSAR DTM/DEM; higher resolution lawful tiles where available | Survey-derived terrain vertices; distinguish bare-earth DTM from surface/vegetation DSM; never infer elevation from a Google map |
| Shoreline and beach boundaries | NOAA shoreline/coastal datasets, other authorized government/public datasets | Align mean-shoreline geometry to DEM and tide datum; assign uncertain mudflat/shore transitions explicitly |
| Bathymetry/harbor depths | NOAA NCEI bathymetry and source-specific licensed hydrography where coverage allows | Separate shallow and deep water collision; depth cannot be invented from missing data without labeling authored approximation |
| Streets, footpaths, docks and many building footprints | OpenStreetMap under ODbL, independently licensed municipal GIS | Use native vector geometry and verified attributes; conform to the local terrain; respect ODbL attribution/share-alike/database boundaries |
| Parcel/road/utility details | City and Borough of Sitka GIS resources, **only after explicit permission/terms review** | Municipal data publication does not automatically grant unrestricted redistribution |
| Building elevations, roof shape, façade identity | Independently licensed survey/building datasets, owner-authored meshes, owner-created/reference-cleared photographs and independent field observations | Mark observed vs estimated values; no Google 3D reconstruction |
| Local species, habitat, season, wildlife | Alaska Department of Fish and Game, NOAA, other wildlife science sources | Model ecology conservatively; don't present virtual fishing conditions as real-time legal advice |

Authoritative source entry must record provider, dataset name, retrieval timestamp, URL, license, data snapshot/coverage, checksum, horizontal and vertical CRS/datum, transformation version, exceptions, and any manual authoring provenance. Provide an in-game Credits/Map Data panel and a machine-readable source ledger. Do not distribute Google screenshots as a game texture. Direct OSM data and any derivative database must be handled under the applicable ODbL obligations; consult license review before distributing mixes of ODbL data and hand-authored meshes.

**Reference sources and rights:**  
- Google Maps Platform content restrictions: https://cloud.google.com/maps-platform/terms  
- City and Borough of Sitka GIS: https://www.cityofsitka.com/Government/City-Departments/Planning-Community-Development/GIS-Map-Resources  
- USGS 3DEP: https://www.usgs.gov/3d-elevation-program/about-3dep-products-services  
- OpenStreetMap: https://www.openstreetmap.org/copyright ; https://osmfoundation.org/wiki/Attribution_Guidelines  
- NOAA bathymetry: https://www.ncei.noaa.gov/products/bathymetry

### 2.1 Coordinate and accuracy contract

Use double-precision geodetic (lat/lon in WGS84) as retained metadata. Choose an appropriate **projected meter CRS** for each build and normalize vertical datum prior to mixing elevations/tides. Document the projection in build metadata; never hard-code guessed Sitka asset coordinates. Convert to right-handed local scene meters: **+X east, +Y up, +Z north**, using a stable origin per regional world cell. CPU stores global projected positions in doubles; GPU uses floating **origin-rebased** local coordinates to avoid jitter. Preserve exact boundary edge samples on shared tiles. The world and chart must round-trip an anchor within the source's documented survey tolerance.

Data uncertainty is a first-class field: \`surveyed\`, \`community_mapped\`, \`field_authored\`, \`procedural_infill\`, or \`estimated\`. A landmark should never appear as a confirmed surveyed reconstruction when it is a generic approximation. Buildings, piers and road centerlines keep immutable geographic anchor IDs and source references. Procedural seeds may affect paint, vegetation and small props; **they must not move streets, bridges, shoreline or building footprints from their anchored locations**.

### 2.2 Coverage tiers

- **Tier A — Downtown walkable core:** Streets, sidewalks, harbor faces, Castle Hill, Crescent Harbor area, connection to the O'Connell Bridge area, visible and usable public waterfront; landmark geometry, shopfronts, player home/dock and selected interiors.
- **Tier B — Connected urban and coastal neighborhoods:** Interconnected public-road corridors, hillside residential exteriors, major commercial/industrial exteriors, ANB Harbor/port-related shore, community amenities, forest edges and approved trailheads. Never force the whole town into a square cell-grid.
- **Tier C — Extended city and outer-road corridors:** Reachable roadside and trail-based viewpoints toward Halibut Point, Starrigavan, Indian River access, and other verified public destinations; lower-density geometry with full collision at accessible routes.
- **Tier D — Sitka Sound and nearby islands:** Connected navigable coastal water and recognizable shoreline/islands, with several landable coves and beachcombing zones. Distant mountainous regions can use skyline meshes/LOD without universal walkability.
- **Tier E — Long-term completion:** Entire mapped developed Sitka road/pedestrian network and coast-focused playable coverage with completeness inventory by zone. Private interiors remain selectively fictionalized/non-enterable, and remote cliff/mountain zones are represented even if inaccessible.

Each zone declares a polygon boundary, road length represented, buildings matched/estimated, number of working exterior doors and interiors, accessible paths, shoreline coverage, source quality and test status. An honest coverage map differentiates fully playable from silhouette-only areas.

## 3. Direct reuse of The Malefic Veil and Beckenburg technologies

**The Malefic Veil is an engineering source of proven techniques, not a city map to copy.** Review the current source and license/provenance of each asset and helper before transferring code or meshes. Reuse the following specific concepts, with a Sitka-specific geo-authoritative adapter:

| Source system | What it actually contributes | Required Sitka adaptation |
| --- | --- | --- |
| Malefic \`CityLayout\` | Connected road graph, parcel footprints, doorway and collision identity | Replace fictional seeded road creation with imported real street/footpath graph, surveyed footprints and special grade/bridge/harbor nodes; reuse integrity tests |
| Malefic \`UrbanTileAssembler\` | Actual modular road, paving, wall, doorway, roof and prop placements | Drive instances from Sitka road classifications and actual building polygons/terrain grades; derive façade/roof from surveyed attributes and local style archetypes |
| Malefic \`UrbanInstanceBatches\` | One immutable source mesh per model, packed transforms for repeated geometry | Bake arrays per loaded Sitka chunk; use verified GLES2-friendly static mesh batches on S9; no unsupported GLES3 instanced-draw assumption |
| Malefic \`CityPlateTerrain\` | City foundations, smooth outer terrain transitions, road-aligned exits | Fit surveyed road height profiles and building pads into USGS topography without artificial flat city islands or retaining-wall seams |
| Malefic \`ContinentLayout\`/\`ContinentZones\` and performance docs | Sector indexing, preloading, deterministic generation/cache keys and visibility budgets | Replace invented geography with region/quadtree tile IDs, source hashes and near-camera streaming; reserve outskirts for lower LOD |
| Malefic \`KenneyCityAssemblyTest\`, \`UrbanInstanceBatchesTest\`, city visual gates | Geometry/collision coherence and source-instance repeatability | Add Sitka-specific road-connection, frontage, coastal, height, entrance, collision, GPU and readability tests |
| Beckenburg \`EditorProject\` and \`EditorPalettePanel\` | Java 17 editor hierarchy/palette and authoring tools | Add authentic georeferenced city, building, road, water and weather tabs rather than duplicating/rewriting editor systems |
| Beckenburg \`dev.mrcalzon.water\` | \`WaterDocument\`, \`WaterEditorModel\`, \`WaterCanvas\`, \`WaterEditorPanel\`, independent \`WaterModule\` | Keep genuine .water authoring, extend export to coastal polygons/sea fields and use a bounded native visual/simulation adapter |
| Beckenburg shared \`dev.mrcalzon.weather.WeatherModule\` | Weather state machine/presets, wind, sky, wetness/audio semantics | Keep the Java editor behavior; export deterministic wind/precipitation/fog/time presets to the native C++ runtime and reconcile with tides |
| Malefic \`VeilWeather\` | Demonstrates same shared Java 17 weather library in another game | Follow compatibility boundary, not game-specific host/reflection/JAR mechanics |

**Important transfer correction:** Malefic's Java2D city renderer is **not** proof of hardware GPU instancing. The original 1×1/1×2 Kenney mesh scale must be respected: do not stretch source roof or road modules 6× just to cover a macroplot. Keep geometry unique/cached and per-chunk transforms immutable; choose **premerged static meshes and VBO/EBO batches on GLES2**, and evaluate true instancing only on renderers/API versions where supported. Do not copy Malefic's bleak post-apocalyptic hazard/resource/safe plots into modern Sitka: reuse the shape/occupancy/road tools, define new civic/coastal archetypes.

### 3.1 Verified existing Sitka integration and unimplemented gap

- **Existing editor, in source:** \`tools/sitka/SitkaEditor.java\` imports \`WaterCanvas\`, \`WaterDocument\`, \`WaterEditorModel\`, \`WaterEditorPanel\`, \`WaterPlacement\`, \`WeatherModule\`, \`WeatherType\`, \`WeatherView\`, \`EditorPalettePanel\` and \`EditorProject\`. \`tools/sitka/launch-editor.sh\` compiles against real, checked-out Beckenburg Java 17 sources; no forked fake editor.
- **Existing native adapter, in source:** \`app/src/main/jni/native/sitka_environment.h\` parses Beckenburg v1 Sitka \`.water\` settings and editor \`weather/sitka.properties\`, computes deterministic wave heights and mapped weather parameters. It **does not yet render complete ocean optics or a live weather scene**.
- **Existing native terrain source proof:** \`sitka_terrain_mesh.h\`, \`sitka_terrain_gl.h\`, \`sitka_terrain_module.cpp\` and \`scripts/sitka/build_coast.py\`. The .sitkam reader and real Mesa stereo test exist; the module is **not registered as playable** and licensed Sitka tiles are not included.
- **Next real integration:** Add city-footprint and road source ingest; layer roads/buildings/terrain co-registration; compile shared water/weather presets and coastal data into runtime manifests; implement actual GPU water, rain/fog/sky, wet ground, fishing and city walk; validate on S9. An editor control or a serialized parameter by itself does not count as rendered behavior.

### 3.2 Module procurement / upgrade strategy

Reference **actual** upstream repositories and pinned commits (The-Beckenburg-Project, The-Malefic-Veil). Create a reuse ledger with exact source paths, SPDX/license, revision, required changes and tests. Keep the reusable Java weather/water/editor sources in authoring tools and translate only needed runtime data/algorithms to GLES2 C++17 after tests: Android must not need Java Swing/Graphics2D, Beckenburg as a running game, or a PC host. Avoid dragging the entire Kenney archive, private model vaults or unlicensed assets into the single ReverieVR APK; stage only approved instances and transformed delivery formats.

Changes to shared reusable technology should ideally be developed upstream or in a clear adapter layer, not divergent copied forks. Avoid editing Malefic's game state, combat, save format or world generation merely to serve Sitka. Keep source/licensing review and all derived artifacts traceable.

## 4. Complete city geodata-to-PSX-model pipeline

A deterministic desktop build pipeline produces **small offline playable scene cells** from geodata and licensed art:

1. **Acquire:** Obtain exact-version USGS DEM tiles, verified coastline polygons, roads/trails, vector footprints, optional legal street/park/building data, licensed landmarks and curated water-edge features. Generate per-source checksum/provenance.
2. **Normalize:** Reproject to the selected meter CRS, reconcile sea-level/vertical datums, clip study area polygons, align bridge and road spans to measured ground, detect water/land overlaps, invalidate broken geometries.
3. **Topology:** Build walkable graph of roads, sidewalks, trail approaches, approved waterfront access, bridge deck and stairs/ramp connections. Maintain separate vehicle, pedestrian, boat and inaccessible/restricted networks. Support cul-de-sac, T and cross intersections without changing the real graph.
4. **Terrain:** Bake multi-resolution DEM terrain with shared edge vertices, road benches and limited graded pads; anchor shoreline to data, add beaches/tidal zone masks, detect unrealistic road cliffs and foundation penetrations. Never fill missing DEM coverage with random cliffs presented as real.
5. **Buildings:** Match vector footprint geometry to class and available measured height/storey/roof tags; compute foundation elevation from nearby terrain and real roads; extrude walls; roof from parametric gable/hip/shed/flat/mixed archetypes; pick credible materials and local Alaska facades. Stable building ID and exterior door orientation face an actual reachable approach.
6. **Landmarks:** Override archetype with individually authored, accurate recognizable silhouette + correct source attribution for notable public landmarks, harbor warehouses, boat facilities, bridge, civic buildings, historical sites and select shopfronts. Preserve building footprint/roof positions.
7. **Details:** Instances of approved facade parts, porches, rain gutters, piles, piers, bollards, moored vessels, fences, benches, vegetation, signage and streetlights, selected by semantic context. Docks require real vertical access and water depth; avoid disconnected or floating props.
8. **Interiors:** Where chosen, generate or author rooms from building footprint, functional entry, accessibility, collision and activity layout. Link exterior and interior as a seamless cell or low-latency portal; keep windows and doorways aligned. Private dwellings: exterior only unless distinctly fictionalized and authorized.
9. **Water/weather:** Bake shore polygons, docks, harbors, current/depth classes and tide masks into native runtime-readable files; emit shared weather profiles with explicit version numbers. Do not rasterize the water surface into static terrain.
10. **Optimize:** Section scene into camera-visible zones; generate low/medium/distant LOD, instanced-source or premerged static batches, coalesced material/texture atlases, far-mountain impostors, bounded collision/navigation primitives and audio markers.
11. **Validate/export:** Write a versioned regional manifest of tiles, scene objects, roads/boat lanes, collision, shore/water masks, textures, ecological tags, interior links and copyright/source records, plus hashes. Reject source gaps, unsupported formats and unlicensed export.
12. **Package:** Deliver source-derived binaries in the **single ReverieVR host APK** with fixed game ID, save schema and optional on-device selective asset load. No network fetching required during play.

### 4.1 Structure grammar and building classes

| Archetype | Required exterior features | Interaction level |
| --- | --- | --- |
| Single-family dwellings, duplexes, apartments | Appropriate roof, doors, windows, stairs, slope foundations, siding, porches | Exterior; non-private interaction only |
| Downtown commercial / waterfront retail | Storefront glazing, awnings, signage mounts, street-facing doors, rear loading | Select shops/interiors: tackle, provisions, crafts, clothing, books |
| Working harbor facilities | Weathered timber/metal siding, storage cages, tanks/gear, docks, pier piles | Charter, boat equipment, fuel, repair and job boards as fictional systems |
| Municipal / civic buildings | Distinct footprint, entrance, roof geometry and wayfinding | Public-facing lobbies/galleries if authored and legally defensible |
| Heritage / culturally significant landmarks | Unique surveyed silhouette, contextual signage and appropriate interpretation | Landmark observation; respect cultural permissions and local knowledge |
| Hotels / visitor spaces | Multi-floor facade, lobby geometry where selected | Optional interior, no assumption of unrestricted guest rooms |
| Schools/clinics/utilities | Readable real-world exterior with no implied public entry to restricted areas | Exterior only unless clearly simulated fictional public space |
| Marinas, sheds, fishing floats | Timber/metal modules, walkable ramps, mooring cleats, buoyancy/height behavior | Functional dock/boat transition |
| Forest outbuildings and cabins | Lightweight procedural wood construction, safe trail approach | Selected exploration shelters |

Variations must be deterministic with source-preserving seeds. A flat generic “building box” is acceptable only as a **far LOD** and must not count as a finished near-field building. Never use pseudo-text or warped letter meshes: use approved actual glyph atlases for street and storefront signs.

### 4.2 Collision, pathfinding and elevation

- Pedestrians use simple capsule/ellipsoid physics based on ReverieVR's host conventions. Streets, sidewalks, steps, interiors, doors and fences have independent authoritative colliders, not merely inferred visual meshes.
- Navigation mesh follows terrain grade and real access, connects via tagged links for ramps, stairs, bridge transitions, docks and ferries, and marks impassable water/cliffs/secured areas.
- Road graph and parcel boundaries originate from geodata, not the procedural render mesh; preserve connectivity through every zone boundary.
- Foundation generation honors water table, steep grades and drainage; correct visible terrain penetration with actual retaining segments rather than moving the whole city onto a flat plateau.
- Interaction raycasts and pickup/cast collision use stable world positions; moving tide updates landing/wading zones only through explicit tide policy.
- No accidentally walkable sea surface, floating poles, traversable cliff walls, sealed public doors, double-stacked building entrances or paintings masquerading as interactables.

## 5. City and region streaming / rendering

**Human-scale world with continuous travel intent.** In a single zone the player walks or teleports without loading screens. Crossing a zone boundary prewarms the next map/mesh/collision chunk; large distances and islands may offer optional chart teleport/boat fast travel as an accessibility convenience, not an excuse to remove actual traversal.

**Proposed tunable—not measured—budgets:** 64–128 m urban ground chunks, smaller detailed interiors, broad low-res terrain regions and 200–500 m scenic sea chunks; choose actual dimensions from source coverage and profile memory. Target sustained 30 FPS on Galaxy S9, with ~33.3 ms frame period inclusive of input, simulation and two eye draws. Treat every initial per-frame draw/triangle/texture number as a test-set budget, not a claim about S9 limits.

- One simulation tick, two eye renders; no per-eye resource creation, random variation, water phase update or gameplay state mutation.
- Static scene geometry uploaded once per GLES context, indexed 16-bit meshes split before reaching index limits. Chunk-level visibility culling; building/forest/town LOD switch; occlusion where meaningful.
- GPU meshes/materials deduplicated by stable source ID, asset revision and LOD. Premerge static repeated elements for GLES2 rather than assuming hardware instancing. Avoid full-city allocation on launch.
- Preload current chunk, forward-direction neighbors and expected boat approach; release distant chunks after nonblocking save/asset checks.
- Separate lightweight collision/network from cosmetic graphics; visual quality reductions never remove movement and fishing rules.
- Frame-time tracker reports p50/p95/p99 CPU update, GPU estimate where supported, upload spikes, resident memory, texture budget, triangles/draws and sector hitch count. Record thermal throttling and 30-minute walking and boating traces on real hardware.
- Desktop clients may raise render distance/textures/foliage, but share coordinates, rules, world coverage and saves. Desktop VR should use HMD-specific timing rather than phone defaults.

## 6. Weather module: actual state affecting every city and marine subsystem

**Canonical authoring owner:** Beckenburg Java17 \`dev.mrcalzon.weather.WeatherModule\`, the shared weather stack also consumed by Malefic Veil. **Current editor:** SitkaEditor already imports the real module and serializes \`weather/sitka.properties\`. **Current runtime:** C++ \`sitka_environment.h\` recognizes nine named conditions and derives rain/snow/fog/wind parameters; this is presently data interpretation, not a finished GPU weather system.

### 6.1 Required weather adapter

Create and version an offline \`SitkaEnvironmentSnapshot\` / native \`WeatherState\` with simulated time/day/season, clear/overcast/drizzle/rain/heavy-rain/fog/dense-fog/snow/storm, wind speed/heading/gust, precipitation, temperature, humidity/wetness and visibility bands. Native C++ advances a deterministic simulation **once** per tick and feeds consistent immutable frame-state to sky, atmosphere, audio, surface wetness, foliage and ocean. Keep per-zone microclimate options limited and deterministic.

**Functional effects:** soft rain and rooftop/drain drips; wet pier reflections simulated cheaply; intermittent canopy drops; fog that occludes distant buildings while preserving local safe navigation; wind-driven conifer movement and flags; weather-readable sky/overcast lighting; gull/ambient sound adjustments; rain intensity dampens some footstep/boat cues; storms produce safe warning and optional shelter/return. Time-of-day changes practical signage/window lights, but no forced curfew.

**Coupling:** wind impacts boat handling and nearshore chop; precipitation changes surface wetness and waterfall/river outlets; weather changes visibility and wildlife/fish behavior gently; temperature and season alter palette and species availability. Do not conflate simulated weather with actual forecast. Optional opt-in real-world weather display requires a separately licensed source and *never* blocks offline simulation.

### 6.2 Weather tests

Validate editor profile round-trip, supported modes, wind heading radians/degrees mapping, deterministic seed replay, pause/resume, context loss, indoor rain exclusion, correct fog falloff, nonnegative precipitation, safe visibility at accessible routes, physical S9 alpha/overdraw and audio/camera comfort. Capture actual rendered frames for clear/rain/fog and audio activity; a source enum alone is not acceptance.

## 7. Water module, tide, bathymetry and two-sided shore gameplay

**Canonical authoring owner:** Beckenburg \`water-module/\`, its genuine \`WaterDocument\`/editor and \`WaterModule\` behavior. Its principles include independently scrolling/pixelated layers, bounded triangular waves, underwater color absorption, shoreline foam/run-up, optional refraction/Fresnel approximation and dry hull/shore overlap. **Current Sitka native adapter** parses .water placement/wave parameters and computes bounded sample heights; it must become real visual/simulation water rather than a static plane.

### 7.1 Functional ocean surfaces

- Use shoreline polygons and mapped sea/harbor masks, independent of terrain vertices. Distinguish connected ocean from freshwater and isolated tide pools.
- Represent sea level relative to a documented vertical datum and a local **simulated tidal phase**. This is not a navigation-grade prediction. Ocean basins need bathymetry where source supports; estimated shallows must be flagged in provenance.
- Surface mesh: shared wave time for two eyes; two quantized scrolling material/noise layers; cheap directional wind chop; shallow foam bands where signed shoreline depth gradient is known; reduced chop inside protected marinas.
- Land-water contact: sea surface may not appear through dry land, inside docks, within hill foundations or under boats without correct occlusion; pier columns and breakwaters occlude water appropriately. Ocean foreshore exposes beach when tide falls and closes some shore paths/collection areas when high.
- **Boats:** Seat/boat body remains a comfortable camera reference; visually animate hull independently with bounded smoothing or optional reduced/disabled roll. Collision with sea, shore and docks uses distinct low-cost physics.
- **Fishing:** Water region, bottom depth, vegetation, current and tide tags influence cast target and habitat. Reel line intersects actual surface, bobber follows modeled phase, fish approach from plausible depth; catch never despawns due to eye render.
- **Seasons:** tide-dependent discoveries, washed driftwood, exposed starfish/kelp areas, sheltered bird activity and foggy harbor reflections.

### 7.2 Quality tiers and compatibility

The existing Beckenburg pixel/refraction system may be CPU/Graphics2D-oriented. **Do not run its Java desktop renderer directly in Daydream.** Transfer exported authoring values and the tested algorithms into a bounded GPU GLES2 coastal adapter. On the Galaxy S9 baseline use a compact opaque/tinted surface, UV/noise wave motion, alpha-limited shoreline foam, static bathymetry tags and **no mandatory screen-space refraction/scene-reflection passes**. Desktop can opt into more advanced Fresnel/refraction if performance is proved. Keep the same water gameplay/tide logic across modes.

Water editor footprints are small placement tools; generating an entire coastline from dozens of manually placed squares is not acceptable. The large ocean comes from geodata-driven polygon/chunk tessellation; the editor's .water authored footprints supplement harbors, tide pools, river mouths, ponds and local water overrides. Need an explicit converter from the existing water document to ocean-chunk masks without breaking v1.

### 7.3 Acceptance

Tests: surfline stays registered with terrain across neighboring tiles; tide changes are saved/replayed; boats cannot spawn under land; dry land remains dry; dock pilings intersect correct waves; float lines land in plausible depth; clear/rain/storm water all render in two eyes; GLES context recreation reloads static buffers; no visual jitter/recomputation on second eye; profile 30-minute coastal run on S9. Water collision and surface mask must continue to work with visual effects turned off.

## 8. Movement, comfort, and geolocation

**Movement modes:** smooth walking (optional), short-range comfort teleport along mapped public walkable navmesh, snap-turn and slow smooth turn (optional), seated world-relative locomotion, selected stair/ramp assist, anchored viewpoints, seated boat helm, hands-free chart destination selection and controlled dock disembark. Preserve the host's recenter, Quick Menu, controller selection, battery/diagnostics and gaze-only recovery. Use the hub's avatar body settings and simple ellipsoid collision, with no unverified full-body physics promises.

**Virtual geolocation:** Offline chart with correct surveyed lat/lon anchors; user's avatar is shown as a map cursor corresponding to the actual current 3D position. Click/highlight a listed destination to show its historical/cultural description, elevation/shore category, accessible route, travel mode, journal entry, discovered species and optionally distance calculated in the local projected metric CRS. Waypoints, compass, mapped routes and named street headings all use **our legal geodata**, not Google route results.

**Device location:** No GPS, Android location permission or IP geolocation is necessary to walk a virtual city. An *optional explicit opt-in* “Find my real-world location in Sitka” feature can, when supported, read device coarse/fine location while the user actively requests it, put a temporary separate *real-you* pin on the chart, and allow dismiss/clear. Never silently start tracking, run background location, reveal exact live coordinates to friends, or require location for VR controller pairing. When real GPS is unavailable, the game simply continues.

**Navigation:** A site marker on the map does not teleport into unsafe water, private property or inaccessible cliffs. Waypoints select verified access graph nodes. Route paths prioritize safe public pedestrian/boat corridors and are clearly labeled **game guidance, not real-world navigation**; no real-world tide/weather/legal reliance.

## 9. Progression: both sides of Sitka should have meaningful upgrades

Two upgrade tracks share optional income, without a grind wall. Offer credits from fictional fishing sales/requests, discoveries, optional deliveries, photography/journal milestones and quiet free exploration. Purchase outcomes persist and are visible in world geometry, handling, range, storage, animations and interactions.

### 9.1 Watercraft and fishing

| Track | Tier progression | Real effect |
| --- | --- | --- |
| Vessel | Rowboat / compact skiff / powered skiff / small covered workboat / comfortable cabin fishing boat | Travel radius, weather shelter, inventory capacity, berth and handling; no mandatory speed |
| Hull/controls | Basic hull, trim, protective bumper, improved helm, stability assist | Reduced wind/drift, soft collisions and comfort |
| Propulsion | Oars, economical outboard, reliable outboard, quiet upgraded drive | Range, sound character, optional faster crossings |
| Safety/navigation | Simple chart, compass, depth indicator, enhanced sounder, lighting | Discoverable fishing depth, night usability, chart annotations |
| Rod/reel | Starter spin setup; specialist trolling, light, heavy, deep-water rigs | Cast/reel behavior, species access and tension forgiveness |
| Line/lures/bait | Stronger line, specialized hooks and lure colors, tackle collections | Habitat-aware catch probabilities; all mechanics observable |
| Boat storage | Basic fish box, organized tackle locker, insulated storage, display/live well alternatives | Higher carrying capacity and thoughtful fish handling |
| Aesthetics | Hull colors, deck mats, cabin lights, name board, seating | Player customization with no mechanical penalty |

Fish species and access approximate Sitka-area ecology: salmon, Dolly Varden, halibut, rockfish, lingcod and selected smaller harbor fish as scientifically appropriate. Fishing loop: choose location → gear → cast/troll/drop → weather/tide/habitat bite → forgiving reel/tension → identify/measure → release, log or retain under game rules. Conservation cues should be careful (especially deepwater rockfish) and no simulated legal limit pretends to be current Alaska regulations.

### 9.2 Town, home, mobility and shore upgrades

| Track | Tier progression | Real effect |
| --- | --- | --- |
| Home base | Simple dock-side room → decorated cottage/workshop → expanded storage/display → personalized waterfront headquarters | More displayed collections, storage, map wall, wardrobe/avatar continuity |
| Dock and berth | Shared landing → assigned mooring → covered gear station → safe repair/launch station | Visible mooring, shorter setup, boat care and custom berth |
| Tackle shop partnership | General tackle shelf → specialty lures → custom rig bench | Better equipment choice, authored object inspection and repair |
| Field gear | Backpack, waterproof satchel, binoculars, field camera, magnifier | Carry size, observation zoom, photo journal, rare tide-pool identifiers |
| Walking exploration | Basic map → annotated chart → trail kit → walking poles / weather clothing | Comfort, improved route awareness, rain/shore access; no stamina tax |
| Artifacts/display | Shell cabinet, beach finds wall, fish records board, photo albums, aquarium-only-where-ethical | Persistent collectible presentation and ambient room detail |
| Town services | Harbor workshop, supply requests, visitor information, fictional neighbors | Optional errands, access to facilities, richer conversation |
| Accessibility | Instant dock return, calmer sailing, high-contrast waypoints, easier reels, automated line retrieve | Opt-in quality of life, never monetized |

Each upgrade must have a corresponding tangible changed mesh/model, inventory/data effect, purchase confirmation, save persistence and rollback-safe loading. No duplicated game store versus campaign-only store; same logical inventory visible through home/dock and local shopfronts. A visual mock card with no effect is not an upgrade.

### 9.3 Suggested economy philosophy

Start with a functional rod, legal-to-play starter skiff, a basic room and ability to roam downtown. Early gains arrive through a variety of activities rather than a fixed repeated catch quota. High tiers are meaningful but the majority of the map remains accessible on foot without buying a vessel. Price balancing is separate data, versioned for save migrations. Never charge for weather or map accuracy.

## 10. Land activities, shore ecology, citizen life and buildings

**Activities:** touring scenic waterfront, reading interpretive displays, sketching/photographing views, visiting author-approved historical/cultural landmarks, finding mushrooms/rocks/shells in context without implying real-world permission to collect, tide-pool observations, checking harbor boats, optional low-stakes package errands, simple ecology field-study tasks and collecting journal narratives. These are peaceful interactions, not combat encounters.

**Wildlife:** eagles/gulls and shorebirds, seals/otters where habitat-appropriate, crab/starfish/urchins/tide-pool creatures, fish schools, conifers and forest ambience. Use tiny deterministic local agents with per-zone caps. Animals are observed at respectful distances and do not serve as always-available farmable objects.

**Town activity:** Fictional pedestrians, working fishers, shopkeepers, harbor traffic and parked vehicles, abstracted around real geographies. Avoid presenting private residents as simulated real identifiable people or replicating actual private home interiors. Traffic/boats obey road/water lane graphs and low-speed safe crossings; no physical collisions or disruption required to make the city feel alive. Some buildings are exteriors, selected public-style interiors house merchants/workstations and rest areas.

**Cultural fidelity:** Sitka includes rich Tlingit, Russian-era and contemporary local history. Do not invent sacred ceremonial details, mislabel cultural assets, or treat sensitive public sites as loot spawn points. A cultural-history chapter should be fact-checked against community or appropriate public institutional resources; optional partnership/permission is preferred for faithful representations.

## 11. Saving, world state and content authority

Per-game save includes \`save_schema\`, world data revision, seed/clock/weather/tide state, player chart position, last safe grounded position, visited zones, known waypoints, unlocked journal pages, collections, avatar customization reference, fishing gear, currency, vessel components, home/dock interior changes and optional quests. All persisted game coordinate values are stable world anchors, not transient origin-rebased GPU floats.

Migrations preserve saves across geographic revisions: remap moved waypoint/building ID via an explicit table; if a building has changed, return player to nearest safe public path and retain collection/equipment. Never wipe saves on hub APK update. Offline play, map journal and all purchased features must be functional without the Internet. Diagnose incomplete geo tiles explicitly; no map that silently changes coastline because a network request failed.

**Module integration:** one native \`sitka\` library registered in the existing ReverieVR in-process host **only after** a functioning playable slice; native game code, geo-derived mesh chunks, audio, readable font atlas and permitted models packaged in one signed ReverieVR main APK. No per-game self-updater, installer or independent Android APK release. Keep test-only authoring tools/dependencies out of runtime APK.

## 12. Exact technical package proposal (not yet implemented)

Authoring layout should remain under \`scripts/sitka/\`, \`tools/sitka/\`, and \`docs/project/\`. Proposed later files:

- \`scripts/sitka/import_city.py\`: authorized GIS/OSM vector import, CRS/footprint/road validation, source ledger.
- \`scripts/sitka/bake_city.py\`: road and city-height alignment, parcel/roof/door generation, chunking, deterministic props and accessible navgraph.
- \`scripts/sitka/bake_ocean.py\`: native coastal polygon/bathymetry/tide masks and small authoring overrides from genuine .water documents.
- \`scripts/sitka/export_weather.py\`: shared WeatherModule profile-to-native stable state contract and conformance fixtures.
- \`scripts/sitka/verify_city.py\`: cross-layer identity, topology, clearance, license, LOD and deterministic binary validators.
- \`app/src/main/jni/native/sitka_city_*.h/.cpp\`: runtime city layers, visibility, batching, walk/collision and landmark/interaction geometry.
- \`app/src/main/jni/native/sitka_water_*.h/.cpp\`: native water surface renderer, tide, shore mask, fishing intersections and debug counters.
- \`app/src/main/jni/native/sitka_weather_*.h/.cpp\`: native weather tick/sky/precipitation/fog/audio signals, derived from same authored presets.
- \`app/src/main/jni/native/sitka_fishing_*.h/.cpp\`, \`sitka_town_*.h/.cpp\`, \`sitka_*.save\`: deterministic gameplay and schema/versioned save system.

**Example dataset structures (illustrative schema, not implementation):**

\`\`\`json
{
  "schema": 1,
  "world": "sitka",
  "crs": "AUTHORING_PROJECTED_METRE_CRS",
  "vertical_datum": "DOCUMENTED_AND_NORMALIZED",
  "tile": "downtown/0001",
  "origin_projected_m": [0.0, 0.0],
  "source_snapshot": "SHA256_OF_AUTHORIZED_MANIFEST",
  "roads": [{"id": "stable-road-id", "kind": "street", "geometry_ref": "line-123", "grade_ref": "grade-123"}],
  "buildings": [{"id": "stable-building-id", "footprint_ref": "poly-123", "shape_source": "community_mapped", "height_source": "estimated", "archetype": "harbor-shop", "interior": "authored-public"}],
  "coastal": {"water_mask_ref": "ocean/0001", "shore_mask_ref": "shore/0001", "tide_datum_ref": "tidal-datum-v1"},
  "lod": ["near", "middle", "far"],
  "access": "public-path-graph-v1"
}
\`\`\`

Production schema must replace fake zero origin and placeholders with data-derived values and checksum-verified references; no dummy city asset can ship as geography.

## 13. Authoring workflows in the editor

Expand the **existing** Beckenburg-based SitkaEditor rather than creating a second pseudo-editor:

1. **Geo viewport:** licensed elevation mesh, coastline, road/footpath lines, footprint polygons, source quality overlay and landmark anchors. Read-only sourced layers unless an explicit authoring correction is recorded.
2. **City assembler:** plot/roof archetype inspector, source mesh selection, facade variation, documented measured/estimated attributes, entrances/stairs, real slope/foundation placement.
3. **Interior builder:** floor/door/interaction nodes linked to generated exterior, walkable collision tests and private-space exclusion.
4. **Terrain tool:** height-field seam inspection, road grade heatmap, foundation clipping and erosion/shore transitions; keep original DEM in provenance.
5. **Water tool:** **actual imported** \`WaterCanvas/WaterEditorModel/WaterEditorPanel/WaterDocument\`; augment with coastal masks, tide/bathymetry and dock contact, rather than replacing the authentic controls with dummy UI.
6. **Weather tool:** **actual imported** \`WeatherModule/WeatherType/WeatherView\`; validate native snapshots, sea/weather coupling, preset and day/night preview with honest renderer distinctions.
7. **Asset source library:** approved Kenney and project-generated components, per-asset license/hash, selected city-specific authoring overrides, readable fonts.
8. **Validation/export:** coverage dashboard, missing sources, visual proxy vs real mesh, unreachable doors, broken road graph, water-through-land, duplicate surfaces, naming/content tests and reproducible Android exports.

Actual desktop Java editor visual compile, real geo preview and integration tests must be performed before claiming completed capabilities.

## 14. Staged milestones and acceptance gates

| Stage | Deliverable | Evidence required |
| --- | --- | --- |
| S0 — Source contract | Register pinned upstream source revisions, licenses, working module imports and geo schema; no city yet | License/source ledger; Java editor+native adapter tests; unchanged Malefic/Beckenburg |
| S1 — First geographic city cell | One genuine downtown elevation/shore/road/footprint tile with correct anchor locations | Independent data receipts and terrain vs road/footprint checks; manual spatial review |
| S2 — Downtown procedural building proof | Actual surveyed footprints become varied 3D exteriors, roof types, reachable front doors and grade-correct roads | C++/GPU draw test, collision test, real-world alignment comparison using lawful references |
| S3 — Native environment | Ocean, pier, shoreline, weather sky/fog/rain and tide actually render with matching runtime simulation | Clear/rain/fog/tide screenshots; same source profiles; stereo/context restore; physical S9 performance |
| S4 — Walking city slice | Continuous street/sidewalk route, selected shop interior, landmark and player dock accessible | S9 head/controller/keyboard path, recenter, safe exit, correct collision and stable saves |
| S5 — Fishing/boat | Functional rod/bite/reel/land, boat launch/return, two distinguishable destinations, inventory and upgrades | Playable user interactions, observable purchased change, persistent save and offline replay |
| S6 — Downtown complete | Public corridors and relevant buildings within the declared downtown polygon, no duplicate doors or blocked sidewalks | Coverage and landmark manifest; long walking capture; no map seams or major stalls |
| S7 — Coastal city expansion | Connected extended public roads, harbors, outer coast, additional boat destinations | Route/navmesh coverage, tide/boat collisions, no fake unvisited neighborhoods |
| S8 — Full city and region | Declared developed-area city coverage and navigable Sitka Sound geography | Coverage dashboard, legal data receipts, measured 30-minute walk + boat soak, saved cross-zone transitions |
| S9 — Cross-platform | Entire game delivered through functioning ReverieVR native desktop hosts as those mature | Actual Windows/Linux/macOS executable/graphics/input/upgrade/save proof; optional PC XR tested separately |

Functional tests are release gates. Art/composition review and geospatial accuracy are **quality acceptance tasks**, not global APK publication blockers unless they break function or integrity. Do not list “entire city implemented,” “full fishing,” “GPU water,” or “S9 proof” merely because a document or script exists.

### 14.1 Immediate build order

1. Pin/verify shared weather, water, editor, Malefic city-template/instance algorithms and exact source licenses.
2. Obtain a lawful first Sitka downtown DEM/coastline/roads/footprints set with provenance; fix projection/vertical datum.
3. Extend the existing terrain baker with a real georeferenced road/footprint and navgraph city cell; test the first actual grade/door.
4. Integrate water masks and weather presets into actual native scene drawing using the already-present C++ environment adapter, with two-eye parity.
5. Put a functional walkable harbor and first fishing point into the native Sitka module *before* adding a hub menu item. Deliver only within the one signed ReverieVR APK.
6. Expand by coherent city blocks, harbor corridors and coastline until the complete mapped Sitka coverage contract is met.

## 15. Explicitly unresolved decisions

- Actual geospatial footprint availability, source rights and accuracy of building heights/interiors.
- Appropriate vertical tidal datum, bathymetry coverage and tolerable authored gap policy.
- Which first actual downtown source tiles and public building interiors are selected after surveying source availability.
- Native GPU ocean detail affordable on the Galaxy S9 after measurement.
- Safe scope/feasibility of optional live GPS and optional externally hosted map displays.
- Cultural representation and permission/review for sensitive landmarks.
- Detailed balancing of credits/upgrade tiers and authentic species/season tables.
- Realistic APK asset size and streaming limits given one unified update package.

**Design invariant:** recognizable **real Sitka geography + thoughtful low-poly synthesis + authentic Beckenburg/Malefic engineering reuse + peaceful land/sea gameplay + truthful evidence.**
