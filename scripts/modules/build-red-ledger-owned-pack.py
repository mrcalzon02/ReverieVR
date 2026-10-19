#!/usr/bin/env python3
"""Build the six genuinely used Red Ledger owner-made bar/patron assets.

This is a scoped game-content update, not a re-publication of Models.zip,
and does not claim to contain the older fourteen private PSX room props.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
GAME="between-deliveries-red-ledger"
FILES=(
    "models/owned_ashtray.fbx",
    "models/owned_cigar.fbx",
    "models/owned_cigar_box_open.fbx",
    "models/owned_cigar_box_closed.fbx",
    "beckenburg/patron_gopnick.rlprop",
    "beckenburg/patron_pink_dress.rlprop",
)
SHA256={
    "models/owned_ashtray.fbx":"def20e20cea13e74c83b34fe62f002fd9e9118021968459048de3cf8952924eb",
    "models/owned_cigar.fbx":"7d52a70086f4a9202c4680a976049a7db38d4172f2cae2cbb39abad32e2b4839",
    "models/owned_cigar_box_open.fbx":"586314665600af4b3bff2bc1ead8e20543f385ca37ce5580803d5e1777e41f75",
    "models/owned_cigar_box_closed.fbx":"b6b08eaa8b45a7d4aafae55e87edf3f25e41ff27115fc2706549f5bc89dd3fca",
    "beckenburg/patron_gopnick.rlprop":"cf4d3415f7e87d748c43513caf6f903203afc62af658d616be47ea197659aeb0",
    "beckenburg/patron_pink_dress.rlprop":"4e0fa322d3c8e9f4aef9c8d51cc7c0609f6fc6fe2d1aaaf98d70e32bd3a874de",
}
SOURCE_IDS={
    "models/owned_ashtray.fbx":"1YwHhFq1VU-C-lnpKSHgamVmlR4OZlycH",
    "models/owned_cigar.fbx":"1ArWVQA_YaqTccMVNwzGzGSACEN9FcM7c",
    "models/owned_cigar_box_open.fbx":"177JIJjphFwIYsGZ0gqmNuqNtkuhnJyPt",
    "models/owned_cigar_box_closed.fbx":"1iLTY7TkgZ3ynqCVjQxVCp-c3ON_Iep_m",
    "beckenburg/patron_gopnick.rlprop":"1LG8sk3hAnSxzNGzIhtXRM5jQlygdGCv9",
    "beckenburg/patron_pink_dress.rlprop":"1El0F9_NiddJo3QfhZubA5wKyYLvyPY7e",
}

def tool(filename):
    spec=importlib.util.spec_from_file_location(filename.replace("-","_"),HERE/filename)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def build(output,revision,version):
    root=ROOT/"app/src/main/assets/native/red-ledger"
    with tempfile.TemporaryDirectory(prefix="red-ledger-owner-curated-") as temp:
        stage=Path(temp)
        for entry in FILES:
            src=root/entry
            if src.is_symlink() or not src.is_file():
                raise ValueError("Missing approved owner-authored model: "+entry)
            data=src.read_bytes()
            if hashlib.sha256(data).hexdigest()!=SHA256[entry]:
                raise ValueError("Changed owner source/derived bytes; review: "+entry)
            target=stage/entry
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(src,target)
        provenance={
            "schema":1,"game":GAME,"count":len(FILES),
            "rights":"Owner-created, commercial game inclusion approved. Raw source ZIP stays in private Beckenburg Drive.",
            "source":"Models.zip extracted, six individually selected in-game files",
            "master_index":"https://docs.google.com/document/d/18G144Qd0-L5rL7NdqxMykqkXaBHtu8-ytr_yYPol6rI/edit",
            "models":[{"file":f,"source_drive_id":SOURCE_IDS[f],
                       "sha256":SHA256[f]} for f in FILES],
            "limits":"Real static patron meshes; rigs/animations and Galaxy S9 visual acceptance still pending"
        }
        result=tool("build-content-pack.py").build(
            GAME,stage,output,revision,version,provenance=provenance)
    verified=tool("verify-content-pack.py").validate(output)
    if verified["game"]!=GAME or verified["assets"]!=len(FILES) or (
        verified["sha256"]!=result["sha256"]):
        output.unlink(missing_ok=True)
        raise ValueError("Red Ledger owner content failed independent verification")
    return {"game":GAME,"assets":len(FILES),"triangles":verified["triangles"],
            "bytes":verified["bytes"],"sha256":verified["sha256"],
            "file":str(output),"status":"unsigned local content; no remote signing or headset proof"}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--revision",type=int,required=True)
    p.add_argument("--version",required=True)
    opts=p.parse_args()
    print(json.dumps(build(opts.output,opts.revision,opts.version),
                     indent=2,sort_keys=True))
if __name__=="__main__":main()
