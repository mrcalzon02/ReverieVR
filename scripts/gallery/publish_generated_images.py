#!/usr/bin/env python3
"""Stage generated images into a Git-tracked review gallery (stdlib only).

This prepares a complete commit; it never silently pushes, overwrites
existing review batches, or asserts media has been committed remotely.
"""
from __future__ import annotations
import argparse
from datetime import date
import hashlib
import json
from pathlib import Path
import re
import shutil

try:
    from verify_generated_gallery import gif_frames, MEDIA_EXTENSIONS, verify_gallery
except ImportError:
    from scripts.gallery.verify_generated_gallery import gif_frames, MEDIA_EXTENSIONS, verify_gallery

SLUG = re.compile(r'[a-z0-9][a-z0-9-]{0,63}\Z')
REV = re.compile(r'[0-9a-f]{40}\Z')


def stage(project: str, collection: str, batch: str, source: Path,
          root: Path, title: str, revision: str, license_name: str,
          source_kind: str) -> Path:
    if not all(SLUG.fullmatch(x) for x in (project, collection, batch)):
        raise ValueError('Use simple reviewed lowercase gallery path segments')
    if not REV.fullmatch(revision):
        raise ValueError('Use exact source commit SHA, not a branch name')
    if not license_name.strip() or not source_kind.strip() or not title.strip():
        raise ValueError('Artwork needs explicit title, source and license')
    media = sorted(f for f in source.iterdir()
                   if f.is_file() and f.suffix.lower() in MEDIA_EXTENSIONS)
    if not 1 <= len(media) <= 80:
        raise ValueError('No generated media or too many files')
    if sum(f.stat().st_size for f in media) > 30 * 1024 * 1024:
        raise ValueError('Web-optimized gallery must stay under 30 MiB')
    target = root / project / collection / batch
    if target.exists():
        raise FileExistsError(f'Gallery batch exists (no silent overwrite): {target}')
    target.mkdir(parents=True)
    entries = []
    readme = [f'# {title}', '',
              f'**Project:** {project} | **Source:** `{revision}` | **License:** {license_name}',
              '', f'**Asset provenance:** {source_kind}.',
              'Image previews are development references, not necessarily in-game screenshots.', '']
    for p in media:
        if not re.fullmatch(r'[a-zA-Z0-9_.-]{4,120}', p.name):
            raise ValueError('Unsafe image filename: ' + p.name)
        destination = target / p.name
        shutil.copyfile(p, destination)
        content = destination.read_bytes()
        record = {'file': p.name, 'sha256': hashlib.sha256(content).hexdigest(),
                  'bytes': len(content)}
        if p.suffix.lower() == '.gif':
            w, h, frames = gif_frames(content)
            record.update(size=[w, h], frames=frames)
        entries.append(record)
        readme += [f'### {p.name}', '', f'![{p.stem}]({p.name})', '']
    (target / 'gallery.json').write_text(json.dumps({
        'format': 'reverievr-development-gallery-v1',
        'title': title, 'date': str(date.today()), 'project': project,
        'source_commit': revision, 'license': license_name,
        'source_kind': source_kind, 'assets': entries
    }, indent=2) + '\n')
    (target / 'README.md').write_text('\n'.join(readme))
    gallery_index = root / 'README.md'
    gallery_index.parent.mkdir(parents=True, exist_ok=True)
    with gallery_index.open('a', encoding='utf-8') as f:
        f.write(f'\n- [{title}]({project}/{collection}/{batch}/README.md) — '
                f'`{project}/{collection}/{batch}`\n')
    verify_gallery(target)
    return target


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project', required=True)
    p.add_argument('--collection', required=True)
    p.add_argument('--batch', required=True)
    p.add_argument('--source-dir', required=True, type=Path)
    p.add_argument('--gallery-root', type=Path, default=Path('docs/gallery'))
    p.add_argument('--title', required=True)
    p.add_argument('--source-commit', required=True)
    p.add_argument('--license', required=True)
    p.add_argument('--source-kind', required=True)
    a = p.parse_args()
    dest = stage(a.project, a.collection, a.batch, a.source_dir,
                 a.gallery_root, a.title, a.source_commit,
                 a.license, a.source_kind)
    print('Staged gallery:', dest)
    print('Next: git add/commit/push on main, wait for CI, read back commit SHA.')


if __name__ == '__main__':
    main()