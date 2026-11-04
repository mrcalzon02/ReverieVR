# Sitka — Surveyed Seafloor, Bathymetric World and Boat Depth Finder

**Game:** Sitka, inside the single main ReverieVR Android APK.
**Decision (2026-11-04):** Model land and underwater terrain from real, provenance-checked datasets, using the existing deterministic native simulation and GLES2 terrain engine. The seabed and boat depth finder share one geographically anchored world. No fabricated seabed, arbitrary map rectangles, online dependencies or invented soundings may masquerade as sourced Sitka.

**Current state:** CPU-only bathymetry raster exporter, strict native tile parser/sampler, boat depth and skiff movement rules, native prototype asset-loader integration and synthetic-data regression fixtures. This is **source-only**: Sitka remains unregistered, without approved geographic data, a playable boat, a visible depth gauge, underwater GL mesh or physical Galaxy S9 evidence.

## 1. Data authority for a multi-mile world

- **Land:** USGS Alaska IfSAR / 3DEP bare-earth 5 m DEM for broad coverage, with the higher-resolution Alaska DGGS/NOAA 2018 Sitka DTM where its real license and tile footprint overlap the selected world cells. Do not use the vegetation/building DSM as the bare-earth terrain.
- **First-choice seafloor:** NOAA NCEI National Ocean Service hydrographic survey grids (validated BAG and multibeam products) for surveyed harbors and coastal channels.
- **Fallback seafloor:** NOAA NCEI coastal relief/CUDEM products, tagged COMPOSITE with source resolution and uncertainty. Sparse track soundings or broad composite grids cannot become fabricated precise shallows.
- **City:** approved OpenStreetMap/municipal vectors for irregular roads, authentic footprint polygons, grade, trails and public waterfront, with attribution/license reviews.

References:
- NOAA bathymetric coverage/download: https://www.ncei.noaa.gov/maps/bathymetry/
- NOS survey archive: https://www.ncei.noaa.gov/products/nos-hydrographic-survey
- USGS Alaska IfSAR: https://www.usgs.gov/ngp-user-engagement-office/alaska-mapping-initiative
- Sitka DGGS lidar: https://dggs.alaska.gov/pubs/id/30531
- NOAA vertical-datum conversion: https://vdatum.noaa.gov/

Each approved source records provider, dataset/survey ID, download/acquisition date, version, licensed redistribution rights, source and output SHA-256, horizontal/vertical CRS/datum, tile coverage, sampling resolution, vertical-transform method/uncertainty and source-quality classification.

**Coordinates:** +X east, +Y up, +Z north. Original projected/metre anchors remain doubles; GPU positions use local, rebased floats. **Never mix MLLW, NAVD88 or other land/seabed/tide vertical datums silently.** The exporter requires the declared terrain and bathymetry datum to match exactly after independently validated preprocessing (VDatum where actually supported). There is no automatic datum conversion, Google data extraction, network access or fake asset fallback.

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

The water-surface level must come from an authored game tide model in the *same vertical datum*, **not a claimed real-time Sitka tide prediction**. The depth finder refreshes every 0.25 seconds on the simulation update, never during either-eye drawing. Missing coverage is clearly reported and never extrapolated.

The skiff model is fixed 30 Hz, gently accelerates with a 4 m/s target cap, responds to bounded throttle/rudder, blocks movement into off-water, unsafe-depth or NoData zones, and never forces head/camera roll or yaw. The vessel draft is a gameplay attribute, not a factual real-boat measurement.

**Next visible HUD — designed, not implemented:** Lato text inside the virtual console: DEPTH 8.4 m / KEEL 8.0 m / SAFE / SURVEY (or COMPOSITE), with NO DATA for gaps, and SIMULATED — NOT FOR NAVIGATION underneath. No actual phone sonar sensor is implied. The measurements are geographically sourced *game simulation*, never suitable for real-world marine navigation.

## 4. Exact development order and acceptance

1. Acquire approved downtown / harbor land DEM and NOAA bathymetric survey/composite files; audit actual coverage and vertical datum; hash source and conversion metadata.
2. Bake and validate a real harbor chunk, plus an aligned public shoreline, water footprint and small navigable boat launch area. Synthetic CI fixtures cannot be staged as geography.
3. Create underwater **visual mesh** only from covered seabed cells, with no triangles spanning NoData. Share indexed GLES2 VBOs across both eyes, decimate far tiles, and validate budget under Galaxy S9 sustained rendering.
4. Bind a real skiff model, launch/dock, input actions, collision, fixed-step water movement and the 4 Hz depth model in the actual native game, not as an inert launcher item.
5. Implement depth/clearance HUD using the licensed Lato glyph atlas, with clear missing/estimated labels. Verify water-surface and sea-floor datum, tide changes, and high/low-grounding cases.
6. Expand to continuous multi-mile coastal cells, ferry/boat corridors and inshore/offshore fishing. Stream adjacent cells with stable projected origins, seamless geographic boundaries, LOD and versioned save identities.
7. Author non-grid downtown roads and P0 polygon-accurate buildings, refine P1/P2 individual architecture and selected P3 public interiors, with no street displacement or pretend private access.
8. Package approved source assets with attribution into the single ReverieVR APK and verify offline launch, two-eye parity, saves, real hardware inputs, comfort and sustained 30 FPS on the Galaxy S9.

Native regression: deterministic C++ parse/sampling/draft/NoData/index/datum/tick/boat tests under GCC and Clang, plus Python GIS fixture for projected GeoTIFF export and source/license rejection. No code-stage pass alone means playable Sitka, actual bathymetric coverage, installed APK or device acceptance.
