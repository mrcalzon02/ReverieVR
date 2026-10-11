#!/usr/bin/env python3
"""Independent readback of named OBJ, manifest, GLB and GLES2 binary outputs."""
import argparse
import json
import pathlib
import struct
import numpy as np
import trimesh
from convert_stl_parts import sha256, topology

def verify(folder):
    folder = pathlib.Path(folder)
    report = json.loads((folder/'conversion_manifest.json').read_text())
    assert sum(p['output_faces'] for p in report['parts']) == report['total_faces'] <= report['target_faces']
    assert report['input_mappings'] == [], 'No physical mapping is supported by observed reports yet'
    for part in report['parts']:
        path = folder/part['output']
        assert sha256(path) == part['output_sha256']
        mesh = trimesh.load(path, force='mesh', process=False)
        assert len(mesh.faces) == part['output_faces']
        assert np.isfinite(mesh.vertices).all()
        np.testing.assert_allclose(mesh.bounds[0], part['output_bbox_min'], atol=1e-5)
        np.testing.assert_allclose(mesh.bounds[1], part['output_bbox_max'], atol=1e-5)
        assert topology(mesh.vertices, mesh.faces) == part['topology']
    path = folder/'controller.rvmesh'
    assert sha256(path) == report['runtime_sha256']
    with path.open('rb') as data:
        assert data.read(8) == b'RVSC0001'
        assert struct.unpack('<I', data.read(4))[0] == len(report['parts'])
        for part in report['parts']:
            size, count = struct.unpack('<II', data.read(8))
            assert data.read(size).decode('ascii') == part['name']
            assert count == part['output_faces']*3
            values = np.frombuffer(data.read(count*24), dtype='<f4').reshape(-1, 6)
            assert np.isfinite(values).all()
            np.testing.assert_allclose(np.linalg.norm(values[:,3:], axis=1), 1, atol=1e-5)
        assert not data.read(1), 'unexpected trailing bytes'
    if (folder/'controller.glb').exists():
        scene = trimesh.load(folder/'controller.glb', force='scene', process=False)
        assert set(scene.geometry) == {p['name'] for p in report['parts']}
        assert sum(len(m.faces) for m in scene.geometry.values()) == report['total_faces']
    print(f"Verified {len(report['parts'])} named parts / {report['total_faces']} triangles; binary normals, hashes and GLB readback pass")

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('folder', type=pathlib.Path)
    verify(p.parse_args().folder)
