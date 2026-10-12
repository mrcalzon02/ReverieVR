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
