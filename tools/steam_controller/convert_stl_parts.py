#!/usr/bin/env python3
"""STL parts -> QEM OBJ + GLES2 binary. Source CAD is never modified.

Install requirements.txt in a local Python environment. Coordinates in OBJ
remain source coordinates; runtime has one shared explicit mm-to-m transform.
"""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import subprocess
import tempfile
import numpy as np
import fast_simplification
import trimesh

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def load_stl(path):
    """Memory-map binary STL; reject corrupt/nonfinite data, never silently drop it."""
    with path.open('rb') as f:
        header = f.read(84)
    if len(header) >= 84:
        count = struct.unpack_from('<I', header, 80)[0]
        if path.stat().st_size == 84 + count * 50:
            if not count:
                raise ValueError(f'empty STL: {path}')
            dtype = np.dtype([('normal', '<f4', (3,)), ('points', '<f4', (3, 3)), ('attr', '<u2')])
            data = np.memmap(path, dtype=dtype, mode='r', offset=84, shape=(count,))
            triangles = data['points']
            if not np.isfinite(triangles).all():
                raise ValueError(f'non-finite STL: {path}')
            return triangles
    vertices = []
    for line in path.read_text(encoding='ascii').splitlines():
        words = line.split()
        if words and words[0].lower() == 'vertex':
            if len(words) != 4:
                raise ValueError(f'malformed STL: {path}')
            vertices.append([float(v) for v in words[1:]])
    if not vertices or len(vertices) % 3:
        raise ValueError(f'empty/malformed STL: {path}')
    triangles = np.asarray(vertices, dtype=np.float64).reshape(-1, 3, 3)
    if not np.isfinite(triangles).all():
        raise ValueError(f'non-finite STL: {path}')
    return triangles

def clean(vertices, faces):
    p = vertices[faces]
    area = np.linalg.norm(np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]), axis=1)
    faces = faces[area > 1e-12]
    _, keep = np.unique(np.sort(faces, axis=1), axis=0, return_index=True)
    faces = faces[np.sort(keep)]
    used, inverse = np.unique(faces, return_inverse=True)
    return vertices[used], inverse.reshape(-1, 3).astype(np.int32)

def weld(triangles):
    # Exact welding avoids rounding across seams or moving small button geometry.
    points, inverse = np.unique(np.asarray(triangles).reshape(-1, 3), axis=0, return_inverse=True)
    return clean(points, inverse.reshape(-1, 3).astype(np.int32))

def topology(vertices, faces):
    edges = np.concatenate([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]])
    edges.sort(axis=1)
    unique, counts = np.unique(edges, axis=0, return_counts=True)
    # Connected components, including small detached pieces, are reported.
    parent = np.arange(len(vertices))
    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for a, b in unique:
        a, b = root(a), root(b)
        if a != b:
            parent[b] = a
    return {'boundary_edges': int(np.sum(counts == 1)),
            'nonmanifold_edges': int(np.sum(counts > 2)),
            'components': len({root(i) for i in range(len(vertices))}),
            'euler': int(len(vertices) - len(unique) + len(faces))}

def allocate(counts, target):
    # Equal minimum prevents densely tessellated thumbstick starving the buttons.
    minimum = [min(n, 128) for n in counts]
    if sum(minimum) > target:
        raise ValueError(f'target must be >= {sum(minimum)} to preserve these parts')
    budgets = minimum[:]
    remaining = target - sum(budgets)
    weights = np.sqrt(counts)
    while remaining and any(b < n for b, n in zip(budgets, counts)):
        eligible = [i for i in range(len(counts)) if budgets[i] < counts[i]]
        total = sum(weights[i] for i in eligible)
        grants = [min(counts[i] - budgets[i], max(1, int(remaining * weights[i] / total))) for i in eligible]
        for i, grant in zip(eligible, grants):
            grant = min(grant, remaining)
            budgets[i] += grant
            remaining -= grant
    return budgets

def write_obj(path, name, vertices, faces):
    with path.open('w', encoding='utf8', newline='\n') as out:
        out.write(f'# Valve Corporation 2016; modified by QEM; CC BY-NC-SA 4.0\no {name}\n')
        for v in vertices:
            out.write('v ' + ' '.join(f'{x:.9g}' for x in v) + '\n')
        for face in faces:
            out.write('f ' + ' '.join(str(int(x) + 1) for x in face) + '\n')

def runtime_vertices(vertices, faces):
    # CAD: X right, Y forward, Z up; GLES: X right, Y up, -Z forward.
    p = vertices[faces][:, :, [0, 2, 1]].astype(np.float32) * 0.001
    p[:, :, 2] *= -1
    n = np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0])
    n /= np.linalg.norm(n, axis=1)[:, None]
    return np.concatenate([p, np.repeat(n[:, None, :], 3, axis=1)], axis=2).reshape(-1, 6)

