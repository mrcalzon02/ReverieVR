#!/usr/bin/env python3
"""Validate the repository's Omega Front 256px menu-art assets. Stdlib only."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
CAT = ROOT / "app/src/main/assets/native/omega-front/ui/icons/catalog.json"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

def check_png(path, dimensions, digest=None):
    path = ROOT / path
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE) or len(data) < 24:
        raise ValueError(f"Invalid PNG: {path}")
    actual = struct.unpack(">II", data[16:24])
    if actual != dimensions:
        raise ValueError(f"Wrong dimensions {path}: {actual} != {dimensions}")
    if digest and hashlib.sha256(data).hexdigest() != digest:
        raise ValueError(f"Hash mismatch: {path}")

def main():
    c = json.loads(CAT.read_text())
    paths = set()
    factions = {}
    for icon in c["icons"]:
        faction = icon["faction_id"].lower()
        path = icon["path"]
        if path in paths or f"/{faction}/" not in path:
            raise ValueError(f"Duplicate or mismatched icon path: {path}")
        paths.add(path)
        factions[faction] = factions.get(faction, 0) + 1
        check_png(path, (256, 256), icon["sha256"])
    if factions != {"f06": 20, "f07": 20, "f08": 20, "f09": 20, "f10": 20}:
        raise ValueError(f"Incomplete faction icons: {factions}")
    for atlas in c["atlases"]:
        check_png(atlas["path"], (1280, 1024))
    check_png(c["style_guide"], (1536, 1024))
    print("Omega Front artwork OK: 100 square PNGs; 5 atlases; 1 style guide")

if __name__ == "__main__":
    main()
