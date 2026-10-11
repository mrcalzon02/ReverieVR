#!/usr/bin/env python3
"""Verify and package the noncommercial render handoff, without original CAD."""
import argparse
import json
import pathlib
import shutil
import zipfile
from convert_stl_parts import sha256
from prepare_local_assets import ARCHIVE_SHA256, ARCHIVE_URL, ANNOUNCEMENT, LICENSE_SHA256
from verify_outputs import verify

def package(folder, archive, license_path, destination):
    folder, archive, license_path, destination = map(pathlib.Path, (folder, archive, license_path, destination))
    if destination.exists():
        raise ValueError('Do not overwrite an existing handoff')
    if sha256(archive) != ARCHIVE_SHA256 or sha256(license_path) != LICENSE_SHA256:
        raise ValueError('Unverified source archive/license')
    verify(folder)
    manifest = json.loads((folder/'conversion_manifest.json').read_text())
    shutil.copyfile(license_path, folder/'LICENSE.TXT')
    provenance = {'source': ARCHIVE_URL, 'announcement': ANNOUNCEMENT,
                  'archive_sha256': ARCHIVE_SHA256, 'archive_bytes': archive.stat().st_size,
                  'license_sha256': LICENSE_SHA256, 'license': 'CC-BY-NC-SA-4.0',
                  'copyright': 'Copyright 2016 Valve Corporation',
                  'modifications': manifest['method'], 'source_cad_preserved_separately': True}
    (folder/'PROVENANCE.json').write_text(json.dumps(provenance, indent=2)+'\n', encoding='utf8')
    flags = [{'part': p['name'], 'topology': p['topology']} for p in manifest['parts']
             if p['topology']['boundary_edges'] or p['topology']['nonmanifold_edges']]
    validation = {'readback': 'passed', 'input_triangles': sum(p['input_faces'] for p in manifest['parts']),
                  'output_triangles': manifest['total_faces'], 'parts': len(manifest['parts']),
                  'remaining_topology_flags': flags, 'android_build': 'not built: Android SDK missing',
                  'renderer_hookup': 'pending online instance', 'device_validation': 'not performed',
                  'physical_input_mappings': []}
    (folder/'VALIDATION.json').write_text(json.dumps(validation, indent=2)+'\n', encoding='utf8')
    (folder/'HANDOFF.md').write_text('''# ReverieVR original Steam Controller handoff

Copyright 2016 Valve Corporation. Modified render assets: exact welding,
QEM simplification and explicitly recorded voxel surface approximations.
Assets are CC BY-NC-SA 4.0. Preserve LICENSE.TXT and PROVENANCE.json when
sharing. Commercial use needs permission from Valve. No Valve endorsement.

Start with controller.glb: named parts, meters, +X right, +Y up, -Z forward.
assembled.obj uses the same meter coordinates and can open in ReverieVR's
existing OBJ viewer. Individual OBJ files retain original source mm coordinates;
use (x,y,z) -> (x,z,-y)*0.001 exactly once. Do not recenter parts independently.
Colors identify parts only. Source file assemblies remain single named parts;
actual hinge pivots still require mechanical/visual review.

conversion_manifest.json records all source/output hashes, part budgets,
bounds, repair resolutions and topology flags. VALIDATION.json records
completed checks and remaining gates. These are render approximations;
seven parts retain boundary/nonmanifold flags. Do not treat them as watertight
manufacturing CAD or validated collision volumes. Prefer primitive colliders.
Tiny details, round openings, lettering and internal cavities may change.

controller.rvmesh is directly packable for GLES2: little-endian magic
RVSC0001 (8 bytes), uint32 part count; per part uint32 name length, uint32
vertex count, ASCII name, then six float32 per vertex (position XYZ, normal
XYZ). Consecutive vertex triples are triangles. Suggested one static VBO,
24-byte stride, position offset 0 / normal offset 12, with named draw ranges.
Keep the source-license/manifest alongside optional runtime assets. Re-upload
on GL context recreation, release GL objects, and preserve shell GL state.

Programming/hookup belongs to the next online instance: work on
mrcalzon02/ReverieVR main, read AGENTS.md and project execution doctrine,
integrate through the existing controller visualization architecture and
document/test the change. No APK or integrated Steam renderer was built here.
No physical Android part mappings are provided: right-pad mouse movement and
an unspecified trigger click are user observations, not exact event reports.
Capture Android device/key/axis/button reports before physical-part mappings.
Do not fabricate gyro, dual-pad, grip or trigger-side support. Test Galaxy S9
stereo, lifecycle/recovery, event visualization and sustained performance.

Original ZIP and CAD are preserved locally and are deliberately excluded from
this compact handoff; the authoritative source URL and SHA256 are in PROVENANCE.
Reproducible pipeline source and dependency pins are included under pipeline/.
SHA256SUMS covers each packaged file except the checksum list itself.
''', encoding='utf8')
    names = [p['output'] for p in manifest['parts']] + ['controller.glb','assembled.obj','controller.rvmesh',
             'conversion_manifest.json','assembled-preview.png','individual-parts-preview.png',
             'LICENSE.TXT','PROVENANCE.json','VALIDATION.json','HANDOFF.md']
    source = pathlib.Path(__file__).parent
    code = [p for p in source.iterdir() if p.suffix in ('.py','.mjs','.md') or p.name in
            ('requirements.txt','package.json','package-lock.json')]
    entries = [(folder/n,n) for n in names] + [(p,'pipeline/'+p.name) for p in code]
    checksum = ''.join(f'{sha256(p)}  {n}\n' for p,n in sorted(entries,key=lambda pair:pair[1]))
    with zipfile.ZipFile(destination, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path, name in entries:
            z.write(path, name)
        z.writestr('SHA256SUMS', checksum)
    with zipfile.ZipFile(destination) as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(entries)+1
    print(f'Package {destination.resolve()} / {destination.stat().st_size} bytes / SHA256 {sha256(destination)}')

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['converted','archive','license','output']:
        p.add_argument(name,type=pathlib.Path)
    a=p.parse_args()
    package(a.converted,a.archive,a.license,a.output)
