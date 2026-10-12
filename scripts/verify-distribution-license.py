#!/usr/bin/env python3
"""Audit real APK payloads and generate a version-specific licensing manifest.

Stdlib only. No downloads, no automatic license grants, no APK modification.
Every bundled arbitrary asset or native library MUST appear in the registry.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parent.parent
REGISTRY = ROOT / "docs/licensing/ASSET_REGISTRY.json"
BASELINE = ROOT / "docs/dependencies/BASELINE.json"
BAD_RIGHTS = {"review-required", "noncommercial-only"}
WATCH_EXT = (".jpg", ".jpeg", ".png", ".webp", ".mp3",
             ".ogg", ".wav", ".flac", ".ttf", ".otf",
             ".obj", ".mtl", ".glb", ".fbx", ".stl", ".zip")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def fail_if_absent(root, paths, errors):
    for path in paths:
        if not (root / path).is_file():
            errors.append("required license/provenance missing: " + path)


def static_audit(registry, baseline, errors):
    required = ["LICENSE", "LICENSES/MIT.txt",
                "LICENSES/CC-BY-4.0.txt", "LICENSES/CC0-1.0.txt",
                "LICENSES/Apache-2.0.txt", "LICENSES/GPL-2.0-or-later.txt",
                "LICENSES/GPL-3.0-only.txt",
                "LICENSES/LicenseRef-OpenKTG-Public-Domain.txt",
                "LICENSES/LicenseRef-Pending-Asset-Rights.txt",
                "LICENSES/LicenseRef-Daydream-Upstream-Review.txt",
                "LICENSES/LicenseRef-id-Software-Shareware.txt",
                "REUSE.toml",
                "app/src/main/assets/licenses/THIRD_PARTY_NOTICES.txt",
                "app/src/main/assets/models/daydream/THIRD_PARTY.md",
                "third_party/dosbox-pure/README.md",
                "third_party/openktg/LICENSE"]
    # Android's aapt2 resource tree cannot contain arbitrary .license files.
    # The REUSE.toml mapping is the authoritative metadata for resource binaries.
    required += [p["source"] for p in registry["pending_public_assets"]]
    required += [p["source"] for p in registry.get("owner_authorized_assets", [])]
    required += ["docs/licensing/SUNO_AUDIO_PROVENANCE.md",
                 "LICENSES/LicenseRef-Suno-Commercial-Use.txt",
                 "app/src/main/assets/music/catalog.tsv"]
    required += [p["record"] for p in registry["apk_assets"].values()]
    fail_if_absent(ROOT, sorted(set(required)), errors)
    license_data = (ROOT / "LICENSE").read_text(encoding="utf-8") if (
        ROOT / "LICENSE").is_file() else ""
    if ("MIT License" not in license_data
            or "2026 ReverieVR contributors" not in license_data):
        errors.append("approved MIT copyright text missing")
    if (ROOT / baseline["assets"]["steam_controller_archive"]["path"]).exists():
        errors.append("restricted Valve mesh exists under auto-packaged assets")
    # Registry cannot silently grant unreviewed third-party files MIT/CC.
    reuse_text = (ROOT / "REUSE.toml").read_text(encoding="utf-8") if (
        ROOT / "REUSE.toml").is_file() else ""
    for asset in registry["pending_public_assets"]:
        if asset["rights"] == "review-required" and asset["license"] != (
                "LicenseRef-Pending-Asset-Rights"):
            errors.append("review-required asset mislicensed: " + asset["source"])
        if asset["source"] not in reuse_text:
            errors.append("resource binary lacks explicit REUSE annotation: " + asset["source"])
        if asset["required"] and not (ROOT / asset["source"]).is_file():
            errors.append("known existing packaged asset missing: " + asset["source"])
    for asset in registry.get("owner_authorized_assets", []):
        if (asset["license"] != "LicenseRef-Suno-Commercial-Use"
                or asset["rights"] != "commercial-use-authorized"
                or asset.get("free_relicense_verified") is not False):
            errors.append("owner-approved Suno audio has invalid license metadata")
        if asset["source"] not in reuse_text:
            errors.append("owner audio lacks explicit REUSE entry: " + asset["source"])
        if len(asset.get("sha256", "")) != 64:
            errors.append("owner audio SHA-256 missing: " + asset["source"])
    catalog_file = ROOT / "app/src/main/assets/music/catalog.tsv"
    if catalog_file.is_file():
        seen = set()
        count = 0
        for row in catalog_file.read_text(encoding="utf-8").splitlines():
            if not row or row.startswith("#"):
                continue
            cells = row.split("\t")
            if len(cells) != 5:
                errors.append("invalid music catalog row")
                continue
            song_id, title, source, license_id, digest = cells
            if song_id in seen or not title or license_id != "LicenseRef-Suno-Commercial-Use":
                errors.append("invalid or duplicate licensed song: " + song_id)
            seen.add(song_id)
            count += 1
            if source.startswith("resource:"):
                if source != "resource:starry_cereal_menu":
                    errors.append("unapproved built-in music resource: " + source)
                if digest != registry["apk_assets"]["res/raw/starry_cereal_menu.mp3"].get("sha256"):
                    errors.append("existing Starry Cereal checksum mismatch")
            else:
                item = registry["apk_assets"].get("assets/" + source)
                if not item or item.get("sha256") != digest or (
                    item.get("rights") != "commercial-use-authorized"):
                    errors.append("unapproved local music in catalog: " + source)
                music_file = ROOT / "app/src/main/assets" / source
                if not music_file.is_file() or sha256(music_file.read_bytes()) != digest:
                    errors.append("actual music file checksum mismatch: " + source)
        if count > registry.get("music_import_policy", {}).get("max_tracks", 1000):
            errors.append("music catalog exceeds maximum allowed tracks")
    # Eleven owner-original masters have explicit authorization, checksum,
    # size, package registry, and source provenance. Fail closed when any
    # master or package declaration diverges.
    owner_manifest = ROOT / "music/source/manifest.json"
    if not owner_manifest.is_file():
        errors.append("owner-original music manifest missing")
    else:
        inventory = load(owner_manifest)
        tracks = inventory.get("tracks", [])
        if (inventory.get("schema") != 1
                or inventory.get("binary_status") != "source-binaries-checked-in"
                or len(tracks) != 11
                or inventory.get("rights_attestation", {}).get(
                    "owner_confirms_commercial_distribution_rights") is not True
                or inventory.get("rights_attestation", {}).get(
                    "owner_claims_original_work") is not True):
            errors.append("owner original music authorization/track count mismatch")
        import re
        seen_tracks = set()
        for track in tracks:
            title = track.get("title", "")
            track_id = re.sub(r"[^a-z0-9]+", "-", title.lower()).strip("-")
            path = "assets/music/tracks/" + track_id + ".mp3"
            item = registry["apk_assets"].get(path, {})
            source_rel = track.get("relative_path", "")
            master = ROOT / source_rel
            if (not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,79}", track_id)
                    or track_id in seen_tracks
                    or source_rel != "music/source/audio/" + track.get("filename", "")
                    or track.get("filename") != title + ".mp3"
                    or track.get("license") != "LicenseRef-Owner-Original-Music"
                    or item.get("license") != "LicenseRef-Owner-Original-Music"
                    or item.get("rights") != "commercial-use-authorized"
                    or item.get("required") is not True
                    or item.get("sha256") != track.get("sha256")
                    or item.get("source_master") != source_rel
                    or not master.is_file()
                    or master.stat().st_size != track.get("bytes")):
                errors.append("owner music source/registry conflict: " + title)
            elif sha256(master.read_bytes()) != track.get("sha256"):
                errors.append("owner music binary checksum mismatch: " + title)
            seen_tracks.add(track_id)
        owner_index = registry["apk_assets"].get(
            "assets/music/owner_library.tsv", {})
        if owner_index.get("required") is not True:
            errors.append("owner-original packaged music catalog missing")
    for name, item in registry["apk_assets"].items():
        if item["license"].startswith("LicenseRef-") and not (
                ROOT / ("LICENSES/" + item["license"] + ".txt")).is_file():
            errors.append("LicenseRef has no local explanatory text: " + name)
    for name, item in registry["native_libraries"].items():
        if not name.endswith(".so"):
            errors.append("invalid native .so allowlist entry: " + name)
        for term in item["license"].split(" AND "):
            if term.startswith("LicenseRef-") and not (
                    ROOT / ("LICENSES/" + term + ".txt")).is_file():
                errors.append("native LicenseRef text missing: " + name)


def classify(path, registry, *, companion):
    for blocked in registry["blocked_apk_prefixes"]:
        if path.startswith(blocked):
            return "BLOCKED", None
    if path.startswith("lib/") and path.endswith(".so"):
        parts = path.split("/")
        if len(parts) != 3 or parts[1] not in ("armeabi-v7a", "arm64-v8a"):
            return "UNKNOWN", None
        return "native", registry["native_libraries"].get(parts[2])
    if path.startswith("assets/"):
        return "asset", registry["apk_assets"].get(path)
    if path.startswith("res/") and path.lower().endswith(WATCH_EXT):
        # Android and library resource tables may contain additional icons.
        # Every independent distributed binary media asset needs registration.
        if companion and path == "res/mipmap-nodpi/ic_launcher.jpg":
            return "asset", {"component": "companion-icon",
                "license": "LicenseRef-Pending-Asset-Rights",
                "rights": "review-required",
                "record": "REUSE.toml"}
        return "asset", registry["apk_assets"].get(path)
    return "generated-or-code", None


def audit_apk(filename, registry, *, companion, channel, errors):
    path = Path(filename)
    if not path.is_file():
        errors.append("APK missing: " + str(path))
        return None, []
    found = set()
    included = []
    pending = []
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        names = [e.filename for e in entries if not e.is_dir()]
        if len(names) != len(set(names)):
            errors.append(str(path) + ": duplicate ZIP entry names")
        for entry in entries:
            if entry.is_dir():
                continue
            name = entry.filename
            category, item = classify(name, registry, companion=companion)
            if category == "BLOCKED":
                errors.append(str(path) + ": restricted payload " + name)
                continue
            if category in ("asset", "native") and item is None:
                errors.append(str(path) + ": UNREGISTERED " + category + ": " + name)
                continue
            if category == "UNKNOWN":
                errors.append(str(path) + ": unapproved native path: " + name)
                continue
            if item is None:
                continue
            body = archive.read(entry)
            actual = sha256(body)
            if "sha256" in item and actual != item["sha256"]:
                errors.append(name + ": original content SHA-256 mismatch")
            found.add(name)
            record = {"path": name, "component": item["component"],
                      "license": item["license"], "rights": item["rights"],
                      "size": len(body), "sha256": actual,
                      "provenance": item["record"]}
            included.append(record)
            if item["rights"] in BAD_RIGHTS:
                pending.append({"path": name, "status": item["rights"],
                                "license": item["license"]})
    if not companion:
        for name, item in registry["apk_assets"].items():
            if item.get("required") and name not in found:
                errors.append(str(path) + ": missing expected asset " + name)
    if channel == "open" and pending:
        errors.append(str(path) + ": open commercial distribution blocked by "
                      + str(len(pending)) + " unresolved or NC assets")
    return {"file": path.name, "size": path.stat().st_size,
            "sha256": sha256(path.read_bytes()), "items": included}, pending


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", help="built headset APK to inspect")
    parser.add_argument("--controller-apk", help="optional companion APK")
    parser.add_argument("--source-revision", default="local")
    parser.add_argument("--channel", choices=("phone-test", "open"),
                        default="phone-test")
    parser.add_argument("--output", help="JSON release manifest output file")
    parser.add_argument("--gpl-source", help="source archive shipped with release")
    options = parser.parse_args(argv)
    registry = load(REGISTRY)
    baseline = load(BASELINE)
    errors = []
    if registry.get("schema") != 1:
        errors.append("unsupported license registry schema")
    static_audit(registry, baseline, errors)
    if options.controller_apk and not options.apk:
        errors.append("--controller-apk requires --apk")
    if options.output and not options.apk:
        errors.append("--output requires --apk")
    apk = controller = None
    pending = []
    if options.apk:
        apk, p = audit_apk(options.apk, registry, companion=False,
                           channel=options.channel, errors=errors)
        pending.extend(p)
    if options.controller_apk:
        controller, p = audit_apk(options.controller_apk, registry,
                                  companion=True, channel=options.channel,
                                  errors=errors)
        pending.extend(p)
    gpl_source = None
    if options.gpl_source:
        gp = Path(options.gpl_source)
        if not gp.is_file() or gp.stat().st_size < 100:
            errors.append("required GPL source archive missing or empty: " + str(gp))
        else:
            gpl_source = {"file": gp.name, "sha256": sha256(gp.read_bytes()),
                          "size": gp.stat().st_size,
                          "revision": registry["source_disclosure"]["dosbox-pure"]["commit"]}
    if apk:
        includes_dosbox = any(x["component"] == "dosbox-pure"
                             for x in apk["items"])
        if includes_dosbox and not gpl_source:
            errors.append("APK contains GPL DOSBox Pure but no corresponding source archive provided")
    if errors:
        for e in errors:
            print("LICENSE AUDIT FAILED:", e, file=sys.stderr)
        return 1
    output = {
        "schema": 1, "source_revision": options.source_revision,
        "distribution_channel": options.channel,
        "legal_notice": "Component licenses, not a blanket MIT license for combined APK.",
        "claim": "provisional-phone-test" if pending else "inventory-verified",
        "review_required": pending, "headset_apk": apk, "controller_apk": controller,
        "gpl_corresponding_source": gpl_source,
        "toolchain_baseline": baseline["baseline"],
        "record": "docs/licensing/ASSET_REGISTRY.json",
        "limitations": ["No legal determination of JNI/GPL combining scope.",
                        "User-provided music/logo rights and Daydream model still await review.",
                        "Pre-release inventories do not retroactively change old releases."]
    }
    if options.output:
        path = Path(options.output)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    print("ReverieVR license audit PASS:", options.channel,
          "bundles", int(bool(apk)) + int(bool(controller)),
          "review flags", len(pending))
    if pending:
        print("WARNING: release is provisional; review_required recorded in manifest.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
