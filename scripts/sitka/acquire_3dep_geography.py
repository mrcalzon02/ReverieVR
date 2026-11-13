#!/usr/bin/env python3
"""Download one bounded, REAL USGS 3DEP elevation grid for Sitka.

This is an OFFLINE-AUTHORED SOURCE ASSET for the ReverieVR game. It is not
bathymetry, not a surveyed coastline, not a current building map, and not a
runtime web request. No synthetic elevations may be used as production data.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import tempfile
import time
from urllib import request

SERVICE = ("https://elevation.nationalmap.gov/arcgis/rest/services/"
           "3DEPElevation/ImageServer/exportImage")
MAX_RESPONSE_BYTES = 12_000_000
NODATA = -9999.0


def sha256(path):
    hash_ = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for buf in iter(lambda: stream.read(262144), b""):
            hash_.update(buf)
    return hash_.hexdigest()


def load_grid(path):
    cfg = json.loads(Path(path).read_text(encoding="utf-8"))
    g = cfg.get("grid")
    if cfg.get("schema") != 1 or not isinstance(g, dict):
        raise ValueError("unsupported authoritative Sitka geographic grid")
    if cfg.get("source_vertical_datum") != "UNVERIFIED_DYNAMIC_3DEP_MOSAIC":
        raise ValueError("dynamic 3DEP vertical datum must not be assumed")
    if cfg.get("horizontal_crs") != "EPSG:32608" or g.get("epsg") != 32608:
        raise ValueError("Sitka grid must be UTM zone 8 metres EPSG:32608")
    for key in ("west_grid_node_m", "north_grid_node_m",
                "columns", "rows", "tile_intervals", "sample_spacing_m"):
        value = g.get(key)
        if type(value) not in (int, float) or not math.isfinite(value):
            raise ValueError("invalid geospatial grid parameter: " + key)
    west = g["west_grid_node_m"]
    north = g["north_grid_node_m"]
    cols, rows = g["columns"], g["rows"]
    intervals = g["tile_intervals"]
    spacing = g["sample_spacing_m"]
    if not (type(cols) is int and type(rows) is int and
            type(intervals) is int and 1 <= cols <= 16 and
            1 <= rows <= 16 and intervals in (64, 128, 256) and
            spacing in (2, 4, 5, 8, 10)):
        raise ValueError("unsupported tile count or interval / spacing")
    if not (470000 <= west <= 490000 and 6_310_000 <= north <= 6_340_000):
        raise ValueError("grid is not within approved Sitka local metric extent")
    if cols * intervals + 1 > 4096 or rows * intervals + 1 > 4096:
        raise ValueError("requested raster is too large for one bounded export")
    return cfg


def geometry(grid):
    g = grid["grid"]
    interval_m = g["tile_intervals"] * g["sample_spacing_m"]
    west = float(g["west_grid_node_m"])
    north = float(g["north_grid_node_m"])
    east = west + g["columns"] * interval_m
    south = north - g["rows"] * interval_m
    step = float(g["sample_spacing_m"])
    # ImageServer accepts pixel OUTER edges, whereas the native terrain
    # vertices are pixel CENTERS. Half-pixel padding preserves shared nodes.
    bbox = (west - step / 2., south - step / 2.,
            east + step / 2., north + step / 2.)
    return bbox, (g["columns"] * g["tile_intervals"] + 1,
                  g["rows"] * g["tile_intervals"] + 1)


def image_url(grid):
    from urllib.parse import urlencode
    bbox, size = geometry(grid)
    values = {
        "bbox": ",".join(format(x, ".6f") for x in bbox),
        "bboxSR": "32608", "imageSR": "32608",
        "size": f"{size[0]},{size[1]}",
        "format": "tiff", "pixelType": "F32",
        "interpolation": "RSP_BilinearInterpolation",
        "f": "image",
    }
    return SERVICE + "?" + urlencode(values)


def download_bytes(url):
    # Limited independent attempts: timeouts/5xx/network interruptions may
    # retry, but a forged service body can never become a geographic asset.
    last = None
    for attempt in range(3):
        try:
            req = request.Request(
                url, headers={"User-Agent": "ReverieVR-Sitka-3DEP/1.0",
                              "Accept": "image/tiff,application/octet-stream"})
            with request.urlopen(req, timeout=90) as result:
                data = result.read(MAX_RESPONSE_BYTES + 1)
            if len(data) > MAX_RESPONSE_BYTES:
                raise ValueError("USGS DEM response exceeds bounded limit")
            if not (data.startswith(b"II*\x00") or
                    data.startswith(b"MM\x00*")):
                raise ValueError("USGS service did not return a GeoTIFF")
            return data
        except (OSError, TimeoutError) as exc:
            last = exc
            if attempt != 2:
                time.sleep(2 ** attempt)
    raise RuntimeError("USGS 3DEP elevation retrieval failed") from last


def normalize_geotiff(payload, grid, target):
    """Verify a real GeoTIFF and preserve ONLY known float elevations."""
    import numpy as np
    import rasterio
    from rasterio.io import MemoryFile
    from rasterio.crs import CRS
    target = Path(target)
    bbox, (expected_w, expected_h) = geometry(grid)
    g = grid["grid"]
    step = float(g["sample_spacing_m"])
    with MemoryFile(payload) as memory:
        with memory.open() as src:
            if (src.count != 1 or src.dtypes[0] != "float32" or
                    src.crs != CRS.from_epsg(32608) or
                    (src.width, src.height) != (expected_w, expected_h)):
                raise ValueError("3DEP export is not one F32 UTM8 height grid")
            t = src.transform
            if (abs(t.b) > 1e-8 or abs(t.d) > 1e-8 or
                    abs(t.a - step) > 1e-5 or abs(t.e + step) > 1e-5 or
                    abs((t.c + t.a / 2.) - g["west_grid_node_m"]) > .01 or
                    abs((t.f + t.e / 2.) - g["north_grid_node_m"]) > .01):
                raise ValueError("3DEP raster is not aligned to the tile grid")
            z = src.read(1)
            mask = src.read_masks(1) > 0
            valid = mask & np.isfinite(z) & (z >= -150.) & (z <= 6000.)
            invalid_finite = mask & np.isfinite(z) & ((z < -150.) | (z > 6000.))
            if np.any(invalid_finite):
                raise ValueError("3DEP source contains out-of-range ground heights")
            count = int(np.count_nonzero(valid))
            if count < .05 * z.size:
                raise ValueError("3DEP has insufficient actual elevation coverage")
            heights = z[valid]
            if float(heights.max() - heights.min()) < .1:
                raise ValueError("3DEP returned an implausibly constant height field")
            normalized = np.where(valid, z, np.float32(NODATA)).astype("float32")
            profile = src.profile.copy()
            profile.update(driver="GTiff", count=1, dtype="float32",
                           crs="EPSG:32608", nodata=NODATA,
                           compress="deflate", predictor=3,
                           tiled=True, blockxsize=256, blockysize=256)
            target.parent.mkdir(parents=True, exist_ok=True)
            with rasterio.open(target, "w", **profile) as dst:
                dst.write(normalized, 1)
            return {
                "sample_count": int(z.size), "valid_samples": count,
                "unknown_samples": int(z.size - count),
                "valid_fraction": round(count / z.size, 7),
                "min_elevation_m": round(float(heights.min()), 3),
                "max_elevation_m": round(float(heights.max()), 3),
                "source_nodata": None if src.nodata is None else float(src.nodata),
                "saved_nodata": NODATA,
                "raster_crs": src.crs.to_string(),
                "grid_node_west_m": g["west_grid_node_m"],
                "grid_node_north_m": g["north_grid_node_m"],
                "pixel_spacing_m": step,
                "raster_width": src.width, "raster_height": src.height,
            }


def acquire(grid_path, output):
    cfg = load_grid(grid_path)
    output = Path(output)
    receipt = output.with_suffix(".source.json")
    if output.exists() or receipt.exists():
        raise FileExistsError("source DEM already exists: refuse overwriting")
    url = image_url(cfg)
    payload = download_bytes(url)
    with tempfile.TemporaryDirectory(prefix="sitka-3dep-", dir=output.parent) as tmp:
        stage = Path(tmp) / output.name
        stats = normalize_geotiff(payload, cfg, stage)
        fetched = datetime.now(timezone.utc).isoformat(timespec="seconds")
        evidence = {
            "schema": 1, "format": "sitka-usgs-3dep-source-receipt-1",
            "source_kind": "AUTHENTIC_USGS_3DEP_INTERPOLATED_ELEVATION",
            "provider": "USGS National Map", "dataset": cfg["source_dataset"],
            "license": "US Government public domain",
            "url": SERVICE, "request_url": url,
            "retrieved": fetched[:10], "retrieved_at_utc": fetched,
            "source_response_sha256": hashlib.sha256(payload).hexdigest(),
            "normalized_dem": output.name,
            "normalized_dem_sha256": sha256(stage),
            "grid_config_sha256": sha256(grid_path),
            "horizontal_crs": "EPSG:32608",
            "vertical_datum": "UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
            "ready_for_real_seafloor_merging": False,
            "ready_for_runtime_coast": False,
            "units": "metres above published 3DEP vertical reference",
            "bounds_outer": list(geometry(cfg)[0]),
            "statistics": stats,
        }
        stage.replace(output)
        receipt.write_text(json.dumps(evidence, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8")
        return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--grid", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    receipt = acquire(args.grid, args.output)
    print("REAL USGS 3DEP Sitka DEM verified:", receipt["statistics"]["valid_samples"],
          "height samples; SHA256", receipt["normalized_dem_sha256"])


if __name__ == "__main__":
    main()
