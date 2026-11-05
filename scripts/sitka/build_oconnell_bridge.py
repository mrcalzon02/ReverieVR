#!/usr/bin/env python3
"""Bake ONLY a reviewed, sourced O'Connell Bridge axis into the native planes.

Never downloads Google/OSM data or guesses an endpoint, roadway grade, pier or
tower. The author must supply a licensed line geometry AND independently
authored elevations for every sampled point, with matching raw-source hash.
"""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

from build_waterfront import HEADER, MAGIC, digest, number

MAX_POINTS = 65
MAX_FILE_BYTES = 8192


def checked_axis(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("type") != "FeatureCollection" or len(data.get("features", [])) != 1:
        raise ValueError("expected one reviewed bridge FeatureCollection")
    feature = data["features"][0]
    if feature.get("type") != "Feature" or not isinstance(feature.get("properties"), dict):
        raise ValueError("reviewed bridge feature invalid")
    geom = feature.get("geometry")
    if not isinstance(geom, dict) or geom.get("type") != "LineString":
        raise ValueError("approved bridge axis requires one LineString")
    coords = geom.get("coordinates")
    if not isinstance(coords, list) or not 2 <= len(coords) <= MAX_POINTS:
        raise ValueError("bridge axis count must be 2..65")
    pts = []
    for c in coords:
        if not isinstance(c, list) or len(c) != 2:
            raise ValueError("bridge point must have two plan coordinates")
        pts.append((number(c[0], "bridge east/lon", -20000000, 20000000),
                    number(c[1], "bridge north/lat", -20000000, 20000000)))
    return data, feature, pts


def bake(terrain_manifest_path, bridge_geojson_path, grade_path, output):
    from pyproj import CRS, Transformer

    terrain_manifest_path = Path(terrain_manifest_path)
    bridge_geojson_path = Path(bridge_geojson_path)
    grade_path = Path(grade_path)
    output = Path(output)
    manifest = json.loads(terrain_manifest_path.read_text(encoding="utf-8"))
    if manifest.get("format") != "sitka-static-terrain-1":
        raise ValueError("unapproved terrain manifest")
    name = manifest.get("mesh")
    if not isinstance(name, str) or Path(name).name != name:
        raise ValueError("terrain mesh must be a local manifest file")
    mesh_file = terrain_manifest_path.parent / name
    if digest(mesh_file) != manifest.get("mesh_sha256"):
        raise ValueError("terrain mesh checksum mismatch")
    with mesh_file.open("rb") as source:
        header = source.read(HEADER.size)
    if len(header) != HEADER.size or header[:8] != MAGIC:
        raise ValueError("wrong Sitka terrain binary header")
    _, _, _, origin_e, origin_n = HEADER.unpack(header)
    if not math.isfinite(origin_e) or not math.isfinite(origin_n):
        raise ValueError("nonfinite terrain origin")
    if abs(origin_e - manifest.get("east_origin", float("inf"))) > .001 or (
        abs(origin_n - manifest.get("north_origin", float("inf"))) > .001
    ):
        raise ValueError("terrain mesh and manifest origins differ")
    crs = CRS.from_user_input(manifest.get("coordinate_system", ""))
    if crs.to_epsg() != 32608:
        raise ValueError("unsupported bridge/terrain CRS (EPSG:32608 required)")
    fc, feature, coords = checked_axis(bridge_geojson_path)
    props = feature["properties"]
    grade = json.loads(grade_path.read_text(encoding="utf-8"))
    if grade.get("format") != "sitka-bridge-grade-1" or (
        grade.get("site_id") != "oconnell_bridge" or
        props.get("site_id") != "oconnell_bridge"
    ):
        raise ValueError("wrong bridge site identity")
    if grade.get("geometry_reviewed") is not True or (
        props.get("physical_status") != "existing"
    ):
        raise ValueError("bridge requires current physical/source review")
    if props.get("source_feature_id") != grade.get("source_feature_id"):
        raise ValueError("bridge way ID must match its reviewed grade")
    source_id = grade.get("source_feature_id")
    if not isinstance(source_id, str) or not source_id.startswith("osm:way:") or (
        not source_id[8:].isdigit()
    ):
        raise ValueError("bridge requires a specific OSM way identity")
    source_sha = digest(bridge_geojson_path)
    if grade.get("source_sha256") != source_sha:
        raise ValueError("bridge source checksum mismatch")
    evidence = grade.get("evidence")
    if not isinstance(evidence, dict) or any(
        not isinstance(evidence.get(key), str) or not evidence[key].strip()
        for key in ("provider", "dataset", "license", "retrieved", "url")
    ):
        raise ValueError("bridge measurement/license provenance incomplete")
    if not evidence["url"].startswith("https://"):
        raise ValueError("bridge measurement reference must be HTTPS")
    heights = grade.get("deck_elevations_m")
    if not isinstance(heights, list) or len(heights) != len(coords):
        raise ValueError("one sourced road-deck elevation required per axis node")
    heights = [number(h, "bridge roadway elevation", -2000, 2000)
               for h in heights]
    width = number(grade.get("width_m"), "bridge width", 2, 50)
    thickness = number(grade.get("deck_thickness_m"), "deck thickness", .05, 5)
    side = number(grade.get("side_height_m"), "pylon silhouette height", .25, 60)
    source_crs_name = fc.get("crs", {}).get("properties", {}).get("name", "EPSG:4326")
    source_crs = CRS.from_user_input(source_crs_name)
    transform = Transformer.from_crs(source_crs, crs, always_xy=True)
    east_north = [transform.transform(x, y) for x, y in coords]
    local = [(e - origin_e, h, n - origin_n)
             for (e, n), h in zip(east_north, heights)]
    for x, y, z in local:
        if not all(math.isfinite(v) for v in (x, y, z)) or (
            abs(x)>100000 or abs(z)>100000
        ):
            raise ValueError("bridge point outside local projected terrain")
    vx = local[-1][0] - local[0][0]
    vz = local[-1][2] - local[0][2]
    length = math.hypot(vx, vz)
    if not 1 <= length <= 5000:
        raise ValueError("invalid source bridge axis length")
    ux, uz = vx/length, vz/length
    previous = -1.
    for i, (x, y, z) in enumerate(local):
        dx, dz = x-local[0][0], z-local[0][2]
        along = dx*ux + dz*uz
        cross = dx*(-uz) + dz*ux
        if abs(cross)>.48 or along < previous+.25 and i>0:
            raise ValueError("bridge source is curved/skewed; split verified spans")
        if i>0:
            distance=math.hypot(x-local[i-1][0], z-local[i-1][2])
            if abs(y-local[i-1][1])>distance*.29:
                raise ValueError("bridge roadway grade too steep")
        previous=along
    # Three decimals are sufficient for a stylized game while staying well
    # inside the runtime half-metre source axis tolerance.
    row = [f"{x:.3f},{y:.3f},{z:.3f}" for x, y, z in local]
    text = "\n".join((
        "SITKA-BRIDGE-1", "site_id=oconnell_bridge",
        "geometry_reviewed=true", "coordinate_system=EPSG:32608",
        "source_feature_id="+source_id, "source_sha256="+source_sha,
        f"origin_east={origin_e:.3f}",f"origin_north={origin_n:.3f}",
        f"width_m={width:.4f}",f"deck_thickness_m={thickness:.4f}",
        f"side_height_m={side:.4f}",f"axis_count={len(local)}",
        *(f"axis.{i}={v}" for i,v in enumerate(row))
    ))+"\n"
    if len(text.encode("utf-8"))>MAX_FILE_BYTES:
        raise ValueError("bridge layout too large")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    report = {
        "format":"sitka-bridge-export-1",
        "geometry_state":"REVIEWED_GEOMETRY_ONLY",
        "source_feature_id":source_id,
        "source_sha256":source_sha,
        "grade_sha256":digest(grade_path),
        "layout_sha256":hashlib.sha256(text.encode()).hexdigest(),
        "terrain_sha256":manifest["mesh_sha256"],
        "coordinate_system":str(crs),"origin_east":origin_e,
        "origin_north":origin_n,
        "axis_count":len(local), "length_m":length,
        "provenance":evidence,
        "caveats":["No inferred deck height or invented alignment",
                   "Pylon placement represented as a texture, not surveyed 3D",
                   "Current source approval is an authoring responsibility"]
    }
    output.with_suffix(".manifest.json").write_text(
        json.dumps(report,sort_keys=True,indent=2)+"\n",encoding="utf-8")
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--terrain-manifest",type=Path,required=True)
    p.add_argument("--bridge-geojson",type=Path,required=True)
    p.add_argument("--grade",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    m=bake(a.terrain_manifest,a.bridge_geojson,a.grade,a.output)
    print(f"Approved Sitka bridge layout: {m['axis_count']} points")


if __name__=="__main__":
    main()
