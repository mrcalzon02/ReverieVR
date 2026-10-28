# Sitka — Irregular City, Placeholder and Bespoke Building Specification

**Status:** Normative architecture/design, rev 1, October 28, 2026. **No new city implementation is claimed.**
**Canonical game:** Sitka, hosted inside ReverieVR. 
**Parent:** [Full Sitka city and game design](SITKA_CITY_WORLD_DESIGN_SPEC.md); [Existing Sitka editor](SITKA_EDITOR_PIPELINE.md).
**Authoritative decision:** **No grid-based street plan or whole-building tile sets.** Every building has an independent, real-world anchored footprint, and roads follow geographic source curves. P0 placeholders allow whole-city coverage; P1 architecture is building-specific procedural synthesis; P2 is custom modeling; P3 is an optional interactive interior. Existing world identity survives every upgrade.

## 1. Normative invariants

1. The sourced **street network, waterfront, elevation and footprint polygons dictate geometry**. A rectangular spatial grid can exist only as an invisible memory/visibility/collision acceleration structure.
2. No Malefic Veil square-lot network, 2×2 lot generator, six-cell block scale, repeated tile-built house silhouette, or right-angle road grammar may dictate Sitka geometry.
3. Buildings may be concave, oblique, L/T/U shaped, multiwing, multi-ring, courtyard-bearing, on steep hills, and partially over water.
4. **Placeholders must respect actual building polygon boundaries.** A box stretched over a polygon bounding rectangle is not an acceptable P0 building.
5. Custom structures are the architectural endpoint for recognizable anchors. Reusable parts (windows, siding, trim, doors, roofs, piles, gutters and props) may still be instanced inside custom overall geometry.
6. Each building owns durable source anchor/ID, footprint, datum/elevation, collision semantics, entrance link IDs and rendering fidelity. Swapping the rendered building must not move saved map pins, doors, paths, world interactions or inventory.
7. Heights, roof information and precise entrance position require explicit provenance. Missing attributes may be **estimated**, never presented as measured fact.
8. A first-city build can use P0 buildings across an entire district without blocking gameplay, but a downtown landmark cannot be counted complete until its unique P2 model is actually displayed.
9. Gameplay must remain functional across zone transitions, with two-eye render sharing one world update, Daydream Galaxy S9 30 FPS reference, and the whole Sitka library delivered through one ReverieVR APK when playability is achieved.
10. No proprietary Google imagery, 3D meshes, derived tracings, elevation products or satellite-based building outlines may be extracted into offline Sitka assets. See parent geodata and license authority.

## 2. Independent coordinate and topology systems

### 2.1 Three coordinate layers

- **Survey/GIS:** original lat/lon and source CRS metadata, verified elevation source and vertical datum; hold projected scene coordinates in double-precision *meters*.
- **Game simulation:** canonical world projected meters; geographic origin plus per-chunk floating origin and stable global IDs. +X east, +Y up, +Z north.
- **Graphics:** camera-relative float vertex coordinates and chunk-local transforms; seams share canonical double-precision edge/height samples and are rebased identically.

Do not use guessed Sitka coordinates. Confirm datum transformations before mixing DEM, shoreline, bathymetry, building foundations or tides.

### 2.2 Source feature types

- **RoadFeature:** stable source ID, polyline with nodes and variable width, named roads and sidewalks, road type/access, source certainty, connected junction and bridge layer, sampled grades.
- **JunctionFeature:** vertex graph node with neighbors and polygonal turn/intersection geometry. A bridge crossing a lower road must NOT accidentally become a shared junction.
- **BuildingFeature:** polygon/multipolygon rings, possible courtyard voids, ordered exterior boundary with orientation, source/reference IDs, height/roof data and uncertainty, entrance candidates and terrain interface.
- **CoastalFeature:** shore polygon, shoreline change/tide uncertainty, seawall/river mouth/jetty/float/dock classification, fixed or floating support and water depth availability.
- **PathFeature:** walkable public access, stairs/ramps/boardwalks, nav connections, graded path and restrictions.
- **Chunk:** invisible spatial index cell or quadtree node referencing all source objects intersecting it; it cannot overwrite object origin or shape.

