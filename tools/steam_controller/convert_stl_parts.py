#!/usr/bin/env python3
"""Offline Valve 2015 Steam Controller STL -> part-preserving low-poly OBJ converter.

Usage:
  python tools/steam_controller/convert_stl_parts.py SOURCE_STL_DIRECTORY OUTPUT_DIRECTORY --target-faces 12000

SOURCE should be the extracted Valve *separate parts* STL archive. Outputs are
not committed automatically; inspect the source archive's actual license first.
Only Python standard library is required.
"""
import argparse
import hashlib
import json
import math
import pathlib
import struct

def load_stl(path):
    data = path.read_bytes()
    if len(data) >= 84:
        count = struct.unpack_from("<I", data, 80)[0]
        if 84 + count * 50 == len(data):
            triangles = []
            for i in range(count):
                offset = 84 + 50*i + 12
                nums = struct.unpack_from("<9f", data, offset)
                if all(math.isfinite(v) for v in nums):
                    triangles.append(tuple(tuple(nums[k:k+3]) for k in (0,3,6)))
            return triangles
    # ASCII STL; explicit vertex records. Raise on malformed or empty data.
    triangles, current = [], []
    for line in data.decode("utf-8", errors="replace").splitlines():
        words = line.strip().split()
        if len(words) == 4 and words[0].lower() == "vertex":
            try:
                vertex = tuple(float(x) for x in words[1:])
            except ValueError:
                raise ValueError(f"invalid STL vertex: {path}")
            if not all(math.isfinite(v) for v in vertex):
                raise ValueError(f"non-finite STL vertex: {path}")
            current.append(vertex)
            if len(current) == 3:
                triangles.append(tuple(current))
                current = []
    if current or not triangles:
        raise ValueError(f"empty/malformed STL: {path}")
    return triangles

def optimize(triangles, cell):
    """Weld nearby vertices with deterministic spatial quantization.

    This is a *coarse* LOD method, not curvature-preserving decimation. Preserve
    individually named parts and their coordinates. Degenerate faces are removed.
    """
    verts, indexes, faces, seen = [], {}, [], set()
    for tri in triangles:
        ids = []
        for point in tri:
            key = tuple(round(v / cell) for v in point)
            if key not in indexes:
                indexes[key] = len(verts) + 1
                verts.append(tuple(v * cell for v in key))
            ids.append(indexes[key])
        if len(set(ids)) < 3:
            continue
        a, b, c = (verts[i-1] for i in ids)
        ab = tuple(b[i] - a[i] for i in range(3))
        ac = tuple(c[i] - a[i] for i in range(3))
        cross = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        if sum(v*v for v in cross) < 1e-20:
            continue
        canonical = tuple(sorted(ids))
        if canonical not in seen:
            seen.add(canonical)
            faces.append(tuple(ids))
    return verts, faces

def convert(src, dst, target):
    parts = sorted(p for p in src.rglob("*") if p.suffix.lower() == ".stl")
    if not parts:
        raise ValueError("No .stl files found: extract the 2015 separate-parts archive")
    dst.mkdir(parents=True, exist_ok=True)
    report = {"source_dir": str(src), "target_faces": target, "parts": []}
    for idx, path in enumerate(parts):
        triangles = load_stl(path)
        coordinates = [v for face in triangles for v in face]
        minimum = [min(v[k] for v in coordinates) for k in range(3)]
        maximum = [max(v[k] for v in coordinates) for k in range(3)]
        diag = math.sqrt(sum((maximum[k]-minimum[k])**2 for k in range(3)))
        # Binary-search quantization resolution to meet per-part face budget.
        budget = max(64, round(target * len(triangles) / max(1, sum_count)))
        lo, hi = max(diag / 1e7, 1e-9), max(diag, 1e-8)
        candidate = optimize(triangles, lo)
        if len(candidate[1]) > budget:
            for _ in range(28):
                mid = (lo * hi)**0.5
                attempt = optimize(triangles, mid)
                if len(attempt[1]) > budget:
                    lo = mid
                else:
                    hi = mid
                    candidate = attempt
        if not candidate[1]:
            raise ValueError(f"quantization removed all triangles in {path}")
        vertices, faces = candidate
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in path.stem)
        name = f"{idx:03d}_{safe}.obj"
        out = dst / name
        with out.open("w", encoding="utf8") as stream:
            stream.write(f"# Derived from {path.name}; not licensed by this tool\n")
            stream.write(f"o {safe}\n")
            for x, y, z in vertices:
                stream.write(f"v {x:.8g} {y:.8g} {z:.8g}\n")
            for a, b, c in faces:
                stream.write(f"f {a} {b} {c}\n")
        report["parts"].append({
            "input": str(path.relative_to(src)), "output": name,
            "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "input_faces": len(triangles), "output_faces": len(faces),
            "vertices": len(vertices), "bbox_min": minimum, "bbox_max": maximum,
        })
    (dst / "conversion_manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf8")
    return report

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--target-faces", type=int, default=12000)
    args = parser.parse_args()
    if args.target_faces < 100:
        parser.error("--target-faces must be at least 100")
    global sum_count
    source_files = sorted(p for p in args.source.rglob("*") if p.suffix.lower() == ".stl")
    sum_count = sum(len(load_stl(p)) for p in source_files)
    report = convert(args.source, args.output, args.target_faces)
    print(f"Converted {len(report['parts'])} parts; "
          f"{sum(item['output_faces'] for item in report['parts'])} total faces")

if __name__ == "__main__":
    main()
