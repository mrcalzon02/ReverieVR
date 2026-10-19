#!/usr/bin/env python3
"""Offline publisher tool for independent ReverieVR game-content releases.

The signing PRIVATE key must be generated and kept outside the repository.
No GitHub Actions; upload signed catalog after release assets exist remotely.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from urllib.parse import urlsplit

RELEASE_PREFIX = "/mrcalzon02/ReverieVR/releases/download/"
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
SEMVER_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.-]+)?$")
MATRIX=Path(__file__).resolve().parents[2]/"modules/game-identity-matrix.json"

def registered_games():
    return {record["id"]:record for record in
            json.loads(MATRIX.read_text("utf-8"))["games"]}

def call(*arguments):
    subprocess.run(["openssl",*map(str,arguments)],check=True)

def init(private,public):
    if private.exists() or public.exists():
        raise SystemExit("Refusing to overwrite a signing key")
    private.parent.mkdir(parents=True,exist_ok=True)
    public.parent.mkdir(parents=True,exist_ok=True)
    # P-256 ECDSA, compatible with Android API 26 SHA256withECDSA.
    call("ecparam","-name","prime256v1","-genkey","-noout","-out",private)
    os.chmod(private,0o600)
    call("pkey","-in",private,"-pubout","-outform","DER","-out",public)
    print(json.dumps({"private_key":str(private),"public_der":str(public),
                      "public_sha256":hashlib.sha256(public.read_bytes()).hexdigest(),
                      "note":"Keep private key offline, NEVER in GitHub or the APK."},indent=2))

def approved_release(entry):
    required={"id","kind","channel","version","revision","size_bytes",
              "sha256","url","min_hub","min_android_sdk","native_abi_min",
              "native_abi_max","gles_major","abis"}
    if not required.issubset(entry):
        raise ValueError("Release is missing required fields")
    if entry["kind"] not in ("content","companion-apk"):
        raise ValueError("Invalid release kind")
    if entry["channel"] not in ("development","testing","stable"):
        raise ValueError("Invalid channel")
    if not SEMVER_RE.fullmatch(str(entry["version"])):
        raise ValueError("Invalid version")
    if not isinstance(entry["revision"],int) or not 0<entry["revision"]<=2147483647:
        raise ValueError("Invalid revision")
    if not isinstance(entry["size_bytes"],int) or not 22<=entry["size_bytes"]<=512*1024*1024:
        raise ValueError("Invalid release size")
    if not SHA_RE.fullmatch(str(entry["sha256"])):
        raise ValueError("Invalid SHA-256")
    if not SEMVER_RE.fullmatch(str(entry["min_hub"])):
        raise ValueError("Invalid minimum ReverieVR hub version")
    for number in ("min_android_sdk","native_abi_min","native_abi_max","gles_major"):
        if not isinstance(entry[number],int):
            raise ValueError("Non-integer compatibility value")
    if not (26 <= entry["min_android_sdk"] <= 100 and
            1 <= entry["native_abi_min"] <= entry["native_abi_max"] <= 32 and
            2 <= entry["gles_major"] <= 4):
        raise ValueError("Unsupported game compatibility requirements")
    abis=entry["abis"]
    if (not isinstance(abis,list) or not 1 <= len(abis) <= 2 or
        len(set(abis))!=len(abis) or
        any(abi not in ("armeabi-v7a","arm64-v8a") for abi in abis)):
        raise ValueError("Unsupported native ABI selection")
    url=urlsplit(entry["url"])
    if (url.scheme!="https" or url.hostname!="github.com" or
        url.port is not None or url.query or url.fragment or
        not url.path.startswith(RELEASE_PREFIX) or
        ".." in url.path or "//" in url.path):
        raise ValueError("Not a pinned ReverieVR GitHub Release asset")
    suffix=".rvpack" if entry["kind"]=="content" else ".apk"
    if not url.path.endswith(suffix):
        raise ValueError("Release URL type mismatch")
    game=registered_games().get(entry["id"])
    if game is None:
        raise ValueError("Catalog contains an unregistered game ID")
    if entry["kind"]=="companion-apk" and game["companion_build_target"] is None:
        raise ValueError("Catalog advertises a game APK with no proven build target")
    # Publisher and Android must agree on which immutable release owns each
    # asset. A well-formed GitHub URL pointing to ANOTHER game is not enough.
    remainder=url.path[len(RELEASE_PREFIX):]
    pieces=remainder.split("/")
    tag="module-"+entry["id"]+"-v"+entry["version"]
    if len(pieces)!=2 or pieces[0]!=tag or not pieces[1]:
        raise ValueError("Catalog URL names wrong game or version release tag")
    return entry

def sign(source,catalog,signature,private):
    if not private.is_file():
        raise ValueError("Offline signing key is missing")
    proposed=json.loads(source.read_text(encoding="utf-8"))
    if proposed.get("schema")!=1 or not isinstance(proposed.get("catalog_revision"),int):
        raise ValueError("Unexpected catalog schema or revision")
    if proposed["catalog_revision"]<=0:
        raise ValueError("Catalog revision must be monotonic and positive")
    modules=proposed.get("modules")
    if not isinstance(modules,list) or len(modules)>128:
        raise ValueError("Invalid release list")
    seen=set()
    for entry in modules:
        approved_release(entry)
        key=(entry["id"],entry["kind"],entry["channel"],entry["revision"])
        if key in seen:raise ValueError("Duplicate game release revision")
        seen.add(key)
    if catalog.exists():
        previous=json.loads(catalog.read_text("utf-8"))
        if proposed["catalog_revision"]<=int(previous.get("catalog_revision",0)):
            raise ValueError("Catalog revision must exceed published local revision")
    # One immutable exact JSON byte sequence for the ECDSA signature.
    raw=(json.dumps(proposed,sort_keys=True,separators=(",",":"))+"\n").encode("utf-8")
    if len(raw)>256*1024:
        raise ValueError("Oversized signed catalog")
    catalog.parent.mkdir(parents=True,exist_ok=True)
    signature.parent.mkdir(parents=True,exist_ok=True)
    catalog.write_bytes(raw)
    call("dgst","-sha256","-sign",private,"-out",signature,catalog)
    print(json.dumps({"catalog":str(catalog),"signature":str(signature),
       "sha256":hashlib.sha256(raw).hexdigest(),"entries":len(modules)},indent=2))

def verify(catalog,signature,public_der):
    if not all(x.is_file() for x in (catalog,signature,public_der)):
        raise ValueError("Catalog, signature or trusted public key missing")
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        pem=Path(d)/"pub.pem"
        call("pkey","-pubin","-inform","DER","-in",public_der,"-pubout","-out",pem)
        call("dgst","-sha256","-verify",pem,"-signature",signature,catalog)
    print("Publisher signature verified.")

def main():
    p=argparse.ArgumentParser()
    subs=p.add_subparsers(dest="mode",required=True)
    i=subs.add_parser("init")
    i.add_argument("--private",required=True,type=Path)
    i.add_argument("--public-der",required=True,type=Path)
    s=subs.add_parser("sign")
    s.add_argument("--source",required=True,type=Path)
    s.add_argument("--catalog",required=True,type=Path)
    s.add_argument("--signature",required=True,type=Path)
    s.add_argument("--private",required=True,type=Path)
    v=subs.add_parser("verify")
    v.add_argument("--catalog",required=True,type=Path)
    v.add_argument("--signature",required=True,type=Path)
    v.add_argument("--public-der",required=True,type=Path)
    args=p.parse_args()
    if args.mode=="init":init(args.private,args.public_der)
    elif args.mode=="sign":sign(args.source,args.catalog,args.signature,args.private)
    else:verify(args.catalog,args.signature,args.public_der)

if __name__=="__main__":
    main()
