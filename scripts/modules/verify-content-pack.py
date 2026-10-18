#!/usr/bin/env python3
"""Offline, read-only acceptance for an independently released ReverieVR pack."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import zipfile

MAX_ENTRIES=512
MAX_UNCOMPRESSED=96*1024*1024
MAX_FILE=4*1024*1024
NAMESPACES={"between-deliveries-red-ledger":"red-ledger","breakwater-battery":"breakwater","omega-front":"omega-front","gumball-armada":"gumball-armada","ministry-lantern-desk":"lantern-desk","iron-sight-forward-detachment":"forward-detachment","procedural-test-chamber":"test-chamber",**{k:k for k in ("classic-chess","classic-go","classic-checkers","classic-pong","classic-snake","classic-reversi","classic-breakout","classic-chutes-ladders")}}
EXTENSIONS=(".rlprop",".png",".ogg",".wav",".mp3",".json",".rvdata",".atlas")

def validate(path):
    if not path.is_file() or not 22<=path.stat().st_size<=64*1024*1024:
        raise ValueError("Missing or oversized content pack")
    with zipfile.ZipFile(path) as z:
        names=z.namelist()
        if len(names)>MAX_ENTRIES or len(names)<2 or len(names)!=len(set(names)) or "module.json" not in names:
            raise ValueError("Invalid or duplicate ZIP entries")
        info=z.getinfo("module.json")
        if info.file_size>16384:
            raise ValueError("Oversized module.json")
        manifest=json.loads(z.read("module.json"))
        if not isinstance(manifest,dict) or manifest.get("schema")!=1 or manifest.get("kind")!="content":
            raise ValueError("Invalid content schema")
        game=manifest.get("id")
        if game not in NAMESPACES: raise ValueError("Unapproved game ID")
        rev=manifest.get("revision")
        if not isinstance(rev,int) or not 1<=rev<=2147483647:
            raise ValueError("Invalid content revision")
        prefix="assets/native/"+NAMESPACES[game]+"/"
        total=0;assets=0;triangles=0;actual={}
        for entry in z.infolist():
            name=entry.filename
            if (name.startswith("/") or ".." in name or "\\" in name or ":" in name or
                "//" in name or not re.fullmatch(r"[A-Za-z0-9_.\-/]+",name)):
                raise ValueError("Unsafe ZIP name: "+name)
            if entry.is_dir():continue
            if entry.file_size>MAX_FILE:
                raise ValueError("Oversized ZIP entry: "+name)
            total+=entry.file_size
            if total>MAX_UNCOMPRESSED:raise ValueError("Expanded content too large")
            if name=="module.json" or name.startswith("licenses/"):
                if name.startswith("licenses/") and not name.endswith((".txt",".json")):
                    raise ValueError("Unrecognized license notice")
                continue
            if not name.startswith(prefix) or not name.endswith(EXTENSIONS):
                raise ValueError("Unapproved game asset: "+name)
            assets+=1
            data=z.read(entry)
            actual[name]={"bytes":len(data),"sha256":hashlib.sha256(data).hexdigest()}
            if name.endswith(".rlprop"):
                if len(data)<16:raise ValueError("Truncated RLBP mesh")
                magic,version,vertices,atlas=struct.unpack("<4sIII",data[:16])
                if ((magic,version,atlas)!=(b"RLBP",1,256) or vertices==0 or
                    vertices>50000 or vertices%3 or
                    len(data)!=16+vertices*40+256*256*2):
                    raise ValueError("Invalid RLBP data: "+name)
                triangles+=vertices//3
        records=manifest.get("files",manifest.get("models"))
        if not isinstance(records,list) or len(records)!=assets:
            raise ValueError("Pack manifest must enumerate every game asset")
        declared={}
        for row in records:
            if not isinstance(row,dict) or not isinstance(row.get("path"),str):
                raise ValueError("Invalid asset manifest entry")
            name=row["path"]
            if name in declared or name not in actual:
                raise ValueError("Duplicate or undeclared game asset")
            if (row.get("bytes")!=actual[name]["bytes"] or
                row.get("sha256")!=actual[name]["sha256"]):
                raise ValueError("Game asset size/hash mismatch: "+name)
            declared[name]=row
        if set(declared)!=set(actual):raise ValueError("Hidden game asset in ZIP")
        if not assets:raise ValueError("Pack has no playable assets")
        if z.testzip() is not None:raise ValueError("Corrupted ZIP checksum")
    return {"game":game,"revision":rev,"assets":assets,"triangles":triangles,"bytes":path.stat().st_size,"sha256":hashlib.sha256(path.read_bytes()).hexdigest()}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("pack",type=Path)
    args=p.parse_args()
    try:print(json.dumps(validate(args.pack),sort_keys=True,indent=2))
    except (ValueError,OSError,zipfile.BadZipFile,json.JSONDecodeError,struct.error) as error:
        print("REJECTED:",error,file=sys.stderr);raise SystemExit(1)

if __name__=="__main__":main()
