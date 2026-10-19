#!/usr/bin/env python3
"""Prepare, NEVER sign/publish, the next catalog from an accepted release receipt.

Requires read-back evidence produced by publish-module-release.py and an
authentic signed previous catalog. No ECDSA private key is read here.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
def signer_module():
    path=Path(__file__).with_name("sign-catalog.py")
    spec=importlib.util.spec_from_file_location("reverie_candidate_signer",path)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

def prepare(receipt,previous,out,channel,min_hub,abis,
            min_android_sdk=26,native_abi=1,gles_major=2):
    signer=signer_module()
    if out.exists() or out.resolve()==previous.resolve():
        raise ValueError("Candidate already exists; refusing overwrite")
    if receipt.get("repository")!="mrcalzon02/ReverieVR":
        raise ValueError("Release receipt is from another repository")
    if receipt.get("draft") is not False or receipt.get("catalog_ready") is not True:
        raise ValueError("Release not remotely published and read back")
    game=receipt.get("game_id")
    version=receipt.get("game_version")
    tag=receipt.get("tag")
    kind=receipt.get("game_kind")
    revision=receipt.get("game_revision")
    filename=receipt.get("name")
    if (not isinstance(game,str) or not isinstance(version,str) or
        not isinstance(tag,str) or not isinstance(kind,str) or
        tag!="module-"+game+"-v"+version):
        raise ValueError("Release receipt game/version/tag mismatch")
    if not isinstance(filename,str) or not re.fullmatch(
            r"[A-Za-z0-9][A-Za-z0-9._-]{0,119}",filename):
        raise ValueError("Unsafe release artifact filename")
    if not re.fullmatch(r"[a-f0-9]{40}",str(receipt.get("source_revision",""))):
        raise ValueError("Release receipt missing verified main source commit")
    size=receipt.get("size_bytes")
    sha=receipt.get("sha256")
    if (type(revision)!=int or type(size)!=int or
        not isinstance(sha,str)):
        raise ValueError("Incomplete release identity, revision or archive hash")
    entry={
        "id":game,"kind":kind,"channel":channel,"version":version,
        "revision":revision,"size_bytes":size,"sha256":sha,
        "url":"https://github.com/mrcalzon02/ReverieVR/releases/download/"+
              tag+"/"+filename,
        "notes":"Verified independent game release from "+str(receipt["source_revision"])[:12],
        "min_hub":min_hub,"min_android_sdk":min_android_sdk,
        "native_abi_min":native_abi,"native_abi_max":native_abi,
        "gles_major":gles_major,"abis":list(abis)
    }
    signer.approved_release(entry)
    data=json.loads(previous.read_text("utf-8"))
    if data.get("schema")!=1 or type(data.get("catalog_revision"))!=int or (
        type(data.get("modules"))!=list):
        raise ValueError("Unusable previously signed catalog")
    if len(data["modules"])>=128:
        raise ValueError("Published game catalog has no remaining capacity")
    for earlier in data["modules"]:
        signer.approved_release(earlier)
        if (earlier["id"],earlier["kind"],earlier["channel"])==(
                game,kind,channel) and earlier["revision"]>=revision:
            raise ValueError("Game update revision is not newer than published entry")
    candidate={"schema":1,"catalog_revision":data["catalog_revision"]+1,
               "modules":data["modules"]+[entry]}
    if candidate["catalog_revision"]>2147483647:
        raise ValueError("Catalog revision exhausted")
    out.parent.mkdir(parents=True,exist_ok=True)
    with out.open("x",encoding="utf-8") as f:
        json.dump(candidate,f,sort_keys=True,indent=2)
        f.write("\n")
    return candidate

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--receipt",required=True,type=Path,
                   help="Verified receipt from publish-module-release.py --publish")
    p.add_argument("--base",type=Path,default=ROOT/"modules/catalog-v1.json")
    p.add_argument("--base-signature",type=Path,
                   default=ROOT/"modules/catalog-v1.sig")
    p.add_argument("--publisher-public-key",type=Path,
                   default=ROOT/"app/src/main/assets/native/modules/catalog-key.der")
    p.add_argument("--output",required=True,type=Path)
    p.add_argument("--channel",choices=("testing","stable","development"),
                   default="testing")
    p.add_argument("--min-hub",required=True)
    p.add_argument("--abis",nargs="+",default=["armeabi-v7a","arm64-v8a"])
    args=p.parse_args()
    signer=signer_module()
    # A candidate must extend the publisher-authenticated previous bytes;
    # an attacker-edited unsigned "current" catalog is not a valid base.
    signer.verify(args.base,args.base_signature,args.publisher_public_key)
    proposal=prepare(json.loads(args.receipt.read_text("utf-8")),
                     args.base,args.output,args.channel,args.min_hub,args.abis)
    print(json.dumps({"candidate":str(args.output),
                      "catalog_revision":proposal["catalog_revision"],
                      "game":proposal["modules"][-1]["id"],
                      "entries":len(proposal["modules"]),
                      "signed":False,"published":False},sort_keys=True,indent=2))
    print("Next: sign with the OFFLINE private key, verify, then commit catalog and signature.")

if __name__=="__main__":
    main()
