#!/usr/bin/env python3
"""Deterministic, allowlisted RV content-only module archive builder.

Reads game-selected files already present locally (including Beckenburg
derivatives); never downloads or selects a full shared asset library.
No GitHub Actions, source archive, executable code or local key handling.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

MATRIX=Path(__file__).resolve().parents[2]/"modules/game-identity-matrix.json"
NAMESPACES={
    entry["id"]:entry["content_namespace"]
    for entry in json.loads(MATRIX.read_text("utf-8"))["games"]
}

OWNED_BAR_FBX = ["owned_ashtray.fbx","owned_cigar.fbx","owned_cigar_box_open.fbx","owned_cigar_box_closed.fbx"]
EXTENSIONS = {".rlprop", ".bwm", ".fbx", ".png", ".ogg", ".wav", ".mp3",
              ".json", ".rvdata", ".atlas"}
MAX_FILE = 4 * 1024 * 1024
MAX_PACKED = 64 * 1024 * 1024
MAX_ENTRIES = 512
MAX_MANIFEST = 64 * 1024

def sha(data):
    return hashlib.sha256(data).hexdigest()

def checked_assets(directory, namespace):
    files = sorted(path for path in directory.rglob("*") if path.is_file())
    if not files or len(files) > MAX_ENTRIES - 2:
        raise ValueError("Selected directory has zero or too many files")
    items = []
    for f in files:
        relative = f.relative_to(directory).as_posix()
        if f.is_symlink() or ".." in relative or not all(
                c.isascii() and (c.isalnum() or c in "._-/")
                for c in relative):
            raise ValueError("Invalid path: " + relative)
        if f.suffix not in EXTENSIONS:
            raise ValueError("Unsupported content extension: " + relative)
        data = f.read_bytes()
        if not data or len(data) > MAX_FILE:
            raise ValueError("Invalid file size: " + relative)
        if f.suffix == ".fbx":
            if namespace!="red-ledger" or relative not in {
                    "models/"+name for name in OWNED_BAR_FBX}:
                raise ValueError("Unapproved owner bar FBX: "+relative)
            if not 100<=len(data)<=100000 or not data.startswith(
                    b"Kaydara FBX Binary  ") or struct.unpack_from(
                    "<I",data,23)[0]!=7400:
                raise ValueError("Invalid owner model FBX 7.4: "+relative)
        if f.suffix == ".rlprop":
            if namespace != "red-ledger":
                raise ValueError("RLBP meshes belong only to Red Ledger")
            if "/" in relative and (not relative.startswith("beckenburg/") or
                                    "/" in relative[len("beckenburg/"):]):
                raise ValueError("RLBP paths must be flat Beckenburg entries")
            if len(data) < 16:
                raise ValueError("Truncated room mesh: " + relative)
            magic, version, vertices, side = struct.unpack_from("<4sIII", data)
            if (magic, version, side) != (b"RLBP", 1, 256):
                raise ValueError("Unknown room mesh format: " + relative)
            if vertices <= 0 or vertices > 50_000 or vertices % 3:
                raise ValueError("Invalid triangle count: " + relative)
            if len(data) != 16 + vertices * 40 + 256 * 256 * 2:
                raise ValueError("Room mesh has wrong byte count: " + relative)
            for row in struct.iter_unpack("<10f", memoryview(data)[16:16+vertices*40]):
                if not all(math.isfinite(value) for value in row[:8]):
                    raise ValueError("Nonfinite room mesh vertex: " + relative)
                if not all(0.0 <= value <= 1.0 for value in row[3:8]):
                    raise ValueError("Room mesh has invalid UV/color: " + relative)
        if f.suffix == ".bwm":
            if namespace != "breakwater" or not relative.startswith("beckenburg/"):
                raise ValueError("BWM1 meshes belong only to Breakwater Beckenburg")
            if len(data)<35 or len(data)>8+2048*27 or data[:4]!=b"BWM1":
                raise ValueError("Invalid BWM1 model header")
            triangles=struct.unpack_from("<I",data,4)[0]
            if not 1<=triangles<=2048 or len(data)!=8+triangles*27:
                raise ValueError("Invalid BWM1 model triangle count")
            for at in range(8,len(data),9):
                xyz=struct.unpack_from("<hhh",data,at)
                if any(value<=-32000 or value>=32000 for value in xyz):
                    raise ValueError("Invalid BWM1 mesh coordinates")
        # Red Ledger's native renderer reads the approved Beckenburg
        # flat directory, not top-level red-ledger/*.rlprop paths.
        if (namespace == "red-ledger" and f.suffix == ".rlprop" and
                not relative.startswith("beckenburg/")):
            relative = "beckenburg/" + relative
        name="assets/native/" + namespace + "/" + relative
        if len(name)>160 or any(previous==name for previous,_ in items):
            raise ValueError("Duplicate or oversized game asset path: "+name)
        items.append((name,data))
    return items

def zip_write(z, path, payload):
    info = zipfile.ZipInfo(path, date_time=(2026, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    z.writestr(info, payload, compresslevel=9)

def build(module, source, output, revision, version, *, provenance=None):
    namespace = NAMESPACES.get(module)
    if namespace is None:
        raise ValueError("Module is not in the authoritative game registry")
    if revision < 1 or revision > 2147483647:
        raise ValueError("Invalid revision")
    if not __import__("re").fullmatch(
        r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.-]+)?",version):
        raise ValueError("Invalid semantic game content version")
    assets = checked_assets(source, namespace)
    manifest = {
        "schema": 1, "id": module, "kind": "content",
        "revision": revision, "version": version,
        "native_abi_min": 1, "native_abi_max": 1,
        "files": [{"path": p, "bytes": len(b), "sha256": sha(b)}
                  for p, b in assets]
    }
    if provenance is None:
        provenance = {"source": "Approved, owner-curated game assets",
                      "game": module, "count": len(assets)}
    if not isinstance(provenance, dict) or provenance.get("game") != module or \
       provenance.get("count") != len(assets):
        raise ValueError("Provenance must bind the selected game and asset count")
    manifest_bytes = json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode()
    if len(manifest_bytes)>MAX_MANIFEST:
        raise ValueError("Game content manifest exceeds bounded Android limit")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w") as z:
        zip_write(z, "module.json",manifest_bytes)
        for p, b in assets:
            zip_write(z, p, b)
        zip_write(z, "licenses/asset-provenance.json",
                  json.dumps(provenance, sort_keys=True).encode())
    if output.stat().st_size > MAX_PACKED:
        output.unlink()
        raise ValueError("Result exceeds the Android installer limit")
    with zipfile.ZipFile(output) as z:
        if z.testzip() is not None or len(z.namelist()) != len(assets) + 2:
            raise ValueError("Invalid post-build ZIP contents")
    return {"path": str(output), "id": module,
            "revision": revision, "bytes": output.stat().st_size,
            "sha256": sha(output.read_bytes()), "assets": len(assets)}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--id", required=True)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--revision", required=True, type=int)
    parser.add_argument("--version", required=True)
    opts = parser.parse_args()
    print(json.dumps(build(opts.id, opts.source, opts.output,
                           opts.revision, opts.version), indent=2))

if __name__ == "__main__":
    main()
