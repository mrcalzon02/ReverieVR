#!/usr/bin/env python3
"""Source-only quality gate for GitHub-hosted, reproducible artwork galleries.

Strictly validates media objects and the index; no network or external packages.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

MAX_ASSET_BYTES = 4 * 1024 * 1024
MAX_GALLERY_BYTES = 30 * 1024 * 1024
MEDIA_EXTENSIONS = {'.gif', '.png', '.jpg', '.jpeg', '.webp'}
SHA256 = re.compile(r'[0-9a-f]{64}\Z')
SOURCE_REVISION = re.compile(r'[0-9a-f]{40}\Z')


def gif_frames(raw: bytes) -> tuple[int, int, int]:
    if len(raw) < 14 or raw[:6] not in (b'GIF87a', b'GIF89a'):
        raise ValueError('invalid GIF signature')
    w, h, packed = struct.unpack_from('<HHB', raw, 6)
    if not 16 <= w <= 2048 or not 16 <= h <= 2048:
        raise ValueError('unexpected GIF pixel dimensions')
    offset = 13
    if packed & 128:
        offset += 3 * (2 ** ((packed & 7) + 1))
    frames = 0

    def skip_subblocks(i: int) -> int:
        while i < len(raw):
            size = raw[i]
            i += 1
            if size == 0:
                return i
            i += size
        raise ValueError('incomplete GIF data blocks')

    while offset < len(raw):
        marker = raw[offset]
        offset += 1
        if marker == 0x3B:
            if offset != len(raw):
                raise ValueError('trailing GIF content')
            return w, h, frames
        if marker == 0x21:
            offset += 1  # extension type
            offset = skip_subblocks(offset)
        elif marker == 0x2C:
            if offset + 9 >= len(raw):
                raise ValueError('incomplete GIF image descriptor')
            _, _, _, _, flags = struct.unpack_from('<HHHHB', raw, offset)
            offset += 9
            if flags & 128:
                offset += 3 * (2 ** ((flags & 7) + 1))
            offset += 1  # LZW min code size
            offset = skip_subblocks(offset)
            frames += 1
        else:
            raise ValueError(f'unknown GIF block 0x{marker:02x}')
        if offset > len(raw):
            raise ValueError('GIF data beyond end of file')
    raise ValueError('missing GIF trailer')


def verify_gallery(folder: Path) -> int:
    data = json.loads((folder / 'gallery.json').read_text(encoding='utf-8'))
    if data.get('format') != 'reverievr-development-gallery-v1':
        raise ValueError(f'{folder}: bad gallery manifest version')
    if not data.get('source_kind') or not data.get('license'):
        raise ValueError(f'{folder}: asset origin/license are mandatory')
    if not SOURCE_REVISION.fullmatch(str(data.get('source_commit', ''))):
        raise ValueError(f'{folder}: source commit must be exact 40-digit SHA')
    files = data.get('assets')
    if not isinstance(files, list) or not 1 <= len(files) <= 80:
        raise ValueError(f'{folder}: no complete asset list')
    readme = (folder / 'README.md').read_text(encoding='utf-8')
    names: set[str] = set()
    total = 0
    for asset in files:
        name = asset.get('file')
        if not isinstance(name, str) or name in names or not re.fullmatch(
            r'[a-zA-Z0-9_.-]{4,120}', name):
            raise ValueError(f'{folder}: invalid/duplicate asset name {name}')
        names.add(name)
        path = folder / name
        if path.suffix.lower() not in MEDIA_EXTENSIONS or not path.is_file():
            raise ValueError(f'{folder}: gallery image not committed: {name}')
        size = path.stat().st_size
        total += size
        if not 64 <= size <= MAX_ASSET_BYTES or size != asset.get('bytes'):
            raise ValueError(f'{folder}: size mismatch: {name}')
        if not SHA256.fullmatch(str(asset.get('sha256', ''))):
            raise ValueError(f'{folder}: missing SHA-256: {name}')
        raw = path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != asset['sha256']:
            raise ValueError(f'{folder}: content changed without provenance: {name}')
        if path.suffix.lower() == '.gif':
            w, h, frames = gif_frames(raw)
            if [w, h] != asset.get('size') or frames != asset.get('frames'):
                raise ValueError(f'{folder}: missing turntable frames: {name}')
        elif path.suffix.lower() in ('.jpg', '.jpeg'):
            if not raw.startswith(b'\xff\xd8\xff') or not raw.endswith(b'\xff\xd9'):
                raise ValueError(f'{folder}: invalid/truncated JPEG: {name}')
        elif path.suffix.lower() == '.png':
            if not raw.startswith(b'\x89PNG\r\n\x1a\n'):
                raise ValueError(f'{folder}: invalid PNG: {name}')
        elif path.suffix.lower() == '.webp':
            if raw[:4] != b'RIFF' or raw[8:12] != b'WEBP':
                raise ValueError(f'{folder}: invalid WEBP: {name}')
        if name not in readme:
            raise ValueError(f'{folder}: media not displayed or linked in README: {name}')
    if total > MAX_GALLERY_BYTES:
        raise ValueError(f'{folder}: web gallery too heavy')
    extra = {x.name for x in folder.iterdir() if x.is_file() and
             x.suffix.lower() in MEDIA_EXTENSIONS} - names
    if extra:
        raise ValueError(f'{folder}: unindexed gallery images: {sorted(extra)}')
    return len(files)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', default='docs/gallery', type=Path)
    args = parser.parse_args()
    if not (args.root / 'README.md').is_file():
        raise SystemExit('Gallery root index missing')
    roots = sorted(args.root.rglob('gallery.json'))
    if not roots:
        raise SystemExit('No published galleries')
    total = 0
    index = (args.root / 'README.md').read_text()
    for manifest in roots:
        relative = manifest.parent.relative_to(args.root).as_posix()
        if relative not in index:
            raise SystemExit('Gallery absent from root index: ' + relative)
        amount = verify_gallery(manifest.parent)
        total += amount
        print(f'PASS {relative}: {amount} verified GitHub images')
    print(f'PASS {len(roots)} galleries, {total} images: exact hashes and real GIF frames')


if __name__ == '__main__':
    main()