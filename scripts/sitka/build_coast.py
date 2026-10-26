#!/usr/bin/env python3
"""Bake *licensed, supplied* metre-CRS DEM+coast GeoJSON into Sitka static triangle mesh.

No network access, Google Earth inputs, or synthetic geography fallback. Requires
rasterio, pyproj, numpy and shapely on the desktop authoring machine, not Android.
"""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

MAGIC = b"SITKA01\0"
HEADER = struct.Struct("<8sIIdd")  # magic, vertex count, triangle count, origin east/north (double precision)
VERTEX = struct.Struct("<fffBBB")  # local X/elevation/local Z, simple baked RGB
MAX_SAMPLES = 512 * 512
MAX_TRIANGLES = 500_000


def _sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(262144), b""):
            h.update(chunk)
    return h.hexdigest()


def bake(dem, coastline, provenance, output, step=12):
    import numpy as np
    import rasterio
    from pyproj import CRS, Transformer
    from shapely.geometry import shape, Point
    from shapely.ops import transform, unary_union
    from shapely.prepared import prep

    dem, coastline, provenance, output = map(Path, (dem, coastline, provenance, output))
    metadata = json.loads(provenance.read_text(encoding="utf-8"))
    # Require a source ledger for both feeds. Explicitly reject Google-derived work.
    for key in ("elevation", "shoreline"):
        record = metadata.get(key, {})
        if not all(isinstance(record.get(k), str) and record[k].strip()
                   for k in ("provider", "dataset", "license", "retrieved", "url")):
            raise ValueError(f"missing {key} provenance fields")
        if "google" in (record["provider"] + " " + record["dataset"]).lower():
            raise ValueError("Google Earth/Maps data cannot be rebaked into game meshes")
    if step < 1 or step > 128:
        raise ValueError("step must be 1..128")
    raw = json.loads(coastline.read_text(encoding="utf-8"))
    if raw.get("type") != "FeatureCollection" or not raw.get("features"):
        raise ValueError("expected GeoJSON FeatureCollection of land polygons")
    polygons = [shape(f["geometry"]) for f in raw["features"]]
    if any(g.is_empty or g.geom_type not in ("Polygon", "MultiPolygon") or not g.is_valid
           for g in polygons):
        raise ValueError("invalid land polygons")
    with rasterio.open(dem) as ds:
        if ds.count != 1 or not ds.crs or ds.nodata is None:
            raise ValueError("DEM must have one band, valid CRS and explicit NoData")
        if not ds.crs.is_projected or CRS.from_user_input(ds.crs).axis_info[0].unit_name.lower() not in ("metre", "meter"):
            raise ValueError("DEM must use a projected metre CRS; reproject with GDAL first")
        if ds.width < 2 or ds.height < 2 or ds.width * ds.height > 10_000_000:
            raise ValueError("DEM too small")
        if abs(ds.transform.b) > 1e-9 or abs(ds.transform.d) > 1e-9:
            raise ValueError("rotate/shear DEM to north-up grid before export")
        if ds.transform.a <= 0 or ds.transform.e >= 0:
            raise ValueError("DEM must have east-positive, north-up affine transform")
        geo_crs = CRS.from_user_input(raw.get("crs", {}).get("properties", {}).get("name", "EPSG:4326"))
        to_dem = Transformer.from_crs(geo_crs, ds.crs, always_xy=True)
        land = prep(unary_union([transform(to_dem.transform, p) for p in polygons]))
        rows = list(range(0, ds.height, step))
        cols = list(range(0, ds.width, step))
        if rows[-1] != ds.height - 1:
            rows.append(ds.height - 1)
        if cols[-1] != ds.width - 1:
            cols.append(ds.width - 1)
        if len(rows) * len(cols) > MAX_SAMPLES:
            raise ValueError("too many baked vertices; increase --step or crop DEM")
        elevation = ds.read(1, masked=True)
        origin_x, origin_z = ds.xy(rows[0], cols[0])
        vertices, mask = [], []
        for row in rows:
            for col in cols:
                value = elevation[row, col]
                east, north = ds.xy(row, col)
                valid = not np.ma.is_masked(value) and math.isfinite(float(value))
                on_land = land.covers(Point(east, north))
                usable = valid and on_land
                # Sea is *not* invented from NoData; triangles are emitted only where
                # all 3 vertices are valid surveyed land, leaving water for water modules.
                mask.append(usable)
                y = float(value) if usable else 0.0
                if usable:
                    rgb = (64, 115, 77) if y < 65 else (91, 112, 97) if y < 280 else (124, 132, 134)
                else:
                    rgb = (0, 0, 0)
                vertices.append((float(east-origin_x), y, float(north-origin_z), *rgb))
        indices = []
        w = len(cols)
        for y in range(len(rows) - 1):
            for x in range(w - 1):
                a = y*w + x
                for tri in ((a,a+w,a+1),(a+1,a+w,a+w+1)):
                    if all(mask[k] for k in tri):
                        indices.extend(tri)
        if not indices:
            raise ValueError("DEM/coastline have no overlapping surveyed land: check CRS, tiles and origin")
        if len(indices)//3 > MAX_TRIANGLES:
            raise ValueError("too many triangles")
        output.parent.mkdir(parents=True, exist_ok=True)
        mesh = output.with_suffix(".sitkam")
        with mesh.open("wb") as f:
            f.write(HEADER.pack(MAGIC, len(vertices), len(indices)//3, origin_x, origin_z))
            for v in vertices:
                f.write(VERTEX.pack(*v))
            for offset in range(0, len(indices), 8192):
                chunk = indices[offset:offset+8192]
                f.write(struct.pack("<"+"I"*len(chunk), *chunk))
        manifest = {
            "format": "sitka-static-terrain-1", "mesh": mesh.name, "mesh_sha256": _sha(mesh),
            "elevation_sha256": _sha(dem), "shoreline_sha256": _sha(coastline),
            "provenance_sha256": _sha(provenance), "coordinate_system": str(ds.crs),
            "vertical_units": "metres, as supplied by DEM; datum not converted",
            "east_origin": origin_x, "north_origin": origin_z,
            "x_axis": "east", "y_axis": "up", "z_axis": "north",
            "vertex_count": len(vertices), "triangle_count": len(indices)//3,
            "step": step, "surveyed_land_only": True, "source": metadata,
        }
        output.write_text(json.dumps(manifest, indent=2, sort_keys=True)+"\n", encoding="utf-8")
        return manifest


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dem", required=True, type=Path)
    p.add_argument("--coast", required=True, type=Path, help="GeoJSON land polygons, NOT Google content")
    p.add_argument("--provenance", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path, help="manifest JSON, also writes .sitkam")
    p.add_argument("--step", type=int, default=12, help="DEM samples per mesh vertex")
    a = p.parse_args()
    m = bake(a.dem, a.coast, a.provenance, a.output, a.step)
    print(f"Sitka terrain: {m['vertex_count']} vertices / {m['triangle_count']} triangles -> {a.output}")

if __name__ == "__main__":
    main()
