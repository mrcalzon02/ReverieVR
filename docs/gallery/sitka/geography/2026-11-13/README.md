# Sitka — genuine tiled geography proof (November 13, 2026)

**These are actual numeric USGS 3DEP elevation samples, not synthesized terrain or an in-game screenshot.**

![Sitka USGS 3DEP elevation tile proof](https://raw.githubusercontent.com/mrcalzon02/ReverieVR/main/data/sitka/geography/generated/downtown-3dep-v1/actual-elevation-preview.png)

Original preview and [all sixteen tiled GeoTIFFs](https://github.com/mrcalzon02/ReverieVR/tree/main/data/sitka/geography/generated/downtown-3dep-v1) are retained with source and tile SHA-256 hashes. 4×4 tiles, each 512×512 m, 4 m sample grid, one shared row/column on every neighboring edge. This illustration is derived only from the verified USGS raster and labels all tile rows/columns.

The colored areas are interpolated elevation values — **not** confirmed shoreline, NOAA bathymetry, roads, buildings, playable terrain or navigation data. No coastal boundary was invented. Reference and blockers: [Sitka GIS tiling and acquisition report](../../../../project/SITKA_GIS_TILING.md).

## Real NOAA/USGS native stereo render

![Actual source-backed Sitka native GLES2 two-eye coastline inspection](sitka-noaa-usgs-gles2-stereo.png)

**Rendering proof:** Genuine C++ native Sitka renderer on Mesa EGL/GLES2, two different headset eyes. Real source-backed NOAA ENC chart-land geometry and USGS 3DEP elevations. It is a geometry-inspection mode inside the existing ReverieVR native game, not a separate APK, true camera footage, or verified marine sounding/2023 shoreline survey. Vertical datum unresolved; NOT FOR NAVIGATION. The original fishing/boat test range is preserved.