def convert(src, dst, target):
    src, dst = pathlib.Path(src).resolve(), pathlib.Path(dst).resolve()
    if src == dst or src in dst.parents or dst in src.parents:
        raise ValueError('source and output must be separate non-nested directories')
    if target < 100:
        raise ValueError('target must be at least 100')
    paths = sorted(p for p in src.rglob('*') if p.suffix.lower() == '.stl')
    if not paths:
        raise ValueError('No STL parts found')
    if dst.exists() and any(dst.iterdir()):
        raise ValueError('output must be empty; do not mix stale models')
    counts = [len(load_stl(p)) for p in paths]
    budgets = allocate(counts, target)
    dst.mkdir(parents=True, exist_ok=True)
    report = {'schema': 2, 'method': 'exact-weld + meshoptimizer QEM; flagged voxel repair where required',
              'numpy': np.__version__, 'meshoptimizer': '1.0.1',
              'target_faces': target, 'units': 'source mm; runtime m',
              'transform': '(x,y,z) -> (x,z,-y)*0.001',
              'attribution': 'Copyright 2016 Valve Corporation; modified by ReverieVR mesh tools',
              'license': 'CC-BY-NC-SA-4.0', 'input_mappings': [], 'parts': []}
    runtime = []
    for path, budget in zip(paths, budgets):
        print(f'Converting {path.name}, budget {budget}', flush=True)
        triangles = load_stl(path)
        low = triangles.min(axis=(0, 1)).tolist()
        high = triangles.max(axis=(0, 1)).tolist()
        vertices, faces = weld(triangles)
        del triangles
        input_faces = len(faces)
        # Source topology is expensive for pathological multi-million-facet CAD;
        # perform full topology accounting on the compact output instead.
        if len(faces) > budget:
            with tempfile.TemporaryDirectory(dir=dst) as temporary:
                tmp = pathlib.Path(temporary)
                vertices.astype('<f4').tofile(tmp / 'points')
                faces.astype('<u4').tofile(tmp / 'faces')
                result = subprocess.run(['node', str(pathlib.Path(__file__).with_name('simplify.mjs')),
                                         str(tmp), str(max(4, budget - 4))], check=True, capture_output=True, text=True)
                simplification_error = float(result.stdout)
                faces = np.fromfile(tmp / 'out', dtype='<u4').reshape(-1, 3)
        else:
            simplification_error = 0.0
        vertices, faces = clean(vertices, faces)
        repair = None
        initial_topology = topology(vertices, faces)
        if len(faces) > budget or initial_topology['nonmanifold_edges'] or initial_topology['boundary_edges']:
            # Repair the error-limited intermediate, not an arbitrarily degraded
            # mesh. This creates a render surface, not manufacturing CAD.
            pitch = max(0.12, np.linalg.norm(np.asarray(high) - low) / 256)
            print(f'  Surface repair at {pitch:.4g} mm: {len(faces)} faces, {initial_topology}', flush=True)
            voxel = trimesh.Trimesh(vertices, faces, process=False).voxelized(pitch).fill()
            surface = voxel.marching_cubes
            surface.vertices = surface.vertices * pitch + voxel.transform[:3, 3]
            vertices, faces = fast_simplification.simplify(surface.vertices, surface.faces.astype(np.int32),
                                                           target_count=max(4, budget - 4), agg=7)
            vertices, faces = clean(vertices, faces)
            repair = {'method': 'filled voxel surface / marching cubes / QEM', 'pitch_mm': float(pitch),
                      'intermediate_topology': initial_topology,
                      'warning': 'Render approximation; sub-voxel details/interior cavities may change. Preview required.'}
        if not len(faces) or len(faces) > budget or not np.isfinite(vertices).all():
            raise ValueError(f'QEM failed face/finite gate ({len(faces)} > {budget}): {path}')
        name = re.sub('[^A-Za-z0-9_-]', '_', path.stem)
        name += '_' + hashlib.sha256(path.relative_to(src).as_posix().encode()).hexdigest()[:8]
        out = dst / (name + '.obj')
        write_obj(out, name, vertices, faces)
        topo = topology(vertices, faces)
        part = {'name': name, 'input': path.relative_to(src).as_posix(), 'output': out.name,
                'source_sha256': sha256(path), 'output_sha256': sha256(out),
                'input_faces': counts[len(report['parts'])], 'clean_faces': input_faces,
                'output_faces': len(faces), 'vertices': len(vertices), 'budget': budget,
                'relative_error_estimate_before_repair': simplification_error, 'repair': repair,
                'bbox_min': low, 'bbox_max': high,
                'output_bbox_min': vertices.min(axis=0).tolist(),
                'output_bbox_max': vertices.max(axis=0).tolist(), 'topology': topo}
        report['parts'].append(part)
        runtime.append((name, runtime_vertices(vertices, faces)))
    binary = dst / 'controller.rvmesh'
    with binary.open('wb') as out:
        out.write(b'RVSC0001')
        out.write(struct.pack('<I', len(runtime)))
        for name, values in runtime:
            encoded = name.encode('ascii')
            out.write(struct.pack('<II', len(encoded), len(values)))
            out.write(encoded)
            out.write(values.astype('<f4').tobytes())
    report['runtime_sha256'] = sha256(binary)
    report['total_faces'] = sum(p['output_faces'] for p in report['parts'])
    (dst / 'conversion_manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf8')
    return report

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    parser.add_argument('--target-faces', type=int, default=12000)
    args = parser.parse_args()
    report = convert(args.source, args.output, args.target_faces)
    print(f"Converted {len(report['parts'])} parts; {report['total_faces']} faces")

if __name__ == '__main__':
    main()
