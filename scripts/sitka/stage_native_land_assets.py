#!/usr/bin/env python3
"""Stage only verified, source-masked NOAA+USGS Sitka land tiles for the APK.

No network, no synthetic elevation, no bathymetry, no fake harbor metadata.
The native viewer marks these chart-derived tiles NON-NAVIGATIONAL and cannot
unlock real Sitka fishing/boats until water/datum/activity inputs are approved.
"""
import argparse
import hashlib
import json
import re
import shutil
import struct
import tempfile
import zlib
from pathlib import Path

HEADER = struct.Struct("<8sIIdd")
MAGIC = b"SITKA01\0"
TILE_FILE = re.compile(r"land/terrain-r([0-9]{2})-c([0-9]{2})\.sitkam\Z")
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/sitka/geography/generated/noaa-enc-chart-land-v1"
INPUT = SOURCE / "land-tiles"
MAX_BYTES = HEADER.size + 16641 * 15 + 32768 * 12

def digest(data):
    return hashlib.sha256(data).hexdigest()

def write_assets(output, source=SOURCE):
    output = Path(output)
    source = Path(source)
    manifest_path = source / "land-tiles/index.json"
    land_receipt_path = source / "noaa-enc-harbour-land.source.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    receipt = json.loads(land_receipt_path.read_text(encoding="utf-8"))
    if (manifest.get("format") != "sitka-land-tiles-1" or
        manifest.get("source") != "USGS_NATIONAL_MAP_3DEP_DYNAMIC" or
        manifest.get("horizontal_crs") != "EPSG:32608" or
        manifest.get("vertical_datum") != "UNVERIFIED_DYNAMIC_3DEP_MOSAIC" or
        manifest.get("game_runtime_ready") is not False or
        manifest.get("sourced_elevation_only") is not False or
        manifest.get("terrain_tiles_baked") != 16 or
        manifest.get("tile_count") != 16 or
        manifest.get("columns") != 4 or manifest.get("rows") != 4 or
        manifest.get("tile_intervals") != 128 or
        manifest.get("pixel_spacing_m") != 4 or
        manifest.get("tile_meters") != 512 or
        manifest.get("grid_west_m") != 478720 or
        manifest.get("grid_north_m") != 6324480):
        raise ValueError("unsupported / unverified Sitka source terrain archive")
    if (receipt.get("geometry_source") != "AUTHENTIC_NOAA_ENC_CHART_LAND" or
        receipt.get("provider") != "NOAA Office of Coast Survey" or
        receipt.get("NOT_FOR_NAVIGATION") is not True or
        receipt.get("grid_config_sha256") != manifest.get("grid_definition_sha256") or
        receipt.get("approved_for_redistribution") is not True):
        raise ValueError("NOAA chart-land identity, source grid or rights mismatch")
    source_geom = (source / "noaa-enc-harbour-land.geojson").read_bytes()
    if digest(source_geom) != receipt.get("source_geojson_sha256"):
        raise ValueError("NOAA source polygon bytes changed since acquisition")
    tiles = manifest.get("tiles")
    if not isinstance(tiles, list) or len(tiles) != 16:
        raise ValueError("all sixteen source tile records are required")
    if output.exists():
        raise FileExistsError("refuse to replace previously staged native assets")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent, prefix="sitka-staging-") as root:
        stage = Path(root) / "build"
        folder = stage / "sitka/coast/tiles"
        folder.mkdir(parents=True)
        records = []
        seen = set()
        for entry in tiles:
            match = TILE_FILE.fullmatch(entry.get("native_mesh_file") or "")
            if not match:
                raise ValueError("missing or unsafe source-masked terrain path")
            row, col = map(int, match.groups())
            if (row, col) in seen or (row, col) != (entry.get("row"), entry.get("column")):
                raise ValueError("duplicate or mismatched tile row/column")
            seen.add((row,col))
            if entry.get("coastline_mask_status") != "SOURCE_MASKED":
                raise ValueError("unreviewed coastline cannot enter native APK")
            src = source / "land-tiles" / match.group()
            if not src.is_file() or src.is_symlink():
                raise ValueError("source terrain tile unavailable or symlinked")
            payload = src.read_bytes()
            if digest(payload) != entry.get("native_mesh_sha256"):
                raise ValueError("source terrain tile SHA-256 mismatch")
            if not HEADER.size + 3*15 + 12 <= len(payload) <= MAX_BYTES:
                raise ValueError("tile size outside native GLES2 vertex budget")
            magic, count, triangles, east, north = HEADER.unpack_from(payload)
            if (magic != MAGIC or not 3 <= count <= 16641 or
                not 1 <= triangles <= 32768 or
                len(payload) != HEADER.size+count*15+triangles*12):
                raise ValueError("invalid binary SITKA01 tile header")
            # A matching manifest hash authenticates bytes, not GPU-safe topology.
            # Reject corrupt source triangles before any indices reach GLES2.
            index_offset = HEADER.size + count*15
            for a, b, c in struct.iter_unpack("<III", payload[index_offset:]):
                if a >= count or b >= count or c >= count:
                    raise ValueError("source SITKA01 triangle index out of vertex range")
                if a == b or b == c or c == a:
                    raise ValueError("source SITKA01 degenerate triangle")
            expected_east = 478720 + col*512
            expected_north = 6324480 - row*512
            if (east != expected_east or north != expected_north or
                entry.get("origin_east_m") != expected_east or
                entry.get("origin_north_m") != expected_north or
                entry.get("terrain_vertices") != count or
                entry.get("terrain_triangles") != triangles):
                raise ValueError("tile is not correctly anchored to authoritative grid")
            name = match.group().split("/")[-1]
            (folder/name).write_bytes(payload)
            crc = zlib.crc32(payload) & 0xffffffff
            records.append((row, col, name, len(payload), crc,
                            count, triangles, east, north))
        if seen != {(r,c) for r in range(4) for c in range(4)}:
            raise ValueError("holes or duplicate indices in the 4x4 land coverage")
        records.sort()
        lines = [
            "SITKA-LAND-INDEX-1",
            "crs EPSG32608",
            "datum UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
            "grid 4 4 512 478720 6324480",
            "tiles 16",
        ]
        for row, col, name, size, crc, verts, tris, east, north in records:
            lines.append(f"{row} {col} {name} {size} {crc} {verts} {tris} {east} {north}")
        (stage/"sitka/coast/index.txt").write_text("\n".join(lines)+"\n", encoding="ascii")
        stage.rename(output)
        return {"source": str(source), "tiles": len(records),
                "native_index": "sitka/coast/index.txt",
                "bytes": sum(r[3] for r in records),
                "not_for_navigation": True,
                "vertical_datum_unverified": True,
                "mask_source": "NOAA_ENC_CHART_LAND"}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--source",type=Path,default=SOURCE)
    a=p.parse_args()
    result=write_assets(a.output,a.source)
    print("STAGED authentic NOAA/USGS source-only geographic viewer assets:")
    print(json.dumps(result,sort_keys=True))

if __name__ == "__main__":
    main()
