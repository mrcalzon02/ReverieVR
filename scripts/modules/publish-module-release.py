#!/usr/bin/env python3
"""Release one independently versioned game payload from verified ReverieVR main.

Operator CLI; uses locally authenticated GitHub CLI (gh), never GitHub Actions.
Draft -> upload -> remote download/hash readback -> (optional) publish.
It does NOT sign a catalog or fabricate a device acceptance result.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time

REPO="mrcalzon02/ReverieVR"
TAG=re.compile(r"^module-[a-z0-9-]+-v[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.-]+)?$")
TRANSIENT=("timeout","timed out","rate limit","429","502","503","504",
           "connection reset","connection refused","temporarily unavailable")

def run(args, retries=3):
    for attempt in range(retries):
        command=subprocess.run(args,stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,text=True)
        if command.returncode==0:return command.stdout.strip()
        error=command.stderr.strip()
        retry=any(x in error.lower() for x in TRANSIENT)
        if retry and attempt+1<retries:
            time.sleep(min(2**attempt,8))
            continue
        raise RuntimeError("Command rejected without verified acceptance: "+
                           " ".join(args[:5])+" — "+error[:1000])
    raise RuntimeError("Unreachable")

def inspect_release(tag):
    try:
        parsed=json.loads(run(["gh","release","view",tag,"--repo",REPO,
                               "--json","tagName,isDraft,assets,url"],1))
        if parsed.get("tagName")!=tag:
            raise RuntimeError("Provider returned a different tag")
        return parsed
    except RuntimeError as e:
        if "release not found" in str(e).lower() or "http 404" in str(e).lower():
            return None
        raise

def digest(path):
    sha=hashlib.sha256()
    with path.open("rb") as handle:
        for data in iter(lambda:handle.read(65536),b""):sha.update(data)
    return sha.hexdigest()

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--tag",required=True)
    p.add_argument("--file",type=Path,required=True)
    p.add_argument("--title",required=True)
    p.add_argument("--notes",required=True,help="Versioned changelog text")
    p.add_argument("--publish",action="store_true",
                   help="Publish after remote byte-for-byte verification")
    args=p.parse_args()
    if not TAG.fullmatch(args.tag):
        raise ValueError("Expected versioned module-<slug>-vMAJOR.MINOR.PATCH tag")
    artifact=args.file.resolve()
    if not artifact.is_file() or artifact.suffix not in (".rvpack",".apk"):
        raise ValueError("Selected game package missing or invalid type")
    size=artifact.stat().st_size
    if not 22<=size<=512*1024*1024:
        raise ValueError("Package outside supported size")
    local_hash=digest(artifact)
    if run(["git","branch","--show-current"],1)!="main":
        raise ValueError("Only authoritative main can publish game releases")
    if run(["git","status","--porcelain"],1):
        raise ValueError("Commit and verify source before publishing")
    local_head=run(["git","rev-parse","HEAD"],1)
    remote_head=run(["gh","api",f"repos/{REPO}/branches/main",
                     "--jq",".commit.sha"],3)
    if local_head!=remote_head:
        raise ValueError("Local source HEAD differs from accepted GitHub main")
    release=inspect_release(args.tag)
    if release is None:
        run(["gh","release","create",args.tag,"--repo",REPO,
             "--target",remote_head,"--title",args.title,
             "--notes",args.notes,"--draft"],3)
        release=inspect_release(args.tag)
    if release is None or not release.get("isDraft"):
        raise ValueError("Expected verified draft release, not mutable published asset")
    existing=[a for a in release.get("assets",[])
              if a.get("name")==artifact.name]
    if len(existing)>1:raise ValueError("Duplicate asset name in release")
    if not existing:
        run(["gh","release","upload",args.tag,str(artifact),"--repo",REPO],3)
    release=inspect_release(args.tag)
    if release is None or not any(a.get("name")==artifact.name and
                                  a.get("size")==size
                                  for a in release.get("assets",[])):
        raise RuntimeError("Remote release did not acknowledge expected asset")
    with tempfile.TemporaryDirectory(prefix="reverie-game-readback-") as temp:
        run(["gh","release","download",args.tag,"--repo",REPO,
             "--pattern",artifact.name,"--dir",temp],3)
        remote=Path(temp)/artifact.name
        if not remote.is_file() or remote.stat().st_size!=size or \
                digest(remote)!=local_hash:
            raise RuntimeError("Remote release asset bytes FAILED SHA-256 readback")
    if args.publish:
        run(["gh","release","edit",args.tag,"--repo",REPO,"--draft=false"],3)
        release=inspect_release(args.tag)
        if release is None or release.get("isDraft"):
            raise RuntimeError("Release publication not accepted remotely")
    receipt={
        "repository":REPO,"tag":args.tag,"name":artifact.name,
        "size_bytes":size,"sha256":local_hash,
        "source_revision":remote_head,
        "draft":release["isDraft"],"release_url":release["url"],
        "catalog_ready":not release["isDraft"],
        "device_tested":False
    }
    print(json.dumps(receipt,sort_keys=True,indent=2))
    print("Release verified; sign/publish a higher catalog revision separately.",
          file=sys.stderr)

if __name__=="__main__":
    try:main()
    except (ValueError,RuntimeError,OSError) as exc:
        print("RELEASE BLOCKED:",exc,file=sys.stderr)
        raise SystemExit(1)
