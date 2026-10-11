#!/usr/bin/env python3
"""Explicit noncommercial local import of Valve's original-controller ZIP.

No download or APK packaging happens by default. Preserve original ZIP/CAD
separately from derivatives. Do not use this payload in commercial releases
without separately obtaining permission from Valve.
"""
import argparse
import json
import pathlib
import shutil
import zipfile
from convert_stl_parts import convert, sha256
from preview_parts import preview

ARCHIVE_SHA256 = '957eafa3a93d388fba1fc42769d06603f40800fb5ea2d613cf19e8ddc10df25b'
ARCHIVE_URL = 'https://cdn.akamai.steamstatic.com/steam/controller/CAD_data/MM06181.rev01.01.SteamControllerWorkshop03.stl.zip'
ANNOUNCEMENT = 'https://store.steampowered.com/news/app/353370/view/3931035846865618326'
LICENSE_SHA256 = '34b2f05d62a57b96f21625b4ad1b024a339d9617bba93f4eeed9f325869e74ce'

def prepare(archive, root, target):
    archive, root = pathlib.Path(archive).resolve(), pathlib.Path(root).resolve()
    if sha256(archive) != ARCHIVE_SHA256:
        raise ValueError('Archive SHA256 does not match the inspected March 2016 release')
    if root.exists() and any(root.iterdir()):
        raise ValueError('Use a new empty import directory')
    root.mkdir(parents=True, exist_ok=True)
    source = root / 'source'
    source.mkdir()
    preserved = source / 'original.zip'
    shutil.copyfile(archive, preserved)
    extracted = source / 'cad'
    extracted.mkdir()
    with zipfile.ZipFile(preserved) as z:
        entries = z.infolist()
        if len(entries) > 100 or sum(e.file_size for e in entries) > 3_000_000_000:
            raise ValueError('Archive exceeds inspected inventory bounds')
        for e in entries:
            p = pathlib.PurePosixPath(e.filename.replace('\\', '/'))
            if p.is_absolute() or '..' in p.parts or ':' in e.filename or (e.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Unsafe archive entry')
            if e.is_dir():
                continue
            out = extracted.joinpath(*p.parts)
            out.parent.mkdir(parents=True, exist_ok=True)
            with z.open(e) as data, out.open('wb') as stream:
                shutil.copyfileobj(data, stream)
    license_path = extracted / 'LICENSE.TXT'
    if sha256(license_path) != LICENSE_SHA256:
        raise ValueError('Unexpected license; inspect before importing')
    converted = root / 'converted'
    report = convert(extracted, converted, target)
    preview(converted)
    payload = root / 'assets' / 'models' / 'steam-controller'
    payload.mkdir(parents=True)
    for name in ['controller.rvmesh', 'conversion_manifest.json']:
        shutil.copyfile(converted / name, payload / name)
    shutil.copyfile(license_path, payload / 'LICENSE.TXT')
    provenance = {'archive_url': ARCHIVE_URL, 'announcement': ANNOUNCEMENT,
                  'archive_sha256': ARCHIVE_SHA256, 'license_sha256': sha256(license_path),
                  'copyright': 'Copyright 2016 Valve Corporation', 'license': 'CC-BY-NC-SA-4.0',
                  'modifications': report['method'], 'source_cad_preserved': True,
                  'physical_input_mappings': [], 'runtime_validation': 'Not device validated'}
    (payload / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    shutil.copyfile(payload / 'provenance.json', converted / 'provenance.json')
    shutil.copyfile(license_path, converted / 'LICENSE.TXT')
    print(f'Local opt-in payload: {payload.parent.parent}')

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('archive', type=pathlib.Path)
    p.add_argument('output', type=pathlib.Path)
    p.add_argument('--target-faces', type=int, default=12000)
    p.add_argument('--accept-noncommercial-sharealike', action='store_true', required=True)
    a = p.parse_args()
    prepare(a.archive, a.output, a.target_faces)
