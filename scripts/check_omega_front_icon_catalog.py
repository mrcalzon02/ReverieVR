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
    accepted = json.loads(
        (ROOT / "art/omega-front/accepted/catalog.json").read_text())
    entries = accepted["sets"]
    if len(entries) != 5 or {entry["faction_id"].lower() for entry in entries} != set(factions):
        raise ValueError("Archived accepted-faction catalog differs from runtime catalog")
    for entry in entries:
        faction = entry["faction_id"].lower()
        if factions[faction] != entry["expected_count"]:
            raise ValueError(f"Wrong accepted icon count for {faction}")
        if not (ROOT / entry["atlas_path"]).is_file() or not (ROOT / entry["manifest"]).is_file():
            raise ValueError(f"Missing accepted atlas or manifest for {faction}")
        if not all(path.startswith(entry["icons_path"])
                   for path in paths if f"/{faction}/" in path):
            raise ValueError(f"Wrong asset directory for {faction}")
    runtime_header = (
        ROOT / "app/src/main/jni/native/omega_front_portrait_catalog.h"
    ).read_text()
    for icon in c["icons"]:
        path = icon["path"].removeprefix("app/src/main/assets/")
        pair = '{"' + icon["role"] + '","' + path + '"}'
        if pair not in runtime_header:
            raise ValueError(f"Runtime portrait mapping drifted: {icon['role']}")
    for atlas in c["atlases"]:
        check_png(atlas["path"], (1280, 1024))
    check_png(c["style_guide"], (1536, 1024))
    print("Omega Front artwork OK: 100 square PNGs; 5 atlases; 1 style guide")

if __name__ == "__main__":
    main()
