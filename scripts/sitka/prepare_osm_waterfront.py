#!/usr/bin/env python3
"""Convert a supplied OSM city extract into review candidates, never game assets.

No Google-derived geometry, network reads, fabricated building styles or height.
OSM ways retain their stable IDs and coordinate rings. Current existence and
distinctive facade identity still require independent photo/source review.
"""
import argparse
import hashlib
import json
import math
import re
import xml.etree.ElementTree as ET
from pathlib import Path

MAX_XML_BYTES = 24_000_000
MAX_NODES = 200_000
MAX_WAYS = 80_000
MAX_OUTPUT = 2000
MAX_VERTICES = 32


def bounds(text):
    try:
        v = [float(s.strip()) for s in text.split(",")]
    except (TypeError, ValueError) as exc:
        raise ValueError("bounds must be west,south,east,north") from exc
    if len(v) != 4 or not all(math.isfinite(x) for x in v):
        raise ValueError("bounds must contain four finite coordinates")
    west, south, east, north = v
    if not (-180 <= west < east <= 180 and -90 <= south < north <= 90):
        raise ValueError("invalid WGS84 bounds")
    if east-west > .25 or north-south > .25:
        raise ValueError("study area exceeds per-batch footprint budget")
    return v


def sourced_height(tags):
    if "height" in tags:
        m = re.fullmatch(r"\s*([0-9]+(?:\.[0-9]+)?)\s*(?:m)?\s*", tags["height"])
        if m and .25 <= float(m.group(1)) <= 70:
            return {"height_m": float(m.group(1)), "height_source": "osm:height"}
    if "building:levels" in tags:
        m = re.fullmatch(r"\s*([0-9]+(?:\.[0-9]+)?)\s*", tags["building:levels"])
        if m and 1 <= float(m.group(1)) <= 20:
            return {"building:levels": float(m.group(1)),
                    "height_source": "osm:building:levels"}
    return {"height_source": "unknown"}


def active_candidate(tags):
    kind = tags.get("building", "").strip().lower()
    if not kind or kind in ("no", "construction", "ruins", "demolished", "proposed"):
        return False
    return not any(key in tags for key in
                   ("demolished:building", "removed:building", "razed:building",
                    "proposed:building"))


def prepare(osm_file, study_bounds, output):
    osm_file, output = Path(osm_file), Path(output)
    extent = bounds(study_bounds)
    if not osm_file.is_file() or osm_file.stat().st_size > MAX_XML_BYTES:
        raise ValueError("OSM extract missing or exceeds the byte budget")
    source_hash = hashlib.sha256(osm_file.read_bytes()).hexdigest()
    nodes = {}
    candidates = []
    node_count = way_count = 0
    for _, elem in ET.iterparse(osm_file, events=("end",)):
        if elem.tag == "node":
            node_count += 1
            if node_count > MAX_NODES:
                raise ValueError("OSM node budget exceeded")
            lat = float(elem.get("lat", "nan"))
            lon = float(elem.get("lon", "nan"))
            if not (math.isfinite(lat) and math.isfinite(lon) and
                    -90 <= lat <= 90 and -180 <= lon <= 180):
                raise ValueError("OSM node coordinates invalid")
            nodes[elem.get("id")] = (lon, lat)
        elif elem.tag == "way":
            way_count += 1
            if way_count > MAX_WAYS:
                raise ValueError("OSM way budget exceeded")
            tags = {t.get("k"): t.get("v") for t in elem.findall("tag")}
            if active_candidate(tags):
                refs = [n.get("ref") for n in elem.findall("nd")]
                if 4 <= len(refs) <= MAX_VERTICES + 1 and refs[0] == refs[-1]:
                    try:
                        outline = [nodes[n] for n in refs[:-1]]
                    except KeyError:
                        outline = []
                    west, south, east, north = extent
                    if len(outline) >= 3 and all(
                            west <= x <= east and south <= y <= north
                            for x, y in outline):
                        if len(set(outline)) != len(outline):
                            raise ValueError("OSM building ring has repeated vertices")
                        way_id = elem.get("id")
                        if way_id is None or not re.fullmatch(r"[1-9][0-9]*", way_id):
                            raise ValueError("invalid source way ID")
                        props = {
                            "feature_id": "osm_w" + way_id,
                            "source_feature_id": "osm:way:" + way_id,
                            "physical_status": "needs_verification",
                            "sitka_kind": "unclassified",
                            "building_osm_tag": tags["building"],
                            "source_timestamp": elem.get("timestamp", "unknown"),
                            "source_version": elem.get("version", "unknown")}
                        props.update(sourced_height(tags))
                        candidates.append({
                            "type": "Feature", "id": props["feature_id"],
                            "properties": props,
                            "geometry": {"type": "Polygon",
                                         "coordinates": [[list(p) for p in
                                                          outline+[outline[0]]]]}})
        # Clearing children before the enclosing way ends discards its <nd>
        # and <tag> references; only clear the completed parent elements.
        if elem.tag in ("node", "way", "relation"):
            elem.clear()
    if not candidates:
        raise ValueError("no closed building polygons in study area")
    if len(candidates) > MAX_OUTPUT:
        raise ValueError("candidate count exceeds per-batch limit")
    candidates.sort(key=lambda f: int(
        f["properties"]["source_feature_id"].rsplit(":", 1)[-1]))
    collection = {
        "type": "FeatureCollection",
        "crs": {"type": "name", "properties": {"name": "EPSG:4326"}},
        "features": candidates}
    report = {
        "format": "sitka-osm-building-review-1",
        "status": "REVIEW_ONLY",
        "source_filename": osm_file.name, "source_sha256": source_hash,
        "license": "ODbL 1.0",
        "attribution": "© OpenStreetMap contributors",
        "license_url": "https://www.openstreetmap.org/copyright",
        "bounds_wgs84": extent,
        "feature_count": len(candidates),
        "missing_height_count": sum(
            f["properties"]["height_source"] == "unknown" for f in candidates),
        "limitations": [
            "Not a live game asset", "Current existence not verified",
            "No inferred facade style or building height",
            "OSM relations/multipolygons require dedicated GIS handling",
            "Cross-boundary footprints are omitted, never cropped"]}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(collection, indent=2, sort_keys=True)+"\n",
                      encoding="utf-8")
    output.with_suffix(".manifest.json").write_text(
        json.dumps(report, indent=2, sort_keys=True)+"\n", encoding="utf-8")
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--osm", type=Path, required=True,
                   help="downloaded OpenStreetMap XML, not a Google Maps export")
    p.add_argument("--bounds", required=True,
                   help="WGS84 west,south,east,north")
    p.add_argument("--output", type=Path, required=True,
                   help="review candidate GeoJSON")
    a = p.parse_args()
    r = prepare(a.osm, a.bounds, a.output)
    print(f"Sitka review-only OSM candidates: {r['feature_count']}")


if __name__ == "__main__":
    main()
