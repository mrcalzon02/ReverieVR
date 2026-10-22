#!/usr/bin/env python3
"""Rebuild 40 Gumball node thumbnails from the approved text-free Drive atlas.
Requires Pillow; no remote requests, silent placeholders, or layout guessing.
Usage: python3 scripts/gumball_prepare_art.py [--verify]
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "art/gumball/node_atlas.json"


def make_tiles(verify=False):
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    names = data["tiles"]
    cols, rows = data["columns"], data["rows"]
    if len(names) != cols * rows or len(names) != len(set(names)):
        raise ValueError("node names must uniquely cover every atlas cell")
    source = ROOT / data["source"]
    if not source.is_file():
        raise FileNotFoundError(f"missing approved source atlas: {source}")
    out = ROOT / data["output"]
    with Image.open(source) as image:
        image.load()
        if image.format != "PNG" or image.size != (1672, 941):
            raise ValueError(f"unexpected source atlas dimensions/format: {image.size}, {image.format}")
        for n, name in enumerate(names):
            col, row = n % cols, n // cols
            # Integer-quantized boundaries avoid dropped or duplicated columns.
            box = (col * image.width // cols, row * image.height // rows,
                   (col + 1) * image.width // cols,
                   (row + 1) * image.height // rows)
            tile = image.crop(box).convert("RGBA")
            tile.thumbnail((data["asset_resolution"], data["asset_resolution"]), Image.Resampling.LANCZOS)
            target = out / (name + ".png")
            if verify:
                if not target.is_file():
                    raise FileNotFoundError(f"missing generated tile: {target}")
                with Image.open(target) as actual:
                    if actual.size != tile.size or actual.convert("RGBA").tobytes() != tile.tobytes():
                        raise ValueError(f"asset out of date: {target}")
            else:
                out.mkdir(parents=True, exist_ok=True)
                tile.save(target, format="PNG", optimize=True)
    print(f"{'Verified' if verify else 'Prepared'} {len(names)} Gumball node tiles in {out}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    make_tiles(parser.parse_args().verify)
