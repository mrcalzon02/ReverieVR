#!/usr/bin/env python3
"""Build finite, seam-verified Sitka land tiles from a real projected DEM.

Every terrain tile reuses build_coast.bake, the existing native SITKA01
exporter, ONLY if licensed polygonal coastline is provided. Without that
mask this produces genuine georeferenced source-height raster tiles but
NO playable terrain: water must never silently become artificial land.
Bathymetry requires an independent source and reconciled vertical datum.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import tempfile

from acquire_3dep_geography import load_grid, geometry, sha256, NODATA
from build_coast import bake as bake_coast


def check_receipt(grid_path, receipt_path, dem_path):
    grid = load_grid(grid_path)
    proof = json.loads(Path(receipt_path).read_text(encoding="utf-8"))
    if (proof.get("format") != "sitka-usgs-3dep-source-receipt-1"
        or proof.get("source_kind") != "AUTHENTIC_USGS_3DEP_INTERPOLATED_ELEVATION"
        or proof.get("grid_config_sha256") != sha256(grid_path)
        or proof.get("normalized_dem_sha256") != sha256(dem_path)
        or proof.get("horizontal_crs") != "EPSG:32608"
        or proof.get("vertical_datum") != "UNVERIFIED_DYNAMIC_3DEP_MOSAIC"
        or proof.get("ready_for_real_seafloor_merging") is not False):
        raise ValueError("unverified / mismatched USGS terrain acquisition")
    if not isinstance(proof.get("retrieved"), str) or not proof["retrieved"]:
        raise ValueError("source acquisition date absent")
    return grid, proof


def check_dem(src, grid):
    from rasterio.crs import CRS
    g = grid["grid"]
    step = float(g["sample_spacing_m"])
    width = g["columns"] * g["tile_intervals"] + 1
    height = g["rows"] * g["tile_intervals"] + 1
    t = src.transform
    if (src.count != 1 or src.crs != CRS.from_epsg(32608)
            or src.dtypes[0] != "float32" or src.nodata != NODATA
            or (src.width, src.height) != (width, height)
            or abs(t.a-step) > 1e-6 or abs(t.e+step) > 1e-6
            or abs(t.b) > 1e-9 or abs(t.d) > 1e-9
            or abs(t.c + t.a/2. - g["west_grid_node_m"]) > .001
            or abs(t.f + t.e/2. - g["north_grid_node_m"]) > .001):
        raise ValueError("source elevation does not align with geographic tiles")


def approved_shoreline(path):
    """No guessed land polygons, Google map tracing or unlabeled OSM data."""
    info = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(info, dict):
        raise ValueError("missing shoreline source ledger")
    keys = ("provider", "dataset", "license", "retrieved", "url")
    if any(not isinstance(info.get(k), str) or not info[k].strip() for k in keys):
        raise ValueError("shoreline source/permission ledger incomplete")
    if "google" in (info["provider"] + " " + info["dataset"]).lower():
        raise ValueError("Google mapping data is not approved coastline geometry")
    if info.get("approved_for_redistribution") is not True:
        raise ValueError("shoreline redistribution rights have not been reviewed")
    if info.get("current_geography_reviewed") is not True:
        raise ValueError("shoreline geometry not approved as current")
    return info


def compare_seams(folder, rows, cols, width):
    """Exact shared sample centers across adjacent tiled GeoTIFFs."""
    import numpy as np
    import rasterio
    folder = Path(folder)
    for row in range(rows):
        for col in range(cols):
            with rasterio.open(folder / f"r{row:02d}-c{col:02d}.tif") as current:
                samples = current.read(1)
                if samples.shape != (width, width):
                    raise ValueError("tile shape mismatch")
                if col + 1 < cols:
                    with rasterio.open(folder / f"r{row:02d}-c{col+1:02d}.tif") as next_col:
                        if not np.array_equal(samples[:, -1], next_col.read(1)[:, 0],
                                              equal_nan=True):
                            raise ValueError(f"east-west tile seam mismatch {row}/{col}")
                if row + 1 < rows:
                    with rasterio.open(folder / f"r{row+1:02d}-c{col:02d}.tif") as next_row:
                        if not np.array_equal(samples[-1], next_row.read(1)[0],
                                              equal_nan=True):
                            raise ValueError(f"north-south tile seam mismatch {row}/{col}")


def tile(dem_path, receipt_path, grid_path, output_dir,
         coast_geojson=None, coast_provenance=None):
    import numpy as np
    import rasterio
    from rasterio.windows import Window
    from rasterio.crs import CRS
    from rasterio.transform import Affine

    grid, source = check_receipt(grid_path, receipt_path, dem_path)
    g = grid["grid"]
    cols, rows, n = g["columns"], g["rows"], g["tile_intervals"]
    output_dir = Path(output_dir)
    if output_dir.exists():
        raise FileExistsError("refuse to replace or mix existing tiled geography")
    if bool(coast_geojson) != bool(coast_provenance):
        raise ValueError("shore geometry and its approved source ledger are a pair")
    coastline = None
    provenance = None
    if coast_geojson:
        coastline = Path(coast_geojson)
        shoreline = approved_shoreline(coast_provenance)
        if not coastline.is_file():
            raise ValueError("real coastline GeoJSON unavailable")
    root = output_dir.parent
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="sitka-tiles-", dir=root) as temporary:
        stage = Path(temporary) / output_dir.name
        elevation_dir = stage / "elevation"
        land_dir = stage / "land"
        elevation_dir.mkdir(parents=True)
        land_dir.mkdir(parents=True)
        if coastline:
            elevation = {key: source[key] for key in
                         ("provider", "dataset", "license", "retrieved", "url")}
            provenance = stage / "land-source-ledger.json"
            provenance.write_text(json.dumps(
                {"elevation": elevation, "shoreline": {
                    key: shoreline[key] for key in
                    ("provider", "dataset", "license", "retrieved", "url")
                }}, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        entries = []
        total_valid = 0
        total_meshes = 0
        for_rowcol = []
        with rasterio.open(dem_path) as src:
            check_dem(src, grid)
            for row in range(rows):
                for col in range(cols):
                    window = Window(col*n, row*n, n+1, n+1)
                    raw = src.read(1, window=window)
                    if raw.shape != (n+1, n+1):
                        raise ValueError("truncated elevation tile")
                    filename = f"r{row:02d}-c{col:02d}.tif"
                    tile_file = elevation_dir / filename
                    profile = src.profile.copy()
                    profile.update(width=n+1, height=n+1, transform=src.window_transform(window),
                                   compress="deflate", predictor=3,
                                   tiled=True, blockxsize=128, blockysize=128)
                    with rasterio.open(tile_file, "w", **profile) as dst:
                        dst.write(raw, 1)
                    valid = np.isfinite(raw) & (raw != NODATA)
                    count = int(np.count_nonzero(valid))
                    total_valid += count
                    x, z = src.xy(row*n, col*n)
                    one_tile = {
                        "row": row, "column": col, "elevation_file": "elevation/"+filename,
                        "elevation_sha256": sha256(tile_file),
                        "origin_east_m": x, "origin_north_m": z,
                        "pixel_spacing_m": g["sample_spacing_m"],
                        "size_samples": [n+1, n+1],
                        "valid_samples": count,
                        "unknown_samples": int(raw.size - count),
                        "source_kind": source["source_kind"],
                        "coastline_mask_status": "WITHHELD_NOT_PROVIDED",
                        "native_mesh_file": None,
                    }
                    if coastline:
                        target = land_dir / f"terrain-r{row:02d}-c{col:02d}.json"
                        try:
                            mesh = bake_coast(tile_file, coastline,
                                              provenance, target, step=1)
                        except ValueError as exc:
                            if "DEM/coastline have no overlapping surveyed land" not in str(exc):
                                raise
                            one_tile["coastline_mask_status"] = "NO_SOURCE_APPROVED_LAND_IN_TILE"
                        else:
                            name = target.with_suffix(".sitkam")
                            one_tile.update(
                                {"coastline_mask_status": "SOURCE_MASKED",
                                 "native_mesh_file": "land/"+name.name,
                                 "native_mesh_sha256": sha256(name),
                                 "terrain_triangles": mesh["triangle_count"],
                                 "terrain_vertices": mesh["vertex_count"]})
                            total_meshes += 1
                    entries.append(one_tile)
        if total_valid == 0:
            raise ValueError("no real sampled elevation within target tile grid")
        if coastline and total_meshes == 0:
            raise ValueError("approved coastline has no overlapping verified land")
        compare_seams(elevation_dir, rows, cols, n+1)
        index = {
            "schema": 1, "format": "sitka-land-tiles-1",
            "source": "USGS_NATIONAL_MAP_3DEP_DYNAMIC",
            "source_dem_sha256": sha256(dem_path),
            "source_receipt_sha256": sha256(receipt_path),
            "grid_definition_sha256": sha256(grid_path),
            "columns": cols, "rows": rows,
            "tile_intervals": n, "pixel_spacing_m": g["sample_spacing_m"],
            "tile_meters": n*g["sample_spacing_m"],
            "grid_north_m": g["north_grid_node_m"],
            "grid_west_m": g["west_grid_node_m"],
            "horizontal_crs": "EPSG:32608",
            "vertical_datum": source["vertical_datum"],
            "geography_currentness": "USGS_3DEP_DYNAMIC_SERVICE_SNAPSHOT",
            "sourced_elevation_only": coastline is None,
            "game_runtime_ready": False,
            "runtime_blockers": (
                ["approved polygonal coastline",
                 "approved aligned maritime water and vertical datum",
                 "game walking/fishing assets", "land tile streaming JNI integration"]
                if coastline is None
                else ["approved aligned maritime water and vertical datum",
                      "game walking/fishing assets", "land tile streaming JNI integration"]),
            "terrain_tiles_baked": total_meshes,
            "tile_count": len(entries),
            "tiles": entries,
        }
        (stage / "index.json").write_text(
            json.dumps(index, indent=2, sort_keys=True)+"\n", encoding="utf-8")
        (stage / "source-receipt.json").write_text(
            json.dumps(source, indent=2, sort_keys=True)+"\n", encoding="utf-8")
        stage.rename(output_dir)
        return index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dem", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--grid", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--coast", type=Path, default=None,
                        help="license-approved polygonal land GeoJSON")
    parser.add_argument("--coast-provenance", type=Path, default=None)
    a = parser.parse_args()
    result = tile(a.dem, a.receipt, a.grid, a.output_dir,
                  a.coast, a.coast_provenance)
    print("Sitka REAL geo-DEM tiled:", result["tile_count"],
          "edge-checked", result["tile_meters"], "m tiles; land meshes:",
          result["terrain_tiles_baked"], "runtime ready:", result["game_runtime_ready"])


if __name__ == "__main__":
    main()
