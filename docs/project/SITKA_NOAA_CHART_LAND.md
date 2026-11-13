# Sitka source coastline: NOAA ENC chart land import

Goal: derive real-source cartographic land geometry for the 16 existing
512-metre source elevation tiles. This is an intermediate geographic
research/render artifact, NOT a second Sitka APK, final shoreline survey,
navigational product, or production game land streamer.

## Sources and honest quality labels

* Land geometry from NOAA's public ENC Harbour "Land_Area" feature layer 233:
  https://encdirect.noaa.gov/arcgis/rest/services/encdirect/enc_harbour/MapServer/233
* Height: existing USGS 3DEP downtown Sitka source DEM, EPSG:32608, 4 m
  samples, stored at data/sitka/geography/generated/downtown-3dep-v1.
* The independent NOAA 2023 Port of Sitka shoreline survey AK2309-CS-N
  https://www.fisheries.noaa.gov/inport/item/72222 is NOT the polygon source
  in this pass. Do not attribute its stated 0.8 m accuracy to these chart
  polygons.

The official NOAA ENC service serves public, geographically attributed
chart features, updated periodically. Some cartographic geometries may be
more generalized or originally compiled at older survey dates. The only
valid currentness claim here is that these are genuine geometry features
from NOAA's published ENC service as retrieved, NOT independently ground
verified shoreline of 2026. Any coordinate made by clipping to our
rectangular study AOI is explicitly an artificial study-area border,
not a surveyed coastal edge.

## Data safety and geometry method

scripts/sitka/acquire_noaa_enc_land.py binds to the one official NOAA URL.
It requests object IDs intersecting the geographic study area and retrieves
bounded batches of true NOAA polygons. It rejects an invalid/missing
geometry type, duplicate IDs, absent source attribution, malformed
coordinates, empty overlap, oversized responses and non-land features.

The source polygon is projected to UTM8, intersected with the measured grid
bounds, and reprojected to WGS84 GeoJSON for the existing build_coast.py
validator. No Google tracing, raster inferred water level, guessed bridge,
manual beach polygon or alpha-mask artwork is used. Source IDs, response
SHA256 and a complete ledger are stored alongside the clipped geometry.

The dedicated GitHub workflow sitka-noaa-land-tiles.yml runs the exact
NOAA importer and the previously tested build_geography_tiles.py exporter,
using the 16 already archived source DEM cells. Tiles only contain actual
landwhere NOAA's cartographic polygon exists, and each native terrain
SITKA01 file is indexed by its source hash/metric origin. For zero valid
polygons it must fail loudly; it cannot silently create a full rectangle
of fabricated coast.

A diagnostic PNG overlays red NOAA source land boundaries on the real
USGS numeric DEM; the image is a provenance/geometry review overlay,
not an actual rendered Sitka world. All generated files are retained in
GitHub workflow artifacts and, only after full tests and main-branch
readback, archived under:

data/sitka/geography/generated/noaa-enc-chart-land-v1/

## Remaining gameplay acceptance

Source-masked land meshes are not yet a live multi-tile ReverieVR world.
The native Sitka module still loads one sitka/coast/terrain.sitkam and
its distinct marine bathymetry streaming cache. Work still needed:
a bounded land index, runtime tile load and unload around the real player,
seam continuity from identical source vertices, source-backed water
footprints, usable verified walking/fishing/harbor spawns, NOAA marine datum
reconciliation and emulator/headset test captures.

The old explicitly NON-GEOGRAPHIC integration range stays available
until native land/marine geometry and playable activity placement are
actually validated. A successful native NDK build is not proof that
the genuine Sitka geographic game is complete.
