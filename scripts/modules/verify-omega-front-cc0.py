#!/usr/bin/env python3
"""Verify actual Kenney CC0 tiles are byte-identical, packaged, and referenced."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
assets = root / "app/src/main/assets"
manifest = assets / "native/omega-front/rpg_cc0/source_catalog.json"
catalog = json.loads(manifest.read_text(encoding="utf-8"))
renderer = (root / "app/src/main/jni/native/omega_front_texture_renderer.h").read_text()
palette = (root / "app/src/main/jni/native/omega_front_cc0_palette.h").read_text()
if catalog["license"] != "CC0-1.0" or len(catalog["tiles"]) != 64 or catalog["count"] != 64:
    raise SystemExit("Omega Front CC0 manifest must declare all 64 licensed sources")
png_paths = re.findall(r'"(native/omega-front/[^"]+\.png)"', renderer)
# The original 108 slots remain ordered identically. New approved land
# materials are append-only at slots 108..120; CC0 source slots stay 44..107.
if len(png_paths) != 121 or len(set(png_paths)) != 121:
    raise SystemExit(f"Expected 121 distinct runtime sprite paths; got {len(png_paths)}")
seen = set()
for i, item in enumerate(catalog["tiles"]):
    path = item["runtime_path"]
    if path in seen or path not in png_paths or item["texture_slot"] != 44 + i:
        raise SystemExit(f"CC0 source unused, duplicate or mis-slotted: {path}")
    seen.add(path)
    blob = (assets / path).read_bytes()
    sha = hashlib.sha1(b"blob " + str(len(blob)).encode("ascii") + b"\0" + blob).hexdigest()
    if sha != item["source_blob_sha"]:
        raise SystemExit(f"CC0 source bytes do not match authoritative Git blob: {path}")
    if blob[:8] != bytes((137, 80, 78, 71, 13, 10, 26, 10)):
        raise SystemExit(f"CC0 source is not a PNG: {path}")
    if int.from_bytes(blob[16:20], "big") != 16 or int.from_bytes(blob[20:24], "big") != 16:
        raise SystemExit(f"CC0 source must be real 16x16 tile: {path}")
if "kCc0VisualTiles=64" not in palette or "GroundVariant" not in palette:
    raise SystemExit("CC0 art lacks authoritative gameplay selection")
print("PASS: 64 byte-identical Kenney CC0 RPG tiles, licensed and live in the 121-path renderer")