Normalize ring winding and reject self-intersections, overlaps impossible under same elevation layer, nonfinite coordinates, invalid holes and zero-area polygons. Geometry invalidity must be a visible authoring error, not a fallback to random rectangle houses. Preserve source provenance and an audit trail of intentional corrections.

### 2.3 City layout policy

Roads are **swept ribbons** along actual centerline polylines/splines, sampled to a curvature-error tolerance. Cross sections include width, curb and shoulder as appropriate. The ends meet with polygon triangulations for oblique or skewed junctions and arbitrary branch count, not just four predefined straight/corner/T/crossroad parts. Road and pathway elevations are sampled and reconciled to terrain; bridges have independent deck elevation and support meshes. Public walking connectors/door landings must be grade-feasible.

The town must look geographically irregular at street scale. Artificial symmetry, aligned square subdivisions and repeated evenly spaced blocks are a failed implementation even if the model count is impressive.

## 3. Placeholder lifecycle and fidelity states

| State | Geometry and capability | Acceptance | Not implied |
| --- | --- | --- | --- |
| **P0 — Geographic massing placeholder** | Real polygon extrusion, basic per-wing or conservative roof, estimated/known height, terrain-appropriate foundation, stable collision and footprint provenance | One correctly anchored model for each approved building in selected area; no street encroachment | Correct individual facade, real interior, measured roof |
| **P1 — Building-specific procedural architecture** | Unique facades split along polygon edges, windows/doors/porches, irregular/multi-wing roof solver, material and foundation variants | Credible individual exterior and door/terrain alignment | Landmark identity/precise historical geometry |
| **P2 — Individually authored custom architecture** | Distinctive mesh, local silhouette, measured/appropriately sourced proportions, texture and LOD, model provenance, compatible pivot and doors | Actual unique mesh in running renderer and human comparison with legal source references | Enterable interiors by default |
| **P3 — Interactive interior extension** | Deliberately authored public/fictionalized interior, lighting, collision, furniture/interaction and verified portal/door | Walk in/out without spurious warp, blocked entrance or lost saved state | All private homes may be entered |

A P3 interior is optional, not an obligation for all P2 buildings. Most ordinary residences may remain exterior-only with no public entrance.

### 3.1 Visual upgrade contract

Before accepting P0→P1→P2, check:
- identical **stable building ID** and geographic source anchor;
- same authoritative footprint and public route relationship, except explicit source correction with migration;
- preserved interaction references, entrance IDs and destination keys;
- new model pivot/rotation/scale measured against P0 envelope;
- facade and roof bounds consistent with available geographic facts;
- collision/hull/nav updated atomically without unwalkable holes;
- checksum + asset/source provenance and LOD references updated;
- existing save loads with the upgraded mesh and still places the avatar safely.

P0 is always a real renderable world structure, not a missing model icon. P0 may be used visibly in unfinished near-field districts, and this must be counted as placeholder coverage rather than custom architectural completion. The P0 envelope can become a *far LOD* for a completed P2 building.

### 3.2 Custom modeling priorities

1. Skyline and navigation anchors visible across multiple neighborhoods.
2. Functional town buildings: player harbor base, tackle shop, dock/boat maintenance, visitor/shop interiors.
3. Distinct maritime buildings and structures (piers, fishery warehouses, mooring sites, floats, bridge approaches, harbor sheds).
4. Highly recognizable downtown structures, public places and local architectural landmarks.
5. Remaining street frontages and neighborhood architecture.

Reference candidate landmarks only after confirming rights, public access and cultural sensitivities; distinct architecture may be sourced through owner-authored models and properly licensed independent references. Do not use Google Maps/Earth-derived 3D content.

## 4. Exact building construction algorithm

### 4.1 Input contract

For each building, accept validated footprint rings, source-ID, projected CRS and reference origin, terrain/road lookup, known or estimated height and roof tags, imagery/model license ledger, semantic class, current fidelity, existing ID/entry anchor mappings, builder version and stable seed.

