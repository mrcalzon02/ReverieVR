#!/usr/bin/env python3
"""Fail-closed game/revision/certificate binding before any GitHub upload.

This is strictly a publisher preflight: the existing content validator,
APK signature verification, GitHub remote readback and signed release
catalog remain separate required acceptance checks.
"""
import json
from pathlib import Path
import re
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[2]
MATRIX=ROOT/"modules/game-identity-matrix.json"
SEMVER=r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.-]+)?"
TAG=re.compile(r"module-([a-z0-9-]+)-v("+SEMVER+r")\Z")
SHA256=re.compile(r"[a-fA-F0-9]{64}\Z")

def pairs_without_duplicates(rows):
    d={}
    for key,value in rows:
        if key in d:raise ValueError("Duplicate manifest field "+key)
        d[key]=value
    return d

def read_process(command):
    p=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                     text=True)
    if p.returncode:
        raise ValueError("Required Android package verification failed: "+
                         p.stderr.strip()[:350])
    return p.stdout

def validate(tag,artifact,publisher_cert_sha256=None,reader=read_process):
    match=TAG.fullmatch(tag)
    if not match:raise ValueError("Tag must use canonical module-GAME-ID-vVERSION")
    game,version=match.groups()
    matrix=json.loads(MATRIX.read_text(encoding="utf-8"))
    by_id={x["id"]:x for x in matrix["games"]}
    if game not in by_id:
        raise ValueError("Release tag does not name a registered game")
    info=by_id[game]
    if not artifact.is_file():raise ValueError("Missing actual package payload")
    if artifact.suffix==".rvpack":
        with zipfile.ZipFile(artifact) as archive:
            members=archive.namelist()
            if members.count("module.json")!=1:
                raise ValueError("Game manifest absent or duplicated")
            metadata=archive.getinfo("module.json")
            if metadata.file_size<2 or metadata.file_size>16384:
                raise ValueError("Invalid manifest size")
            data=json.loads(archive.read("module.json"),
                            object_pairs_hook=pairs_without_duplicates)
        if (data.get("id")!=game or data.get("version")!=version or
            data.get("schema")!=1 or data.get("kind")!="content"):
            raise ValueError("Release tag disagrees with content manifest game/version")
        revision=data.get("revision")
        if type(revision)!=int or not 1<=revision<=2147483647:
            raise ValueError("Invalid content revision")
        return {"game":game,"version":version,"revision":revision,"kind":"content"}
    if artifact.suffix!=".apk":
        raise ValueError("Unsupported game release payload type")
    if info["companion_build_target"] is None:
        raise ValueError("Game has no proven companion APK target")
    if not publisher_cert_sha256 or not SHA256.fullmatch(publisher_cert_sha256):
        raise ValueError("A real trusted hub publisher certificate digest is required")
    # aapt reads binary AndroidManifest.xml. Never treat filename, title,
    # ZIP entries or untrusted sidecar JSON as APK identity evidence.
    metadata=reader(["aapt","dump","badging",str(artifact)])
    found=re.search(r"^package:\s+name='([^']+)'\s+"
                    r"versionCode='([0-9]+)'\s+versionName='([^']+)'",
                    metadata,re.MULTILINE)
    if not found:
        raise ValueError("Could not inspect Android package identity")
    package,code,actual_version=found.groups()
    if package!=info["companion_package"] or actual_version!=version or int(code)<1:
        raise ValueError("Release tag disagrees with signed companion APK identity/version")
    signer=reader(["apksigner","verify","--print-certs",str(artifact)])
    values=re.findall(r"Signer #\d+ certificate SHA-256 digest:\s*([0-9a-fA-F]{64})",
                      signer)
    if len(values)!=1 or values[0].lower()!=publisher_cert_sha256.lower():
        raise ValueError("APK signer differs from required ReverieVR hub certificate")
    return {"game":game,"version":version,"revision":int(code),
            "kind":"companion-apk","package":package,
            "publisher_cert_sha256":values[0].lower()}
