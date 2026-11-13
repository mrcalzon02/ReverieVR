#!/usr/bin/env python3
"""Acquire genuine NOAA ENC Harbour LAND polygons for a bounded Sitka AOI.

Authoritative NOAA ENC map service, not a coastline invented from DEM heights.
The output can mask REAL terrain heights inside chart-source polygons; it is NOT
a tidal shoreline survey, navigation chart, or independently dated coastline.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import tempfile
import time
from urllib import parse, request
from urllib.error import HTTPError, URLError

from acquire_3dep_geography import load_grid, sha256

BASE = ("https://encdirect.noaa.gov/arcgis/rest/services/"
        "encdirect/enc_harbour/MapServer/233")
QUERY = BASE + "/query"
MAX_IDS = 750
MAX_BATCH = 50
MAX_RESPONSE = 12_000_000

def get_json(params):
    # Refuse arbitrary URLs: only one official NOAA layer is authorized.
    url = QUERY + "?" + parse.urlencode(params)
    last = None
    for attempt in range(3):
        try:
            req = request.Request(url, headers={
                "User-Agent": "ReverieVR-Sitka-ENC-Land/1.0",
                "Accept": "application/json,application/geo+json"})
            with request.urlopen(req, timeout=70) as response:
                data = response.read(MAX_RESPONSE + 1)
            if not 0 < len(data) <= MAX_RESPONSE:
                raise ValueError("NOAA query response exceeds size budget")
            obj = json.loads(data)
            if isinstance(obj, dict) and obj.get("error"):
                raise ValueError("NOAA ArcGIS error: " + str(obj["error"])[:250])
            if not isinstance(obj, dict):
                raise ValueError("NOAA returned malformed JSON")
            return obj, hashlib.sha256(data).hexdigest()
        except (URLError, HTTPError, TimeoutError, OSError) as exc:
            last = exc
            if attempt < 2:
                time.sleep(2 ** attempt)
    raise RuntimeError("official NOAA ENC service did not respond") from last

def aoi(grid):
    from pyproj import Transformer
    from shapely.geometry import box
    g = grid["grid"]
    west=float(g["west_grid_node_m"])
    north=float(g["north_grid_node_m"])
    east=west+g["columns"]*g["tile_intervals"]*g["sample_spacing_m"]
    south=north-g["rows"]*g["tile_intervals"]*g["sample_spacing_m"]
    metric=box(west,south,east,north)
    to_geo=Transformer.from_crs(32608,4326,always_xy=True)
    corners=[to_geo.transform(x,y) for x,y in
             ((west,south),(west,north),(east,south),(east,north))]
    bbox=[min(p[0] for p in corners),min(p[1] for p in corners),
          max(p[0] for p in corners),max(p[1] for p in corners)]
    return metric,bbox

def query_features(grid):
    region,bbox=aoi(grid)
    envelope=",".join(f"{v:.10f}" for v in bbox)
    common={"where":"1=1", "geometry":envelope,
            "geometryType":"esriGeometryEnvelope",
            "inSR":"4326","spatialRel":"esriSpatialRelIntersects","f":"json"}
    ids_result, ids_digest=get_json({**common,"returnIdsOnly":"true"})
    ids=ids_result.get("objectIds")
    if not isinstance(ids,list) or not ids:
        raise ValueError("NOAA ENC Harbour returned no Sitka land polygons")
    if len(ids)>MAX_IDS or any(type(i) is not int or i<0 for i in ids):
        raise ValueError("NOAA ENC polygon ID budget or type invalid")
    if len(set(ids)) != len(ids):
        raise ValueError("NOAA ENC returned duplicate IDs")
    all_features=[]
    digests=[ids_digest]
    for start in range(0,len(ids),MAX_BATCH):
        selection=",".join(str(x) for x in ids[start:start+MAX_BATCH])
        obj,digest=get_json({"objectIds":selection,
            "outFields":"*","returnGeometry":"true",
            "outSR":"4326","f":"geojson"})
        if obj.get("type")!="FeatureCollection" or not isinstance(obj.get("features"),list):
            raise ValueError("NOAA ENC batch is not GeoJSON FeatureCollection")
        if not obj["features"]:
            raise ValueError("NOAA ENC returned empty polygon batch")
        all_features.extend(obj["features"])
        digests.append(digest)
    return all_features,ids,digests,bbox

def validate_land(features,ids,grid):
    from shapely.geometry import shape,box,mapping
    from shapely.ops import transform
    from pyproj import Transformer
    area, bbox=aoi(grid)
    to_utm=Transformer.from_crs(4326,32608,always_xy=True)
    to_geo=Transformer.from_crs(32608,4326,always_xy=True)
    accepted=[]
    seen=set()
    total_m2=0
    for feature in features:
        if feature.get("type")!="Feature":
            raise ValueError("NOAA ENC returned a non-feature record")
        geom=feature.get("geometry")
        if not isinstance(geom,dict) or geom.get("type") not in ("Polygon","MultiPolygon"):
            raise ValueError("NOAA ENC land item is not polygonal")
        raw=shape(geom)
        if not raw.is_valid or raw.is_empty:
            raise ValueError("NOAA source polygon invalid; no artificial repair")
        projected=transform(to_utm.transform,raw)
        clip=projected.intersection(area)
        if clip.is_empty:
            continue
        if not clip.is_valid or not math.isfinite(clip.area):
            raise ValueError("NOAA ENC clipped polygon invalid")
        surfaces=([clip] if clip.geom_type=="Polygon"
                  else list(clip.geoms) if clip.geom_type=="MultiPolygon"
                  else [p for p in getattr(clip,"geoms",[]) if p.geom_type=="Polygon"])
        props=feature.get("properties") or {}
        source_id=str(feature.get("id") or props.get("OBJECTID")
                      or props.get("objectid") or props.get("RCID") or "").strip()
        if not source_id:
            raise ValueError("NOAA polygon has no attributable record id")
        for piece in surfaces:
            if piece.area < 1.0:
                continue
            if len(accepted)>=MAX_IDS:
                raise ValueError("too many intersecting land parts")
            total_m2+=float(piece.area)
            accepted.append({
                "type":"Feature",
                "id":f"noaa-enc-{source_id}-{len(accepted)}",
                "properties":{
                    "provider":"NOAA Office of Coast Survey",
                    "source_product":"ENC Harbour Land Area",
                    "source_id":source_id,
                    "land_area_m2":round(piece.area,2),
                    "quality":"NOAA_ENC_CHART_POLYGON_NOT_SHORELINE_SURVEY",
                    "aoi_clip_boundary_is_not_shoreline":True,
                },
                "geometry":mapping(transform(to_geo.transform,piece))
            })
    if not accepted:
        raise ValueError("NOAA ENC returned no valid Sitka land polygons")
    if total_m2<10000:
        raise ValueError("NOAA ENC land polygons have implausibly low mapped area")
    return {"type":"FeatureCollection",
            "crs":{"type":"name","properties":{"name":"EPSG:4326"}},
            "features":accepted}, total_m2

def acquire(grid_file,destination):
    grid=load_grid(grid_file)
    destination=Path(destination)
    ledger=destination.with_suffix(".source.json")
    if destination.exists() or ledger.exists():
        raise FileExistsError("existing shoreline source must not be overwritten")
    features,ids,digests,bbox=query_features(grid)
    clipped,area=validate_land(features,ids,grid)
    now=datetime.now(timezone.utc).isoformat(timespec="seconds")
    info={
        "schema":1,
        "provider":"NOAA Office of Coast Survey",
        "dataset":"ENC Harbour Land Area (dynamic published cartographic polygons)",
        "license":"U.S. federal NOAA data; attribution; not for navigation",
        "retrieved":now[:10],
        "retrieved_at_utc":now,
        "url":BASE,
        "query_url":QUERY,
        "survey_project_AK2309_CS_N":"NOT_VERIFIED_IN_ENC",
        "geometry_type":"Polygon or MultiPolygon",
        "geometry_source":"AUTHENTIC_NOAA_ENC_CHART_LAND",
        "source_geometry_review":"official NOAA mapped land area; on-site shoreline accuracy not certified",
        "epsg_output":"EPSG:4326",
        "epsg_dem":"EPSG:32608",
        "source_ids_count":len(ids),
        "coast_polygons":len(clipped["features"]),
        "clipped_area_m2":round(area,1),
        "query_bbox_wgs84":bbox,
        "source_batch_sha256":digests,
        "grid_config_sha256":sha256(grid_file),
        "approved_for_redistribution":True,
        "current_geography_reviewed":True,
        "currentness_scope":"current NOAA ENC published polygons, not independently ground-surveyed",
        "NOT_FOR_NAVIGATION":True,
        "shoreline_date":"UNKNOWN_USE_NOAA_CHART_SOURCE_ONLY",
        "has_aoi_clipping_edges_not_actual_shoreline":True,
    }
    destination.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="noaa-enc-",dir=destination.parent) as folder:
        output=Path(folder)/destination.name
        output.write_text(json.dumps(clipped,sort_keys=True,separators=(",",":"))+"\n",
                          encoding="utf-8")
        info["source_geojson_sha256"]=sha256(output)
        destination.write_bytes(output.read_bytes())
        ledger.write_text(json.dumps(info,sort_keys=True,indent=2)+"\n",encoding="utf-8")
    return info

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--grid",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    data=acquire(args.grid,args.output)
    print("VERIFIED NOAA ENC sitka land:",data["coast_polygons"],
          "polygons from",data["source_ids_count"],
          "source features,",data["clipped_area_m2"],"square metres")
if __name__=="__main__":
    main()