Algorithm, with documented fallback:
1. Project coordinates to metric local doubles; validate outer and hole polygons. Correct source problems through a separately logged authored override, never random geometry shifts.
2. Sample DEM under the footprint boundary and accessible frontage. Distinguish uphill/downhill foundation and retain nearby road grade.
3. Determine height from sourced measurement when available; otherwise estimate by building class/storey info with explicit confidence and stable seed. Record unresolved errors.
4. Triangulate footprint with holes correctly and extrude **each actual boundary edge**, creating wall faces and roof/floor polygon surfaces. A bounding-box cube is forbidden for any final P0 asset.
5. Generate roof: P0 roof proxy; P1 may decompose polygon into actual architectural wings and use roof ridge graph, shed/hip/gable/flat sections. Complex concave roofs need manual overrides or graceful clearly marked conservative roof, not twisted geometry.
6. Compute building foundation at sampled grades: flush slab, stepped/cut foundation, retaining walls, pier-supported shoreline, or elevated support. Avoid sinking or floating walls.
7. Place **entry anchors from walkable approach graph** and measured/verified facade placement when known. Unknown entrances remain unresolved/closed; do not create arbitrary publicly enterable private houses.
8. Build collision footprint/hulls, floor/steps, path access and spatial index references; preserve independent representation from LOD.
9. Bake local static mesh, appropriately licensed materials, authoritative collisions and provenance; support stable mesh revision and optional custom P2 override.
10. Export deterministic per-object and chunk manifests, hashes and source/quality status, then check P0→P2 identity and route invariants.

### 4.2 Roofs and facades

A P1 roof is an authored combination of building-specific polygon sections; it cannot assume every footprint is rectangular or single-ridged. Support flat/parapet, angled shed, gable, hip, split-height, intersecting wings, dormer add-ons and special manual ridge paths. Exposed gable ends must match actual wall geometry. If the algorithm cannot solve the roof, retain a flagged P0 roof instead of making nonsensical wedges.

Facade details repeat along **actual individual wall lengths and local normals**, not global grid positions. Windows need margin/entrance clearances; narrow wall segments may be blank. Porches/stairs, setbacks, roof overhang, gutters, painted siding and historical coastal material profiles vary by building. A unique facade geometry/material combination should come from the identity and morphology, not random decorations scattered on generic units.

### 4.3 Steep terrain, docks and shore

Malefic Veil's `CityPlateTerrain` concept provides grade/seam logic, but a *flat city plate* is not an acceptable Sitka topography replacement. Grade roads locally, retain real hillside profiles and create independently constrained building pads. For harbor structures assign fixed piles, float supports, cantilever, seawall, boardwalk or grounded class. Fixed supports and floating platforms have different simulated tide behavior; gangways remain reachable across the permitted tide envelope. Water masks and building foundations must not intersect dry terrain incorrectly.

## 5. Reuse from Malefic Veil and Beckenburg

| Existing source | Keep / transfer | Explicitly reject |
| --- | --- | --- |
| Malefic `CityLayout` | Road graph connectivity, occupancy/entrance and collision validation ideas | Seeded rectangular street lattice and fixed rectangular parcel footprint |
| Malefic `UrbanTileAssembler` | Reusable source mesh components and deterministic placement strategy | Tile-set whole-building roofs/facades that determine city geometry |
| Malefic `UrbanInstanceBatches` | Immutable source mesh reuse and transform batching | Claiming existing Java2D renderer already delivers GPU instancing |
| Malefic `CityPlateTerrain` | Edge contact, graded road/foundation transitions and crack tests | One giant flat platform city and vertical artificial city-edge walls |
| Malefic performance and urban tests | Chunk prewarm, cache identity, frame/load budgets, road/nav/asset-source checks | Fictional city scale or performance claims transplanted without measurement |
| Beckenburg `WaterDocument`, `WaterCanvas`, `WaterEditorModel`, `WaterModule` | **Genuine** water editor, authoring values, wave/contact/shore principles | Direct Swing/CPU water renderer inside Android game, or ocean assembled from dozens of square water objects |
| Beckenburg shared `WeatherModule` / Malefic `VeilWeather` | Shared source weather presets and behavior/sky/audio semantics; native-export data agreement | Assuming Java editor weather preview equals working native rain/fog GPU effect |
| ReverieVR `sitka_environment.h` and `sitka_terrain_gl.h` | Existing deterministic water/weather parameter parsing and terrain native GLES source proof | Claiming a playable/fully rendered Sitka city or game from those readers |

