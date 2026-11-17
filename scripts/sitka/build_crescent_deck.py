#!/usr/bin/env python3
"""Bake the municipal dock footprints. Offline, Shapely 2.1+, no build-time dependency."""
import json
import struct
from pathlib import Path
from shapely import Polygon, constrained_delaunay_triangles, normalize

ROOT = Path(__file__).resolve().parents[2]
INPUT = ROOT / "data/sitka/geography/crescent-harbor/municipal-docks.json"
OUTPUT = ROOT / "data/sitka/geography/crescent-harbor/crescent-deck.sitkam"
EAST, NORTH = 480000, 6322900


def bake():
    records = json.loads(INPUT.read_text())["docks"]["features"]
    vertices, indices = [], []
    for record in records:
        outline = [(round(x-EAST, 3), round(n-NORTH, 3)) for x, n in record["ring"][:-1]]
        deck = Polygon(outline)
        if not deck.is_valid:
            raise ValueError(f'Invalid municipal dock {record["id"]}')
        # Stable numbering and triangle order across input traversal order.
        tops = {xy: len(vertices)+i for i, xy in enumerate(outline)}
        for x, n in outline:
            vertices.append((x, .85, n, 188, 158, 114))
        triangles = sorted(tuple(sorted(tuple(p) for p in t.exterior.coords[:-1]))
                           for t in constrained_delaunay_triangles(normalize(deck)).geoms)
        for tri in triangles:
            indices.extend(tops[xy] for xy in tri)
        if abs(sum(Polygon(t).area for t in triangles) - deck.area) > .0001:
            raise ValueError("Deck tessellation missed the source footprint")
        # Float skirts leave the geographic top edges and route unchanged.
        for a, b in zip(outline, outline[1:] + outline[:1]):
            start = len(vertices)
            for xy, h in ((a,.85),(b,.85),(b,.05),(a,.05)):
                vertices.append((xy[0],h,xy[1],88,82,66))
            indices.extend(start+k for k in (0,1,2,0,2,3))
    result = struct.pack("<8sIIdd", b"SITKA01\0", len(vertices), len(indices)//3, EAST, NORTH)
    result += b"".join(struct.pack("<fffBBB", *v) for v in vertices)
    result += struct.pack(f"<{len(indices)}I", *indices)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(result)
    print(f"{len(records)} city features, {len(indices)//3} triangles, {len(result)} bytes")


if __name__ == "__main__":
    bake()
