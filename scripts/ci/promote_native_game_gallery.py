#!/usr/bin/env python3
"""Promote only genuine installed Android GLES2 game menu captures to the archive."""
import csv
import hashlib
import io
import json
import sys
from pathlib import Path

from PIL import Image, ImageOps, ImageStat

ROOT = Path(__file__).resolve().parents[2]
FEATURED = [
    "breakwater-battery", "between-deliveries-red-ledger", "gumball-armada",
    "iron-sight-forward-detachment", "ministry-lantern-desk", "omega-front",
    "procedural-test-chamber",
]
CELL = (168, 230)
COLUMNS = 4


def produce(capture_dir: Path):
    matrix = json.loads((ROOT / "modules/game-identity-matrix.json").read_text())
    registered = [item["id"] for item in matrix["games"]]
    if len(registered) != 15 or len(set(registered)) != 15:
        raise ValueError("Expected exactly 15 unique integrated ReverieVR games")
    if not set(FEATURED).issubset(registered):
        raise ValueError("Featured menu IDs do not match the native game catalog")
    ordered = FEATURED + [item for item in registered if item not in FEATURED]
    with (capture_dir / "native-menu-capture.csv").open(newline="") as src:
        rows = list(csv.DictReader(src))
    if len(rows) != 15 or {r["module_id"] for r in rows} != set(registered):
        raise ValueError("Capture CSV lacks exactly one record for every game")

    captures = {}
    for row in rows:
        game_id = row["module_id"]
        if row["status"] != "CAPTURED" or int(row["eye_frame_crc32"]) <= 0:
            raise ValueError("Game has no real rendered menu: " + game_id)
        data = (capture_dir / f"native-menu-{game_id}.png").read_bytes()
        if len(data) != int(row["png_bytes"]) or len(data) <= 300:
            raise ValueError("Capture evidence does not match CSV: " + game_id)
        with Image.open(io.BytesIO(data)) as source:
            source.verify()
        with Image.open(io.BytesIO(data)) as source:
            image = source.convert("RGB")
        if image.size != (512, 512):
            raise ValueError("Unexpected native framebuffer resolution: " + game_id)
        if max(ImageStat.Stat(image.resize((32, 32))).stddev) < 2:
            raise ValueError("Likely blank native framebuffer: " + game_id)
        captures[game_id] = (data, image, hashlib.sha256(data).hexdigest(), row)

    # All evidence is validated *before* touching a published archive file.
    archive = ROOT / "art/archive/native-game-menus"
    cards = archive / "cards"
    gallery = ROOT / "app/src/main/assets/native/gallery"
    cards.mkdir(parents=True, exist_ok=True)
    gallery.mkdir(parents=True, exist_ok=True)
    atlas = Image.new("RGB", (CELL[0] * COLUMNS, CELL[1] * 4), (7, 14, 24))
    manifest = {
        "schema": 1,
        "source": "installed Android 10 emulator; real JNI/GLES2 initial-eye frame",
        "capture_size": [512, 512],
        "card_size": list(CELL),
        "atlas_columns": COLUMNS,
        "ordered_ids": ordered,
        "games": {},
    }
    for index, game_id in enumerate(ordered):
        data, image, digest, row = captures[game_id]
        (archive / f"{game_id}.png").write_bytes(data)
        # No crop or stretch: keep the exact menu in a letterboxed title picture.
        fitted = ImageOps.contain(image, CELL, method=Image.Resampling.LANCZOS)
        card = Image.new("RGB", CELL, (7, 14, 24))
        card.paste(fitted, ((CELL[0] - fitted.width) // 2,
                            (CELL[1] - fitted.height) // 2))
        card.save(cards / f"{game_id}.jpg", "JPEG", quality=87,
                  optimize=True, subsampling=0)
        atlas.paste(card, ((index % COLUMNS) * CELL[0],
                           (index // COLUMNS) * CELL[1]))
        manifest["games"][game_id] = {
            "sha256": digest, "bytes": len(data),
            "eye_frame_crc32": int(row["eye_frame_crc32"]),
            "atlas_cell": [index % COLUMNS, index // COLUMNS],
        }
    atlas.save(gallery / "menu-preview-atlas.jpg", "JPEG", quality=87,
               optimize=True, subsampling=0)
    (archive / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(f"VERIFIED: {len(ordered)} real installed menu captures archived")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: promote_native_game_gallery.py CAPTURE_DIRECTORY")
    produce(Path(sys.argv[1]))
