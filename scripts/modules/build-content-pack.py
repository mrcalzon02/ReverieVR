#!/usr/bin/env python3
"""Deterministic, allowlisted RV content-only module archive builder.

Reads game-selected files already present locally (including Beckenburg
derivatives); never downloads or selects a full shared asset library.
No GitHub Actions, source archive, executable code or local key handling.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

NAMESPACES = {
    "between-deliveries-red-ledger": "red-ledger",
    "breakwater-battery": "breakwater",
    "omega-front": "omega-front",
    "gumball-armada": "gumball-armada",
    "ministry-lantern-desk": "lantern-desk",
    "iron-sight-forward-detachment": "forward-detachment",
}
EXTENSIONS = {".rlprop", ".png", ".ogg", ".wav", ".mp3",
              ".json", ".rvdata", ".atlas"}
MAX_FILE = 4 * 1024 * 1024
MAX_PACKED = 64 * 1024 * 1024
MAX_ENTRIES = 512

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
        if f.suffix == ".rlprop":
            if len(data) < 16:
                raise ValueError("Truncated room mesh: " + relative)
            magic, version, vertices, side = struct.unpack_from("<4sIII", data)
            if (magic, version, side) != (b"RLBP", 1, 256):
                raise ValueError("Unknown room mesh format: " + relative)
            if vertices <= 0 or vertices > 50_000 or vertices % 3:
                raise ValueError("Invalid triangle count: " + relative)
            if len(data) != 16 + vertices * 40 + 256 * 256 * 2:
                raise ValueError("Room mesh has wrong byte count: " + relative)
        # Red Ledger's native renderer reads the approved Beckenburg
        # flat directory, not top-level red-ledger/*.rlprop paths.
        if namespace == "red-ledger" and f.suffix == ".rlprop":
            relative = "beckenburg/" + relative
        items.append(("assets/native/" + namespace + "/" + relative, data))
    return items

def zip_write(z, path, payload):
    info = zipfile.ZipInfo(path, date_time=(2026, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    z.writestr(info, payload, compresslevel=9)

def build(module, source, output, revision, version):
    namespace = NAMESPACES.get(module)
    if namespace is None:
        if module.startswith("classic-") and module[8:].replace("-", "").isalnum():
            namespace = module
        else:
            raise ValueError("Module is not approved")
    if revision < 1 or revision > 2147483647:
        raise ValueError("Invalid revision")
    assets = checked_assets(source, namespace)
    manifest = {
        "schema": 1, "id": module, "kind": "content",
        "revision": revision, "version": version,
        "native_abi_min": 1, "native_abi_max": 1,
        "files": [{"path": p, "bytes": len(b), "sha256": sha(b)}
                  for p, b in assets]
    }
    provenance = {"source": "Approved, owner-curated game assets",
                  "game": module, "count": len(assets)}
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w") as z:
        zip_write(z, "module.json",
                  json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode())
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
