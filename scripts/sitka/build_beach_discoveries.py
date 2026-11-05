#!/usr/bin/env python3
"""Author tiny *game* shoreline animals inside licensed, reviewed beach polygons.
This never invents the beach geometry or places archaeological artifacts.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import re

ZONES={
 "rocky":("purple_shore_crab","green_shore_crab","small_sea_star",
          "limpet","chiton","barnacle_patch","mussel_cluster","sea_snail"),
 "tidepool":("purple_shore_crab","hermit_crab","small_sea_star",
             "sea_anemone","nudibranch","sea_cucumber","sea_urchin"),
 "gravel":("empty_clam_shell","beach_pebble","sea_glass",
           "littleneck_clam","driftwood_chip"),
 "wrack":("driftwood_chip","kelp_float","crab_molt",
          "empty_clam_shell","sea_glass")
}
WEIGHTS={"nudibranch":1,"sea_cucumber":1,"sea_urchin":2,
         "sea_glass":2,"hermit_crab":3,"crab_molt":3}
SITE_NAME=re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
SHA=re.compile(r"[0-9a-f]{64}\Z")

def numeric(n,field,limit=100000.):
    if isinstance(n,bool) or not isinstance(n,(float,int)) or not math.isfinite(n) or abs(n)>limit:
        raise ValueError("bad "+field)
    return float(n)

def inside(x,z,points):
    hit=False
    for a,b in zip(points,points[1:]+points[:1]):
        if (a[1]>z)!=(b[1]>z) and x<(b[0]-a[0])*(z-a[1])/(b[1]-a[1])+a[0]:
            hit=not hit
    return hit

def polygon(value):
    if not isinstance(value,list) or not 3<=len(value)<=32:
        raise ValueError("beach polygon requires 3..32 vertices")
    p=[]
    for v in value:
        if not isinstance(v,list) or len(v)!=2:
            raise ValueError("invalid authored polygon point")
        p.append((numeric(v[0],"x"),numeric(v[1],"z")))
    if len(set(p))!=len(p):
        raise ValueError("repeated authored vertex")
    area=sum(p[i][0]*p[(i+1)%len(p)][1]-p[(i+1)%len(p)][0]*p[i][1]
             for i in range(len(p)))*.5
    if abs(area)<=1.:
        raise ValueError("degenerate beach polygon")
    return p

def export(document,catalog_text):
    if not isinstance(document,dict) or document.get("format")!="SITKA-BEACH-AREAS-1":
        raise ValueError("wrong beach source version")
    p=document.get("provenance")
    if not isinstance(p,dict):
        raise ValueError("missing licensed geographic source")
    sid=p.get("source_id")
    sha=p.get("sha256")
    url=p.get("url")
    if (not isinstance(sid,str) or not SITE_NAME.fullmatch(sid) or
        not isinstance(sha,str) or not SHA.fullmatch(sha) or
        not isinstance(url,str) or not url.startswith("https://") or
        "google.com" in url.lower() or
        not isinstance(p.get("license"),str) or not p["license"].strip() or
        p.get("reviewed") is not True or p.get("crs")!="EPSG32608"):
        raise ValueError("beach source attribution/CRS/review missing")
    origin=document.get("origin")
    if not isinstance(origin,list) or len(origin)!=2:
        raise ValueError("no EPSG32608 terrain origin")
    east=numeric(origin[0],"origin_east",1.e8)
    north=numeric(origin[1],"origin_north",1.e8)
    areas=document.get("areas")
    if not isinstance(areas,list) or not 1<=len(areas)<=32:
        raise ValueError("no approved beach habitat areas")
    used=set()
    names=set()
    rows=[]
    for area in sorted(areas,key=lambda a:a.get("id","")):
        if not isinstance(area,dict):
            raise ValueError("invalid source area")
        label=area.get("id")
        zone=area.get("zone")
        if (not isinstance(label,str) or not SITE_NAME.fullmatch(label)
            or label in names or zone not in ZONES):
            raise ValueError("invalid approved beach habitat")
        names.add(label)
        start=area.get("first_site_id")
        count=area.get("count")
        if (type(start)!=int or type(count)!=int or count<1 or count>12 or
            start<0 or start+count>64):
            raise ValueError("invalid immutable 64-slot site range")
        indexes=set(range(start,start+count))
        if used & indexes:
            raise ValueError("would reuse existing permanent discovery ID")
        used.update(indexes)
        boundary=polygon(area.get("polygon"))
        lo_x=min(x for x,z in boundary)
        hi_x=max(x for x,z in boundary)
        lo_z=min(z for x,z in boundary)
        hi_z=max(z for x,z in boundary)
        placed=[]
        for site_id in sorted(indexes):
            seed=int.from_bytes(hashlib.sha256(
                f"{sha}:{label}:{site_id}".encode()).digest()[:8],"big")
            rng=random.Random(seed)
            names_in_zone=ZONES[zone]
            item=rng.choices(names_in_zone,
                [WEIGHTS.get(name,5) for name in names_in_zone],k=1)[0]
            if f'"{item}"' not in catalog_text:
                raise ValueError("model token not in native registry")
            for _ in range(500):
                x=round(rng.uniform(lo_x,hi_x),3)
                z=round(rng.uniform(lo_z,hi_z),3)
                if inside(x,z,boundary) and all(
                    (x-ox)**2+(z-oz)**2>.09 for ox,oz in placed):
                    placed.append((x,z))
                    rows.append((site_id,item,x,z,zone))
                    break
            else:
                raise ValueError("authored polygon too small for allowed find count")
    out=["SITKA-BEACH-1",
         "# Generated GAME specimens in licensed beach habitat, not actual sightings",
         f"P|{sid}|{sha}|EPSG32608|reviewed",
         f"O|{east:.3f}|{north:.3f}"]
    out.extend(f"D|{site}|{item}|{x:.3f}|{z:.3f}|{zone}"
               for site,item,x,z,zone in sorted(rows))
    result="\n".join(out)+"\n"
    if len(result.encode("ascii"))>8192:
        raise ValueError("beyond Sitka native beach-asset budget")
    return result

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--areas",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--catalog",type=Path,default=
        Path(__file__).resolve().parents[2]/
        "app/src/main/jni/native/sitka_intertidal_models.h")
    args=parser.parse_args()
    result=export(json.loads(args.areas.read_text()),
                  args.catalog.read_text())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(result,encoding="ascii")
    print("Exported",result.count("\nD|"),
          "source-grounded GAME beach finds; zero archaeological artifacts")
