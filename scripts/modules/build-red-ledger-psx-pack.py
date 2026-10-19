#!/usr/bin/env python3
"""Package exactly the fourteen privately converted Red Ledger PSX interiors.

Never downloads or republishes Beckenburg source packs. The operator supplies
the selected game-ready .rlprop files in a PRIVATE local input directory.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

HERE=Path(__file__).resolve().parent
GAME="between-deliveries-red-ledger"
SOURCE_INDEX="https://docs.google.com/document/d/18G144Qd0-L5rL7NdqxMykqkXaBHtu8-ytr_yYPol6rI/edit"
EXPECTED=(
    "switch","lamp_off","door","shelf","chair","wall","floor",
    "window","crate","barrel","table","mattress","lamp_on","coffee_table"
)

def tool(filename):
    spec=importlib.util.spec_from_file_location(filename.replace("-","_"),HERE/filename)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def selected_files(source):
    if not source.is_dir() or source.is_symlink():
        raise ValueError("Private model input must be an existing directory")
    files=[p for p in source.rglob("*") if p.is_file() or p.is_symlink()]
    actual=set()
    selected=[]
    for path in files:
        if path.is_symlink():
            raise ValueError("Symlinked private input refused: "+path.name)
        relative=path.relative_to(source).as_posix()
        if relative.startswith("beckenburg/"):
            relative=relative[len("beckenburg/"):]
        if "/" in relative or not relative.endswith(".rlprop"):
            raise ValueError("Unexpected private source file: "+str(path))
        if relative in actual:
            raise ValueError("Duplicate curated name: "+relative)
        actual.add(relative)
        selected.append((relative,path))
    expected={name+".rlprop" for name in EXPECTED}
    missing=expected-actual
    extra=actual-expected
    if missing or extra:
        raise ValueError("Curated pack mismatch; missing="+",".join(sorted(missing))+
                         " extra="+",".join(sorted(extra)))
    return sorted(selected)

def build(source,output,revision,version):
    chosen=selected_files(source)
    builder=tool("build-content-pack.py")
    verifier=tool("verify-content-pack.py")
    original={
        "schema":1,"game":GAME,"count":len(chosen),
        "source":"Beckenburg owner-purchased PSX Mega Pack (selected game-use derivatives)",
        "license":"Commercial game use authorized by asset owner; source pack not freely redistributable",
        "master_index":SOURCE_INDEX,
        "version_note":"Expanded folder preview v2.9 and separate ZIP v3.2.1 are NOT provenance-equated",
        "files":[{"name":name,
                  "derived_bytes":path.stat().st_size,
                  "derived_sha256":hashlib.sha256(path.read_bytes()).hexdigest()}
                 for name,path in chosen]
    }
    result=builder.build(GAME,source,output,revision,version,provenance=original)
    verified=verifier.validate(output)
    if verified["game"]!=GAME or verified["assets"]!=len(EXPECTED) or \
       verified["sha256"]!=result["sha256"]:
        output.unlink(missing_ok=True)
        raise ValueError("Post-build Red Ledger asset manifest mismatch")
    return {"game":GAME,"models":verified["assets"],"triangles":verified["triangles"],
            "revision":revision,"sha256":verified["sha256"],
            "bytes":verified["bytes"],"file":str(output),
            "status":"local artifact only; unsigned, unreleased, not S9-verified"}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source",required=True,type=Path,
                        help="PRIVATE directory holding exactly the 14 converted RLBP meshes")
    parser.add_argument("--output",required=True,type=Path)
    parser.add_argument("--revision",type=int,required=True)
    parser.add_argument("--version",required=True)
    args=parser.parse_args()
    if args.output.resolve().is_relative_to(args.source.resolve()):
        parser.error("Output cannot be written inside the source directory")
    print(json.dumps(build(args.source,args.output,args.revision,args.version),
                     indent=2,sort_keys=True))

if __name__=="__main__":
    main()
