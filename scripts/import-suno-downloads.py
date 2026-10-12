#!/usr/bin/env python3
"""Stage ONLY owner-verified official Suno downloads into the offline APK.

No Suno API, scraping, CDN extraction, or account credentials. User supplies
approved MP3/WAV downloads and an exact song list. All bytes are verified
before staging. The import amends the real licensing registry and REUSE file.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parent.parent
PROFILE = "https://suno.com/@jovianpostpunkrevival7375"
ASSETS = ROOT / "app/src/main/assets"
CATALOG = ASSETS / "music/catalog.tsv"
REGISTRY = ROOT / "docs/licensing/ASSET_REGISTRY.json"
REUSE = ROOT / "REUSE.toml"
MAX_TRACKS = 1000
MAX_SIZE_BYTES = 1250 * 1024 * 1024


def hash_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def stage(manifest_path, sources, *, dry_run):
    spec = json.loads(manifest_path.read_text(encoding="utf-8"))
    if spec.get("schema") != 1 or spec.get("profile") != PROFILE:
        raise ValueError("Artist profile or import schema mismatch")
    songs = spec.get("tracks")
    if not isinstance(songs, list) or not songs or len(songs) > MAX_TRACKS:
        raise ValueError("Expected 1..1000 explicitly approved downloads")
    if not sources.is_dir():
        raise ValueError("Official downloaded-audio directory does not exist")

    registry = json.loads(REGISTRY.read_text(encoding="utf-8"))
    original_catalog = CATALOG.read_text(encoding="utf-8")
    reuse = REUSE.read_text(encoding="utf-8")
    current_ids = {line.split("\t")[0] for line in original_catalog.splitlines()
                   if line and not line.startswith("#")}
    added = []
    total = 0
    seen = set()
    for song in songs:
        uid = song.get("id")
        title = song.get("title", "")
        filename = song.get("downloaded_file", "")
        permalink = song.get("suno_url", "")
        expected = song.get("sha256", "")
        if not isinstance(uid, str) or not re.fullmatch(r"[a-z0-9][a-z0-9_-]{0,79}", uid):
            raise ValueError("Invalid stable song ID")
        if uid in seen or uid in current_ids:
            raise ValueError("Duplicate or already packaged track ID: " + uid)
        seen.add(uid)
        if not isinstance(title, str) or not title.strip() or len(title) > 170 or (
                "\t" in title or "\r" in title or "\n" in title):
            raise ValueError("Invalid music title: " + uid)
        if not isinstance(filename, str) or not re.fullmatch(
                r"[A-Za-z0-9._ -]{1,190}\.(mp3|wav)", filename, re.I):
            raise ValueError("Only plain official .mp3/.wav filenames allowed: " + uid)
        if not isinstance(permalink, str) or not re.fullmatch(
                r"https://suno\.com/(song|s)/[A-Za-z0-9-]{8,100}", permalink):
            raise ValueError("Track must have its own canonical Suno song link: " + uid)
        if any(song.get(key) is not True for key in (
                "owner_attests_commercial_rights",
                "official_paid_suno_download",
                "no_third_party_remix_or_unlicensed_samples")):
            raise ValueError("Rights or approved-download declaration missing: " + uid)
        if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected):
            raise ValueError("Missing SHA-256 digest: " + uid)
        candidate = sources / filename
        if candidate.is_symlink() or not candidate.is_file():
            raise ValueError("Downloaded source missing or symlinked: " + filename)
        actual = hash_file(candidate)
        if actual != expected:
            raise ValueError("Suno downloaded file SHA-256 mismatch: " + filename)
        size = candidate.stat().st_size
        if size < 2048 or size > 48 * 1024 * 1024:
            raise ValueError("Track file outside accepted size budget: " + filename)
        total += size
        ext = candidate.suffix.lower()
        rel = f"music/tracks/{uid}{ext}"
        target = ASSETS / rel
        if target.exists():
            raise ValueError("Refusing overwrite of existing packaged music: " + str(target))
        added.append((song, candidate, target, rel))
    if len(current_ids) + len(added) > MAX_TRACKS or total > MAX_SIZE_BYTES:
        raise ValueError("Music catalog or single-APK asset budget exceeded")

    if dry_run:
        print(f"Verified {len(added)} official owner-authorized downloads; "
              f"{total} bytes; no repository files changed.")
        return

    # All validation above must pass before the first project mutation.
    next_catalog = original_catalog.rstrip("\n") + "\n"
    for song, candidate, target, rel in added:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(candidate, target)
        if hash_file(target) != song["sha256"]:
            target.unlink(missing_ok=True)
            raise ValueError("Staged file does not match approved SHA-256")
        next_catalog += "\t".join((
            song["id"], song["title"], rel,
            "LicenseRef-Suno-Commercial-Use", song["sha256"])) + "\n"
        registry["apk_assets"]["assets/" + rel] = {
            "component": "suno-owner-authorized-music",
            "license": "LicenseRef-Suno-Commercial-Use",
            "rights": "commercial-use-authorized",
            "required": True,
            "sha256": song["sha256"],
            "record": "docs/licensing/SUNO_AUDIO_PROVENANCE.md",
            "suno_url": song["suno_url"],
            "free_relicense_verified": False
        }
        registry.setdefault("owner_authorized_assets", []).append({
            "source": "app/src/main/assets/" + rel,
            "apk": "assets/" + rel,
            "name": song["title"],
            "license": "LicenseRef-Suno-Commercial-Use",
            "rights": "commercial-use-authorized",
            "required": True,
            "sha256": song["sha256"],
            "notes": "Owner attests paid-tier permitted official download; no third-party remix/samples.",
            "suno_url": song["suno_url"],
            "free_relicense_verified": False,
        })
        reuse += (
            "\n[[annotations]]\n"
            f'path = ["app/src/main/assets/{rel}"]\n'
            'precedence = "override"\n'
            'SPDX-FileCopyrightText = "NOASSERTION"\n'
            'SPDX-License-Identifier = "LicenseRef-Suno-Commercial-Use"\n'
        )

    CATALOG.write_text(next_catalog, encoding="utf-8")
    REGISTRY.write_text(json.dumps(registry, indent=2) + "\n",
                        encoding="utf-8")
    REUSE.write_text(reuse, encoding="utf-8")
    print(f"Added {len(added)} rights-attested tracks to ReverieVR's offline "
          f"native music catalog ({total} bytes). Run dependency/license tests "
          "and review the git diff before committing or shipping.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True,
                        help="owner-reviewed list of official paid Suno downloads")
    parser.add_argument("--download-dir", type=Path, required=True,
                        help="local files downloaded using Suno's approved download command")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        stage(args.manifest, args.download_dir, dry_run=args.dry_run)
    except (ValueError, OSError, KeyError, json.JSONDecodeError) as exc:
        print("Suno import refused: " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