**Reuse mandate:** audit upstream pinned source revision, license, provenance and needed dependency before bringing code/assets into Sitka. Import only components actually needed, keep Beckenburg/Malefic runtime independent of ReverieVR, and port authoring data into compact C++ native runtime. Repeat source mesh pieces as needed but preserve individual building structure/identity.

On Galaxy S9 GLES2, do **not** assume `glDrawElementsInstanced` is available. Premerge repeated static details and upload 16-bit index-bounded chunks. The editor may use Java17 and Beckenburg tools; the runtime must not depend on Swing or a running PC.

## 6. Data manifests and editor controls

Every BuildingRecord should include:

| Field | Purpose / contract |
| --- | --- |
| `stable_id` | Namespaced persistent identity, retained across fidelity upgrades and save migrations |
| `source_ref`, `source_sha256`, `license_ref` | GIS provider/record, verified version/checksum and redistribution rights |
| `rings`, `crs`, `datum` | One or more real polygons including holes, projected meters, normalized vertical datum |
| `geometry_confidence`, `height_confidence` | Surveyed, community mapped, independently authored, estimated or unresolved |
| `grounding_mode` | Slab, stepped, piles, retaining, floating etc. with terrain/water contact contract |
| `road_links`, `public_entry_anchors` | Stable nav graph IDs, explicit allowable public access |
| `fidelity_state` | P0, P1 or P2 with optional P3 interior |
| `model_ref`, `model_revision`, `lod_refs` | Actual approved mesh instances, geometry bounds and LOD candidates |
| `collision_ref`, `nav_ref` | Durable traversable/solid behavior independent of graphics quality |
| `generator_version`, `seed` | Deterministic generated placeholder/parametric artifact identity |
| `authoring_overrides` | Audited custom roof, facade, height and entrance corrections |
| `migrations` | Map source-ID or door-ID changes when real GIS data evolves |

Source feature ID and rendering chunk ID **must not be the same**. One large building can overlap several stream chunks but must be loaded/drawn once; resident shared geometry/metadata may be referenced in each visible zone without duplicate collision. Region save references the canonical feature ID, not temporary chunk number.

### 6.1 Editor flow

Extend existing **SitkaEditor** and its genuine Beckenburg imported editor, water and weather components. It needs these working authoring steps:

1. Select the actual sourced building footprint in geographic viewport and inspect data and source accuracy.
2. Generate real P0 polygon massing and preview the *actual generated mesh*.
3. Refine via P1 wall/roof/porch/steep-grade controls.
4. Import custom P2 mesh, compare translucent footprint envelope, check pivot, actual collision and entrance alignment and reject invalid swaps.
5. Optionally attach P3 approved public/fictional interior linked to functional door.
6. Preview Beckenburg-authored weather and water settings in the editor, then validate exported *native* sky, tide and wet foundation behavior separately.
7. Verify source rights, chunk placement, collision, visual representation, sign font, safe road/nav path and deterministic export.
8. Export only the selected approved game assets into the **one ReverieVR main APK**; don't add a per-game downloader.

Controls are to be implemented before appearing, with real usable focus/activation/undo and error messages; no inert tabs, dummy preview modes or fake checks.

## 7. Test corpus and acceptance criteria

**Mandatory synthetic geometry fixture corpus:** rotated rectangle, L-shape, U-shape, acute irregular triangle, concave polygon, polygon-with-courtyard hole, adjacent unrelated buildings, multipolygon, hillside stepped foundation, fixed harbor pile, floating dock access, acute road fork, winding road, multi-layer bridge and tightly fitted waterfront shop. Synthetic fixtures test geometry only; they do NOT prove Sitka GIS coverage.

**Mandatory source-area tests:** use legally sourced actual Sitka footprints/road graph/terrain for the declared first study zone; each record needs source hash, date, CRS, license and confidence. Unavailable coverage is shown as a gap, not fabricated geography.

