#!/usr/bin/env python3
"""Create labeled orthographic/isometric QA views and a named-parts GLB."""
import argparse
import json
import pathlib
import os
os.environ.setdefault('MPLCONFIGDIR', str(pathlib.Path(__file__).resolve().parent / '__pycache__' / 'matplotlib'))
import numpy as np
import trimesh
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

def preview(folder):
    folder = pathlib.Path(folder)
    manifest = json.loads((folder / 'conversion_manifest.json').read_text())
    meshes = []
    scene = trimesh.Scene()
    palette = plt.get_cmap('tab20')
    for i, part in enumerate(manifest['parts']):
        mesh = trimesh.load(folder / part['output'], force='mesh', process=False)
        meshes.append(mesh)
        runtime = mesh.copy()
        runtime.vertices = runtime.vertices[:, [0, 2, 1]] * [0.001, 0.001, -0.001]
        runtime.visual.face_colors = np.asarray(palette(i % 20)) * 255
        scene.add_geometry(runtime, node_name=part['name'], geom_name=part['name'])
    (folder / 'controller.glb').write_bytes(scene.export(file_type='glb'))
    with (folder / 'assembled.obj').open('w', encoding='utf8', newline='\n') as stream:
        stream.write('# Valve Corporation 2016; QEM/voxel render adaptation; CC BY-NC-SA 4.0\n')
        stream.write('# Coordinates in metres: +X right, +Y up, -Z forward\n')
        offset = 1
        for mesh, part in zip(meshes, manifest['parts']):
            stream.write('o ' + part['name'] + '\n')
            vertices = mesh.vertices[:, [0, 2, 1]] * [0.001, 0.001, -0.001]
            for point in vertices:
                stream.write('v ' + ' '.join(f'{x:.9g}' for x in point) + '\n')
            for face in mesh.faces:
                stream.write('f ' + ' '.join(str(int(x) + offset) for x in face) + '\n')
            offset += len(vertices)
    points = np.concatenate([m.vertices for m in meshes])
    center = (points.min(0) + points.max(0)) / 2
    radius = max(points.max(0) - points.min(0)) / 2
    fig = plt.figure(figsize=(15, 11))
    for index, (title, elevation, azimuth) in enumerate([
            ('Top: CAD +Z; X right / +Y forward', 90, -90),
            ('Front: looking from CAD +Y', 0, 90),
            ('Side: looking from CAD +X', 0, 0),
            ('Assembled named parts: isometric', 35, -65)]):
        ax = fig.add_subplot(2, 2, index + 1, projection='3d')
        for i, mesh in enumerate(meshes):
            poly = Poly3DCollection(mesh.vertices[mesh.faces], linewidths=.10,
                                    edgecolor=(0, 0, 0, .3), alpha=1)
            poly.set_facecolor(palette(i % 20))
            ax.add_collection3d(poly)
        ax.set(xlim=(center[0]-radius, center[0]+radius),
               ylim=(center[1]-radius, center[1]+radius),
               zlim=(center[2]-radius, center[2]+radius),
               xlabel='X mm', ylabel='Y mm', zlabel='Z mm', title=title)
        ax.set_box_aspect((1, 1, 1))
        ax.set_proj_type('ortho')
        ax.view_init(elevation, azimuth)
        if index < 3:
            ax.set_axis_off()
    fig.suptitle(f"Valve original Steam Controller: {manifest['total_faces']} triangles / {len(meshes)} parts")
    fig.tight_layout()
    fig.savefig(folder / 'assembled-preview.png', dpi=130)
    plt.close(fig)
    fig = plt.figure(figsize=(18, 16))
    for i, (mesh, part) in enumerate(zip(meshes, manifest['parts'])):
        ax = fig.add_subplot(5, 5, i + 1, projection='3d')
        poly = Poly3DCollection(mesh.vertices[mesh.faces], linewidths=.1, edgecolor=(0, 0, 0, .35))
        poly.set_facecolor(palette(i % 20))
        ax.add_collection3d(poly)
        low, high = mesh.bounds
        c = (low + high) / 2
        r = max(high - low) / 2 * 1.05
        ax.set(xlim=(c[0]-r, c[0]+r), ylim=(c[1]-r, c[1]+r), zlim=(c[2]-r, c[2]+r))
        ax.set_box_aspect((1, 1, 1))
        ax.view_init(35, -65)
        ax.set_title(part['input'].split('.rev')[0] + f"\n{part['output_faces']} faces", fontsize=9)
        ax.set_axis_off()
    fig.tight_layout()
    fig.savefig(folder / 'individual-parts-preview.png', dpi=120)
    plt.close(fig)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=pathlib.Path)
    preview(parser.parse_args().folder)
