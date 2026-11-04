#!/usr/bin/env python3
"""Offline-only Breakwater material derivation; requires Pillow on the authoring machine."""
from pathlib import Path
from PIL import Image
import argparse

MAPPING = (
    ("coarse_shingle_beach_texture.png", "shingle.png"),
    ("weathered_coastal_bunker_concrete.png", "quay_concrete.png"),
    ("seamless_pixel_art_basalt_rock_texture.png", "black_basalt.png"),
    ("seamless_oxidized_coastal_steel_texture.png", "oxidized_steel.png"),
    ("seamless_pixel_art_sandbag_wall.png", "sandbag_burlap.png"),
    ("chipped_olive_military_metal_texture.png", "olive_enamel.png"),
)
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "art/breakwater/source"
DESTINATION = ROOT / "app/src/main/assets/native/breakwater/materials"

def game_tile(source):
    image = Image.open(source).convert("RGB").resize((64, 64), Image.Resampling.BOX)
    output = Image.new("RGB", (64, 64))
    result = []
    for r, g, b in image.getdata():
        packed = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        result.append((((packed >> 11) & 31) * 255 // 31,
                       ((packed >> 5) & 63) * 255 // 63,
                       (packed & 31) * 255 // 31))
    output.putdata(result)
    return output

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true",
                        help="Compare decoded pixels to committed outputs without writing")
    opts = parser.parse_args()
    for original, target in MAPPING:
        source = SOURCE / original
        destination = DESTINATION / target
        if not source.is_file():
            raise SystemExit(f"Missing authored source: {source}")
        tile = game_tile(source)
        if opts.verify:
            if not destination.is_file() or Image.open(destination).convert("RGB").tobytes() != tile.tobytes():
                raise SystemExit(f"Material mismatch: {target}")
        else:
            DESTINATION.mkdir(parents=True, exist_ok=True)
            tile.save(destination, optimize=True)
        print(f"OK: {target}")

if __name__ == "__main__":
    main()
