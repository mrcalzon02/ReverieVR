#!/usr/bin/env python3
"""Bake reviewed, publicly accessible Sitka pedestrian source lines to .sitkaw.

Input must be a licensed GeoJSON FeatureCollection of actual mapped
walkway LineStrings plus specifically reviewed start/fishing stance Points.
No implicit junctions, road width guesses, crossing roads, access permissions,
waterfront fill, fake shore nodes, or terrain-based bridge deck inference.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path

from build_waterfront import HEADER, MAGIC, digest, number

KINDS = frozenset(("sidewalk","seawalk","trail","shore_path",
                   "crossing","bridge_walk","steps","public_pier"))
MAX_NODES,MAX_EDGES,MAX_STANCES,MAX_BYTES=4096,8192,128,262144
SOURCE_KEYS=("provider","dataset","license","retrieved","url")
FID=re.compile(r"[A-Za-z0-9_-]{1,64}\Z")


def require_source(obj, what):
    if not isinstance(obj,dict) or any(
        not isinstance(obj.get(k),str) or not obj[k].strip()
        for k in SOURCE_KEYS
    ):
        raise ValueError(f"{what}: missing licensed provenance")
    if not obj["url"].startswith("https://") or (
        "google" in (obj["provider"]+" "+obj["dataset"]).lower()
    ):
        raise ValueError(f"{what}: map license/source not permitted")


def bake(manifest_path,dem_path,geometry_path,ledger_path,fish_path,output):
    import rasterio
    from pyproj import CRS,Transformer
    manifest_path,dem_path,geometry_path,ledger_path,fish_path,output=map(
        Path,(manifest_path,dem_path,geometry_path,ledger_path,fish_path,output))
    manifest=json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("format")!="sitka-static-terrain-1":
        raise ValueError("reviewed Sitka terrain manifest required")
    mesh_name=manifest.get("mesh")
    if not isinstance(mesh_name,str) or Path(mesh_name).name!=mesh_name:
        raise ValueError("terrain mesh must be adjacent to manifest")
    mesh=manifest_path.parent/mesh_name
    if digest(mesh)!=manifest.get("mesh_sha256"):
        raise ValueError("terrain mesh SHA-256 mismatch")
    with mesh.open("rb") as f:
        header=f.read(HEADER.size)
    if len(header)!=HEADER.size or header[:8]!=MAGIC:
        raise ValueError("invalid terrain mesh header")
    _,_,_,east0,north0=HEADER.unpack(header)
    if (not math.isfinite(east0) or not math.isfinite(north0) or
        abs(east0-number(manifest.get("east_origin"),"east origin",-1e8,1e8))>.001 or
        abs(north0-number(manifest.get("north_origin"),"north origin",-1e8,1e8))>.001):
        raise ValueError("terrain origin and manifest disagree")
    projection=CRS.from_user_input(manifest.get("coordinate_system",""))
    if projection.to_epsg()!=32608:
        raise ValueError("Sitka walkway CRS must be EPSG:32608")
    ledger=json.loads(ledger_path.read_text(encoding="utf-8"))
    require_source(ledger.get("walkways"),"pedestrian sources")
    require_source(ledger.get("fishing_access"),"shore fishing access")
    if ledger.get("geometry_reviewed") is not True:
        raise ValueError("pedestrian source licensing/field review incomplete")
    data=json.loads(geometry_path.read_text(encoding="utf-8"))
    if data.get("type")!="FeatureCollection" or not isinstance(data.get("features"),list):
        raise ValueError("pedestrian input must be a GeoJSON FeatureCollection")
    source_crs=CRS.from_user_input(
        data.get("crs",{}).get("properties",{}).get("name","EPSG:4326"))
    transform=Transformer.from_crs(source_crs,projection,always_xy=True)
    known_fish=set()
    fish=fish_path.read_text(encoding="utf-8").splitlines()
    if not fish or fish[0]!="SITKA-FISH-SITES-1":
        raise ValueError("matching reviewed native fishing spot source required")
    for row in fish[1:]:
        if not row or row.startswith("#"):continue
        parts=row.split()
        if len(parts)!=6 or not FID.fullmatch(parts[0]):
            raise ValueError("malformed matching fishing spot")
        known_fish.add(parts[0])
    if not known_fish:
        raise ValueError("no verified fishing spots to link shore stances")
    nodes={}
    routes=[]
    starts=[]
    stances=[]
    seen=set()
    def source_properties(feature):
        p=feature.get("properties")
        if not isinstance(p,dict):
            raise ValueError("missing pedestrian feature properties")
        fid=p.get("feature_id")
        sourceid=p.get("source_feature_id")
        if not isinstance(fid,str) or not FID.fullmatch(fid) or fid in seen:
            raise ValueError("unstable or duplicate walk feature_id")
        seen.add(fid)
        if not isinstance(sourceid,str) or not sourceid.strip() or (
            p.get("physical_status")!="existing" or
            p.get("public_access") is not True or
            p.get("geometry_reviewed") is not True
        ):
            raise ValueError("pedestrian feature needs reviewed current public access")
        return p,fid
    def xyz(point,vertical):
        if not isinstance(point,(list,tuple)) or len(point)!=2:
            raise ValueError("walk source must contain exactly XY")
        x,z=transform.transform(
            number(point[0],"walk source X",-2e7,2e7),
            number(point[1],"walk source Y",-2e7,2e7))
        if not math.isfinite(x) or not math.isfinite(z):
            raise ValueError("pedestrian CRS conversion nonfinite")
        xx,zz=round(x-east0,3),round(z-north0,3)
        yy=round(number(vertical,"sourced walkway elevation",-2000,2000),3)
        if abs(xx)>100000 or abs(zz)>100000:
            raise ValueError("walk route outside terrain-local world")
        return (xx,yy,zz)
    with rasterio.open(dem_path) as dem:
        if str(dem.crs)!=str(projection) or dem.count!=1 or dem.nodata is None or (
            digest(dem_path)!=manifest.get("elevation_sha256")
        ):
            raise ValueError("pedestrian DEM must match approved terrain source")
        for f in data["features"]:
            if not isinstance(f,dict) or f.get("type")!="Feature":
                raise ValueError("malformed pedestrian GeoJSON feature")
            p,fid=source_properties(f)
            shape=f.get("geometry",{})
            role=p.get("role")
            if role=="walkway":
                kind=p.get("sitka_kind")
                if kind not in KINDS or shape.get("type")!="LineString":
                    raise ValueError("walkway requires source LineString and type")
                xy=shape.get("coordinates")
                if not isinstance(xy,list) or not 2<=len(xy)<=256:
                    raise ValueError("walkway coordinates count invalid")
                width=number(p.get("width_m"),"reviewed walkway width",.4,15)
                elev=p.get("sourced_elevations_m")
                if kind in ("bridge_walk","public_pier") and elev is None:
                    raise ValueError("bridge and waterfront pier require deck elevations")
                if elev is not None:
                    if (not isinstance(elev,list) or len(elev)!=len(xy)
                        or not isinstance(p.get("elevation_source"),str)
                        or not p["elevation_source"].strip()):
                        raise ValueError("individually sourced elevations require provenance")
                    height=[number(v,"authored walkway height",-2000,2000) for v in elev]
                else:
                    east_north=[transform.transform(x,z) for x,z in xy]
                    values=list(dem.sample(east_north,masked=True))
                    height=[]
                    for value in values:
                        v=value[0]
                        if getattr(v,"mask",False) or not math.isfinite(float(v)):
                            raise ValueError("pedestrian route outside DEM coverage")
                        height.append(float(v))
                points=[xyz(point,vertical) for point,vertical in zip(xy,height)]
                for a,b in zip(points,points[1:]):
                    if a==b:raise ValueError("duplicate walkway point")
                    distance=math.hypot(b[0]-a[0],b[2]-a[2])
                    limit=1.5 if kind=="steps" else .35
                    if distance<.1 or distance>500 or abs(a[1]-b[1])>distance*limit:
                        raise ValueError("unverified giant span or steep walk path")
                    nodes[a]=None;nodes[b]=None
                    routes.append((fid,a,b,kind,width))
            elif role=="start":
                if shape.get("type")!="Point" or "sourced_elevation_m" not in p:
                    raise ValueError("walk start needs surveyed point/elevation")
                starts.append((fid,xyz(shape["coordinates"],p["sourced_elevation_m"])))
            elif role=="fishing_stance":
                if shape.get("type")!="Point" or "sourced_elevation_m" not in p:
                    raise ValueError("fishing access needs surveyed point/elevation")
                if p.get("spot_id") not in known_fish:
                    raise ValueError("shore stance requires existing surveyed fishing site")
                radius=number(p.get("radius_m"),"legal stance radius",.3,2.5)
                stances.append((fid,xyz(shape["coordinates"],p["sourced_elevation_m"]),
                                p["spot_id"],radius))
            else:
                raise ValueError("unapproved walkway/shore feature role")
    if not 1<=len(nodes)<=MAX_NODES or len(routes)>MAX_EDGES:
        raise ValueError("Sitka walk graph exceeds node/edge byte budget")
    if len(starts)!=1 or len(stances)>MAX_STANCES:
        raise ValueError("one verified public spawn and bounded shore access required")
    sorted_points=sorted(nodes)
    ids={pt:f"n{i:05d}" for i,pt in enumerate(sorted_points)}
    if starts[0][1] not in ids:
        raise ValueError("public start must coincide with verified walkway vertex")
    for _,pt,_,_ in stances:
        if pt not in ids:
            raise ValueError("public fishing stance must be on an exact walkway vertex")
    rows=["SITKA-WALK-1",
          f"P|sitka_walk|{digest(geometry_path)}|EPSG32608|reviewed",
          f"O|{east0:.6f}|{north0:.6f}"]
    rows.extend(f"N|{ids[pt]}|{pt[0]:.3f}|{pt[1]:.3f}|{pt[2]:.3f}"
                for pt in sorted_points)
    for index,(_,a,b,kind,width) in enumerate(sorted(routes,
        key=lambda entry:(entry[0],entry[1],entry[2]))):
        rows.append(f"E|e{index:05d}|{ids[a]}|{ids[b]}|{kind}|{width:.3f}")
    rows.append(f"S|{starts[0][0]}|{ids[starts[0][1]]}")
    for fid,pt,fish_id,radius in sorted(stances):
        rows.append(f"F|{fid}|{ids[pt]}|{fish_id}|{radius:.3f}")
    text="\n".join(rows)+"\n"
    if len(text.encode("utf-8"))>MAX_BYTES:
        raise ValueError("Sitka walk native graph exceeds 256 KiB")
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(text,encoding="utf-8")
    report={
        "format":"sitka-reviewed-walkways-1",
        "source_geometry_sha256":digest(geometry_path),
        "source_ledger_sha256":digest(ledger_path),
        "source_dem_sha256":digest(dem_path),
        "fishing_site_sha256":digest(fish_path),
        "layout_sha256":hashlib.sha256(text.encode()).hexdigest(),
        "site_access_provenance":ledger,
        "coordinate_system":"EPSG:32608",
        "east_origin":east0,"north_origin":north0,
        "node_count":len(nodes),"edge_count":len(routes),
        "stance_count":len(stances),
        "status":"source-reviewed network geometry; headset path collision and citywide asset paging not yet validated",
        "limitations":[
            "Only surveyed or approved public ways may be imported",
            "No implicit road crossing or coastline shortcut from geometric proximity",
            "No terrain-height inference on bridges or exposed piers",
            "Real-world access can change and requires periodic review"]
    }
    output.with_suffix(".manifest.json").write_text(
        json.dumps(report,sort_keys=True,indent=2)+"\n",encoding="utf-8")
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--terrain-manifest",type=Path,required=True)
    p.add_argument("--dem",type=Path,required=True)
    p.add_argument("--walkways",type=Path,required=True)
    p.add_argument("--provenance",type=Path,required=True)
    p.add_argument("--fishing-spots",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    print(bake(a.terrain_manifest,a.dem,a.walkways,
               a.provenance,a.fishing_spots,a.output))


if __name__=="__main__":
    main()
