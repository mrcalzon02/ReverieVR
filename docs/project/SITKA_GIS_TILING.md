# Sitka real-world geographic tiling

Status (2026-11-13): **real elevation acquisition pipeline implemented; the
native tiled coastline is NOT installed yet.** This is inside ReverieVR.
The fallback non-geographic test range remains separate.

## Source (no invented geography)

The acquisition script requests a small numeric F32 **USGS 3DEP bare-earth DEM**
from the official National Map 3DEPElevation ImageServer:

https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer

Public-domain product details:
https://www.usgs.gov/3d-elevation-program/about-3dep-products-services

There is also a high-resolution 2018 Alaska DGGS / NOAA Sitka lidar DEM,
dataset #9612, distributed as approximately 476 MB of GeoTIFF data:

https://noaa-nos-coastal-lidar-pds.s3.amazonaws.com/dem/AK_Sitka_DEM_2018_9612/index.html

The first bounded USGS service export does **not** claim every sample is from
the 2018 Sitka source. The service is a dynamic multi-source DEM. Its vertical
datum has not yet been independently resolved. The 2018 NOAA original is
listed as a separate source candidate, NOT secretly merged into 3DEP.

## Coordinate grid and tile geometry

Authority: data/sitka/geography/sitka-downtown-grid.json

- WGS84 UTM Zone 8 north, EPSG:32608 projected horizontal coordinate system.
- West grid node: E 478720 m. North grid node: N 6324480 m.
- East end: E 480768 m. South end: N 6322432 m.
- 4 columns by 4 rows, each tile 512 m by 512 m.
- Source node spacing: 4 m. Whole DEM: 513 x 513 samples.
- Each tile: 129 x 129 samples, with adjacent tiles sharing the entire
  boundary sample line; tile meshes do not introduce gaps or offsets.
- Coverage: approximately 2.048 km by 2.048 km in downtown Sitka and harbor
  approaches, including the approximate O'Connell Bridge research locator.
  It is not the full Sitka city, town borough or marine navigation region.

## Pipeline

1. scripts/sitka/acquire_3dep_geography.py requests only the bounded USGS
   ImageServer numeric GeoTIFF. It refuses HTML, corrupt TIFF, wrong projection,
   rotated/misaligned grids, insufficient real data and wrong sample dimensions.
   It preserves the raw download hash and publishes a normalized, georeferenced
   Float32 TIFF with an explicit NoData value. Unknown pixels remain unknown.
2. scripts/sitka/build_geography_tiles.py checks the source receipt and every
   SHA256, splits into 16 compressed GeoTIFF source tiles, proves all horizontal
   and vertical shared-edge samples match, and publishes tile index + coverage
   metadata and geographic transform/CRS for each tile.
3. scripts/sitka/render_geography_preview.py creates a diagnostic elevation
   map strictly from the real height samples. The image is labeled source-only
   and must not be mistaken for the coast, roads, buildings or real sonar.
4. The single GitHub Action .github/workflows/sitka-geography-acquisition.yml
   runs the local coast/tile regression tests, downloads public USGS data,
   uploads an artifact and attempts a non-force source commit with remote
   readback. If main advances during the job, artifact retention wins; no
   overwrite or conflicting source commit is attempted.

Commands:

    python scripts/sitka/acquire_3dep_geography.py \
      --grid data/sitka/geography/sitka-downtown-grid.json \
      --output work/sitka/usgs3dep-downtown-dem.tif

    python scripts/sitka/build_geography_tiles.py \
      --dem work/sitka/usgs3dep-downtown-dem.tif \
      --receipt work/sitka/usgs3dep-downtown-dem.source.json \
      --grid data/sitka/geography/sitka-downtown-grid.json \
      --output-dir work/sitka/tiled-geography

    python -m unittest discover -s scripts/sitka \
      -p 'test_geography_tiles.py' -v

## Conversion to game-native land tiles

The original scripts/sitka/build_coast.py is reused unchanged. When a
verified polygonal land/coast GeoJSON becomes available, tile builder options
--coast and --coast-provenance enable one terrain-rNN-cNN.sitkam per valid
land tile. Each tile has the native SITKA01 header, local high-precision
projected origin and existing GLES2 vertex/triangle budget.

The coastline source ledger must contain provider, dataset, license, retrieved
date, URL and explicit, reviewed booleans:

    approved_for_redistribution: true
    current_geography_reviewed: true

With no approved coastline geometry, the exporter deliberately produces
**real elevation raster tiles but zero coastline meshes**. Water must never
be fabricated as dry terrain. No missing or invalid shoreline can be inferred
from historical building footprint candidates.

The runtime currently expects one sitka/coast/terrain.sitkam, while marine
depth streaming uses separate SITKAB01 tiles. A real land tile index/streamer,
the shoreline-vector import, municipal road/pier feature review, and complete
gameplay anchors remain open engineering tasks before this becomes a seamless,
playable coastal world. The existence of the real height tile alone is not
proof of runtime map integration.

## Vertical datums, seabed, historical/currentness

Land elevation and marine seafloor must not be naively spliced at zero. NOAA
soundings, tidal heights and USGS terrestrial height surfaces may use
different vertical datums such as MLLW and NAVD88. This pipeline retains
UNVERIFIED_DYNAMIC_3DEP_MOSAIC rather than inventing a datum. The existing
build_bathymetry.py can consume approved depth data independently; only
reconcile after a documented supported datum transformation.

2017 building footprint research is real historical geographic evidence
but **not** a current-world coastline or safe proof that buildings remain
standing in 2026. Current municipal GIS and independently cleared OSM land
polygon inputs will be needed for shore topology and public access.

## Acceptance and safeguards

- One source response + hash, one normalized F32 projected metre GeoTIFF +
  hash, 16 indexed georeferenced 129-square tile rasters + hashes.
- North/south and east/west shared sample values must match exactly.
- No missing samples are filled, no fake coast, roads, shore fishing access,
  marina launch or surveyed bathymetry.
- Unit-test fixtures are synthetic **test-only** and are never copied into
  game assets.
- CI source artifact and real-data commit are verifiable independently.
- The generated-data directory is source research, not current APK assets.
- There is no separate Sitka APK, no save schema migration, no new game-count
  release blocker and no visual-style release gate.
