#!/usr/bin/env python3
"""Functional, not cosmetic: assert all registered game modules boot/render.

Evidence comes from installed Android 10 JNI/GLES2 native menu capture.
No similarity threshold, golden screenshot, artistic/pixel-quality gate.
"""
import csv
import json
import struct
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("Usage: verify-installed-native-game-captures.py DIR")
out = Path(sys.argv[1])
root = Path(__file__).resolve().parents[2]
games = json.loads((root / "modules/game-identity-matrix.json").read_text())["games"]
ids = [g["id"] for g in games]
with (out / "native-menu-capture.csv").open(newline="") as handle:
    rows = list(csv.DictReader(handle))
if len(rows) != len(ids):
    raise SystemExit(f"Expected {len(ids)} actual game capture rows; got {len(rows)}")
seen = set()
for row in rows:
    game = row["module_id"]
    if game not in ids or game in seen or row["status"] != "CAPTURED":
        raise SystemExit(f"Missing/duplicate/failed native game run: {row}")
    seen.add(game)
    if int(row["eye_frame_crc32"]) <= 0 or int(row["png_bytes"]) <= 300:
        raise SystemExit(f"No actual GLES2 framebuffer evidence: {game}")
    frame = out / f"native-menu-{game}.png"
    contents = frame.read_bytes()
    if len(contents) != int(row["png_bytes"]):
        raise SystemExit(f"Captured byte-count mismatch: {game}")
    if contents[:16] != b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR":
        raise SystemExit(f"Invalid captured PNG header: {game}")
    width, height = struct.unpack(">II", contents[16:24])
    if (width, height) != (512, 512):
        raise SystemExit(f"Wrong installed GLES2 capture size: {game} {width}x{height}")
    if not contents.endswith(b"\x00\x00\x00\x00IEND\xaeB\x60\x82"):
        raise SystemExit(f"Incomplete native framebuffer capture: {game}")
    print(f"PASS native Android game: {game} 512x512 {len(contents)} bytes CRC={row['eye_frame_crc32']}")
if seen != set(ids):
    raise SystemExit(f"Uncaptured registered games: {sorted(set(ids)-seen)}")
print(f"PASS: all {len(ids)} registered native games launched and rendered genuine Android GLES2 images")
