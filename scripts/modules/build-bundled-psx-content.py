#!/usr/bin/env python3
"""Build independent content revisions from assets already admitted to ReverieVR.

No new source downloads. Existing Breakwater BWM1 meshes and Omega Front PNG
art must be present in the checked-out repo. Build game-specific packages,
verify ZIP payload identity, and retain all originals untouched.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import tempfile
import shutil

ROOT=Path(__file__).resolve().parents[2]
TOOL_ROOT=Path(__file__).resolve().parent
BREAKWATER=(
    "ammo_box_mp_1.bwm","concrete_block_1.bwm","wooden_crate_1.bwm",
    "owner_boat_rowboat.bwm"
)
OMEGA_SHARED=(
    "ammo_box.png","concrete.png","dead_tree.png","grass.png",
    "living_tree.png","metal_barrel.png","relay_monitor.png",
    "road.png","rock.png","sand.png","steel.png","supply_crate.png"
)
GAMES={
    "breakwater-battery":("breakwater",BREAKWATER),
    "omega-front":("omega-front",OMEGA_SHARED)
}

def tool(name):
    spec=importlib.util.spec_from_file_location(name.replace("-","_"),TOOL_ROOT/name)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def source_files(game):
    if game not in GAMES:
        raise ValueError("Only shipped Breakwater or Omega content is supported")
    namespace,required=GAMES[game]
    source=ROOT/"app/src/main/assets/native"/namespace
    shared=source/"beckenburg" if game=="breakwater-battery" else source/"shared"
    missing=[name for name in required if not (shared/name).is_file()]
    if missing:
        raise ValueError("Missing checked-in game art: "+", ".join(missing))
    if game=="breakwater-battery":
        return source,[Path("beckenburg")/name for name in required]
    # Whole Omega gameplay content, not just the shared visuals: partial
    # content packs must retain their terrain, unit and catalog dependencies.
    files=sorted(p.relative_to(source) for p in source.rglob("*") if p.is_file())
    if any(p.is_symlink() for p in source.rglob("*")):
        raise ValueError("Symlinks not allowed in shipped game content")
    return source,files

def build(game,output,revision,version):
    source,selected=source_files(game)
    builder=tool("build-content-pack.py")
    verifier=tool("verify-content-pack.py")
    with tempfile.TemporaryDirectory(prefix="reverie-curated-pack-") as temp:
        stage=Path(temp)
        for rel in selected:
            dst=stage/rel
            dst.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source/rel,dst)
        provenance={
            "schema":1,"game":game,"count":len(selected),
            "source":"Selected built-in ReverieVR assets; no original PSX source archives",
            "master_index":"https://docs.google.com/document/d/18G144Qd0-L5rL7NdqxMykqkXaBHtu8-ytr_yYPol6rI/edit",
            "license_note":"Curated in-game assets per current ReverieVR source registry; source licenses stay attached to each underlying asset.",
            "files":[rel.as_posix() for rel in selected]
        }
        result=builder.build(game,stage,output,revision,version,provenance=provenance)
    verification=verifier.validate(output)
    if verification["game"]!=game or verification["assets"]!=len(selected) or \
       verification["sha256"]!=result["sha256"]:
        output.unlink(missing_ok=True)
        raise ValueError("Post-build native content mismatch")
    return {"game":game,"assets":len(selected),"triangles":verification["triangles"],
            "revision":revision,"bytes":verification["bytes"],
            "sha256":verification["sha256"],"file":str(output),
            "status":"local verified content candidate; unsigned/unpublished/uninstalled"}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--id",choices=sorted(GAMES),required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--revision",type=int,required=True)
    parser.add_argument("--version",required=True)
    args=parser.parse_args()
    print(json.dumps(build(args.id,args.output,args.revision,args.version),
                     indent=2,sort_keys=True))

if __name__=="__main__":
    main()