- `CITY-NONGRID-01`: imported vertex positions are not snapped to any construction grid; neighborhood remains nonorthogonal as supplied.
- `CITY-FOOT-02`: P0 wall/roof mesh follows all actual polygon ring boundaries with no triangulation across holes.
- `CITY-ID-03`: P0→P1→P2 visual swap preserves stable ID, save references, entry anchors and footprint.
- `CITY-ROOF-04`: concave/multiwing roof mesh has sensible clipping/normal/winding, no unexplained overlap or cracks.
- `CITY-ROAD-05`: road graph and pedestrian connectivity survive curved roads, oblique junctions, grade changes and bridge crossings.
- `CITY-GRADE-06`: foundations, doors, retaining walls and sidewalks meet terrain and remain traversable without floating.
- `CITY-DOCK-07`: tide-driven floating docks and shore contact preserve walkable boat-to-land route and do not intersect dry geometry.
- `CITY-LOD-08`: chunk boundaries, culling and LOD do not duplicate/lose structures or perturb colliders.
- `CITY-ASSET-09`: custom models require compatible source/pivot/collision/LOD provenance and real renderer execution.
- `CITY-PROV-10`: unauthorized Google-derived meshes and unlicensed assets fail import/export.
- `CITY-REND-11`: source-derived first city slice actually draws both eyes with no per-eye simulation update.
- `CITY-PERF-12`: Galaxy S9 real device walking + rain + sea + neighboring buildings profiling shows sustained 30 FPS target or documents actual bottlenecks; no speculative acceptance.
- `CITY-SAVE-13`: saved game before custom model swap restores safely after versioned asset update.

Numeric geographic tolerance is **source-accuracy-driven**, not a made-up universal accuracy promise. Compare relative footprint extents and road elevations to the specific data precision/CRS record; set tolerances per dataset and game collision needs.

## 8. Build plan, in order

**IG-0: Legal small-city seed.** Acquire a lawful real Sitka downtown DEM, independent street vectors and *non-grid* building polygons with source ledger. Confirm their geographic alignment and rights.

**IG-1: P0 occupation.** Build exact source-footprint polygon extrusion (including concave + holes), collision and height/foundation with metadata, for every valid footprint in selected region. Display a real populated city block, not a tileset demonstration.

**IG-2: Curved street and access.** Sweep streets, process intersections, attach public nav to building anchors and footpaths, and prove traversable road-grade continuity.

**IG-3: P1 diversity.** Generate distinctly shaped roofs, facades, hillside foundations, stairs and docks using source-grounded data and sampled local styles.

**IG-4: P2 bespoke proof.** Replace at least one geographically anchored placeholder with a real hand-authored distinctive building mesh, independently source-cleared, while maintaining its physical identity/doors and saves.

**IG-5: Wet urban environment.** Bring genuine Beckenburg water/weather settings through native C++ GLES runtime to the *actual city* and harbor, not isolated previews.

**IG-6: Playable town.** Complete a functional foot-route among buildings and the dock; integrate interaction, audio, walking, navigation, save and safe home exit before registering Sitka as playable in hub.

**IG-7: Coverage expansion.** Enlarge the city by authentic source/authoring polygons, with measured P0/P1/P2 building counts, accessible route length and validated shoreline travel. Expand custom landmark catalog iteratively.

### Definition of done for first city slice

- At least one real georeferenced winding public street segment and its connected paths;
- all valid source building footprints in the declared slice occupy one grounded P0 or better mesh, no arbitrary rectangular substitutions;
- a custom P2 building demonstrably replaces a P0 at identical world coordinates and save ID;
- user can walk from a street to a real dock interface without phantom collision, water-through-land or inaccessible door;
- recorded provenance and geometry validation passes; the actual GLES scene renders both eyes;
- the build remains honest about source gaps and does not claim the entire city is finished.

**Core invariant:** *Geography dictates form; P0 placeholders provide occupancy; P1 provides varied architectural synthesis; P2 provides recognizable custom identity; P3 selectively supports real gameplay; shared Beckenburg/Malefic techniques provide tooling and performance without imposing their grids.*
