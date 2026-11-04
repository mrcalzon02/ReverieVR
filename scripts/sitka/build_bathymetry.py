#!/usr/bin/env python3
"""Bake approved, local projected-metre bathymetry into deterministic Sitka tiles.

No network access, Google extraction, invented seabed, datum transformation or
synthetic fallback. The vertical datums must already be identical; do any
supported transform upstream and document it in the source ledger first.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

HEADER = struct.Struct("<8sIIddddII")
PIXEL = struct.Struct("<fB")
MAGIC = b"SITKAB01"
MAX_TILE_INTERVALS = 512
MAX_TILES = 2048
REQUIRED = ("provider", "dataset", "license", "retrieved", "url",
            "vertical_datum", "terrain_vertical_datum", "encoding", "quality")

def sha256(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for data in iter(lambda: stream.read(262144), b""):
            h.update(data)
    return h.hexdigest()

def datum_id(value):
    if not isinstance(value, str) or not 1 <= len(value) <= 96 or any(
            not 32 < ord(ch) <= 126 for ch in value):
        raise ValueError("invalid vertical datum identifier")
    h = 2166136261
    for ch in value.encode("ascii"):
        h = ((h ^ ch) * 16777619) & 0xffffffff
    if h == 0:
        raise ValueError("invalid zero datum hash")
    return h

def check_provenance(meta):
    if not isinstance(meta, dict) or any(not isinstance(meta.get(k), str)
                                         or not meta[k].strip() for k in REQUIRED):
        raise ValueError("missing bathymetry provenance/datum")
    if "google" in (meta["provider"] + meta["dataset"]).lower():
        raise ValueError("Google Earth/Maps source is not redistributable bathymetry")
    if meta["vertical_datum"] != meta["terrain_vertical_datum"]:
        raise ValueError("bathymetry vs terrain vertical datum mismatch: transform before baking")
    if meta["encoding"] not in ("elevation_up", "positive_down_depth"):
        raise ValueError("bathymetry encoding must be explicitly declared")
    if meta["quality"] not in ("surveyed", "composite"):
        raise ValueError("source quality must be surveyed or composite")
    return datum_id(meta["vertical_datum"])

def bake(raster, provenance, output_dir, intervals=256):
    import numpy as np
    import rasterio
    from pyproj import CRS
    raster = Path(raster)
    provenance = Path(provenance)
    output_dir = Path(output_dir)
    source = json.loads(provenance.read_text(encoding="utf-8"))
    datum = check_provenance(source)
    if not 2 <= intervals <= MAX_TILE_INTERVALS:
        raise ValueError("tile intervals must be in 2..512")
    tiles = []
    # Validate before writing any derived artifact.
    with rasterio.open(raster) as ds:
        if ds.count != 1 or ds.nodata is None or ds.crs is None or (
            not ds.crs.is_projected):
            raise ValueError("require one-band projected-metre raster with explicit NoData")
        crs = CRS.from_user_input(ds.crs)
        if not crs.axis_info or crs.axis_info[0].unit_name.lower() not in (
                "metre", "meter"):
            raise ValueError("horizontal raster CRS must use metres")
        t = ds.transform
        if abs(t.b) > 1e-9 or abs(t.d) > 1e-9 or t.a <= 0 or t.e >= 0:
            raise ValueError("rotate/shear/reproject raster north-up before baking")
        if t.a > 500 or -t.e > 500 or ds.width < 2 or ds.height < 2:
            raise ValueError("unsupported cell spacing or raster dimensions")
        if ds.width > 10000 or ds.height > 10000:
            raise ValueError("source raster too large; crop into regional inputs")
        nx = (ds.width - 2) // intervals + 1
        ny = (ds.height - 2) // intervals + 1
        if nx * ny > MAX_TILES or nx > 100 or ny > 100:
            raise ValueError("raster exceeds bounded streaming directory; crop region")
        pixel_quality = 1 if source["quality"] == "surveyed" else 2
        # A missing NOAA coverage pixel is never filled by interpolation.
        band = ds.read(1, masked=True)
        candidate = []
        for tr in range(ny):
            for tc in range(nx):
                r0, c0 = tr * intervals, tc * intervals
                r1 = min(r0 + intervals + 1, ds.height)
                c1 = min(c0 + intervals + 1, ds.width)
                subset = band[r0:r1, c0:c1]
                rows, cols = subset.shape
                if rows < 2 or cols < 2:
                    raise ValueError("tile too small")
                e, n = ds.xy(r0, c0)
                if not math.isfinite(e) or not math.isfinite(n):
                    raise ValueError("bad bathymetric grid origin")
                raw = bytearray()
                known = 0
                for row in range(rows):
                    for col in range(cols):
                        value = subset[row, col]
                        valid = (not np.ma.is_masked(value) and
                                 math.isfinite(float(value)))
                        z = float(value) if valid else 0.
                        if valid and source["encoding"] == "positive_down_depth":
                            if z < 0:
                                raise ValueError("negative depth in positive-down raster")
                            z = -z
                        if valid and not -12000 <= z <= 100:
                            raise ValueError("bottom elevation out of bounds")
                        raw.extend(PIXEL.pack(z if valid else 0.,
                                              pixel_quality if valid else 0))
                        known += int(valid)
                if known == 0:
                    # Keep a tile with no coverage out of the runtime atlas.
                    continue
                name = f"bathymetry-r{tr:02d}-c{tc:02d}.sitkab"
                blob = HEADER.pack(MAGIC, cols, rows, e, n,
                                   t.a, -t.e, datum, 0) + raw
                candidate.append((name, bytes(blob), {
                    "file": name, "first_east_m": e, "first_north_m": n,
                    "width": cols, "height": rows,
                    "east_step_m": t.a, "south_step_m": -t.e,
                    "valid_samples": known,
                    "quality": source["quality"],
                }))
        if not candidate:
            raise ValueError("raster has no valid bathymetric cells")
        output_dir.mkdir(parents=True, exist_ok=True)
        for name, payload, desc in candidate:
            target = output_dir / name
            target.write_bytes(payload)
            desc["sha256"] = sha256(target)
            tiles.append(desc)
        manifest = {
            "format": "sitka-bathymetry-1",
            "purpose": "NON_NAVIGATIONAL_GAME_SIMULATION",
            "horizontal_crs": str(ds.crs),
            "vertical_datum": source["vertical_datum"],
            "datum_id": datum,
            "units": "seabed elevation metres, positive up",
            "raster_sha256": sha256(raster),
            "provenance_sha256": sha256(provenance),
            "tile_intervals": intervals,
            "tile_count": len(tiles),
            "index_schema": 2, "resident_tile_target": 2,
            "tiles": tiles,
            "source": source,
        }
        tile_lines = []
        for item in tiles:
            values = (item["first_east_m"], item["first_north_m"],
                      item["east_step_m"], item["south_step_m"])
            fields = " ".join(format(value, ".17g") for value in values)
            tile_lines.append(
                f'{item["file"]} {fields} {item["width"]} {item["height"]}\n'
            )
        tile_index = ("SITKA-BATHY-INDEX-2\n"
                      + "datum " + source["vertical_datum"] + "\n"
                      + "tiles " + str(len(tiles)) + "\n"
                      + "".join(tile_lines))
        (output_dir / "index.txt").write_text(tile_index, encoding="ascii")
        (output_dir / "manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return manifest

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--bathymetry", type=Path, required=True,
                   help="approved north-up projected-metre GeoTIFF")
    p.add_argument("--provenance", type=Path, required=True,
                   help="licensed source ledger and reconciled vertical datum")
    p.add_argument("--output-dir", type=Path, required=True)
    p.add_argument("--tile-intervals", type=int, default=256)
    args = p.parse_args()
    result = bake(args.bathymetry, args.provenance,
                  args.output_dir, args.tile_intervals)
    print(f"Sitka seabed: {result['tile_count']} tiles, "
          f"datum {result['vertical_datum']} -> {args.output_dir}")
if __name__ == "__main__":
    main()
