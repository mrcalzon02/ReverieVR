#!/usr/bin/env python3
"""Fail-closed verifier for ReverieVR's approved dependency baseline.

Uses Python standard library only. Never downloads, installs, updates or
licenses a dependency. Exact-version approval lives in BASELINE.json;
this checker compares it against the actual authoritative source/config.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
BASELINE = ROOT / "docs/dependencies/BASELINE.json"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-toolchain", action="store_true",
                        help="also check installed JDK/Android SDK packages")
    parser.add_argument("--require-full-build", action="store_true",
                        help="require the DOS core and original DOOM archive")
    args = parser.parse_args()
    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    errors = []
    checked = []

    def fail(message):
        errors.append(message)

    def content(path):
        file = ROOT / path
        if not file.is_file():
            fail(f"missing baseline input: {path}")
            return ""
        return file.read_text(encoding="utf-8")

    def contains(path, fragment, label=None):
        value = content(path)
        if fragment not in value:
            fail(f"{label or path}: required pin absent: {fragment!r}")
        else:
            checked.append(label or path)

    def exactly(path, expression, expected, label):
        value = content(path)
        found = re.findall(expression, value, re.M)
        if found != [str(expected)]:
            fail(f"{label}: expected single {expected!r}, found {found!r}")
        else:
            checked.append(label)

    def git_blob(path, expected):
        file = ROOT / path
        if not file.is_file():
            fail(f"missing frozen packaged asset: {path}")
            return
        data = file.read_bytes()
        digest = hashlib.sha1(
            f"blob {len(data)}\0".encode("ascii") + data).hexdigest()
        if digest != expected:
            fail(f"{path}: Git blob SHA does not match approved asset")
        else:
            checked.append(f"content {path}")

    def hash_file(path, expected, label):
        file = ROOT / path
        if not file.is_file():
            fail(f"missing hash-pinned {label}: {path}")
        elif hashlib.sha256(file.read_bytes()).hexdigest() != expected:
            fail(f"{label}: sha256 mismatch")
        else:
            checked.append(label)

    if baseline.get("schema") != 1 or baseline.get("policy") != "reviewed-update-only":
        fail("baseline schema/policy not recognized")
    app = baseline["application"]
    tool = baseline["toolchain"]
    sources = baseline["source_dependencies"]
    coords = baseline["gradle_coordinates"]
    ci = baseline["ci"]
    assets = baseline["assets"]

    # Full, exact Gradle plugin set: additions require an explicit review.
    plugins = dict(re.findall(
        r'\bid\s+["\x27]([\w.]+)["\x27]\s+version\s+["\x27]([^"\x27]+)["\x27]',
        content("build.gradle")))
    if plugins != coords["plugins"]:
        fail(f"Gradle plugin drift: expected {coords['plugins']}, found {plugins}")
    else:
        checked.append("Android/protobuf Gradle plugins")

    content("gradle/wrapper/gradle-wrapper.properties")
    exactly("gradle/wrapper/gradle-wrapper.properties",
            r'^distributionUrl=.*gradle-([0-9.]+)-bin\.zip$',
            tool["gradle"], "Gradle distribution version")
    exactly("gradle/wrapper/gradle-wrapper.properties",
            r'^distributionSha256Sum=([a-f0-9]{64})$',
            tool["gradle_distribution_sha256"], "Gradle distribution SHA-256")
    hash_file("gradle/wrapper/gradle-wrapper.jar",
              tool["gradle_wrapper_jar_sha256"], "Gradle wrapper JAR")

    for p in ("app/build.gradle", "controller-app/build.gradle",
              "cardboard-adapter/build.gradle"):
        for field, key in (("compileSdk", "compile_android_api"),
                           ("targetSdk", "target_android_api"),
                           ("minSdk", "min_android_api")):
            exactly(p, rf'^\s*{field}\s*=\s*(\d+)\s*$',
                    app[key], f"{p} {field}")
        # Assert both source and target compatibility without generated tasks.
        # Both source and target must use the approved Java level.
        occurrences = re.findall(
            r'(?:sourceCompatibility|targetCompatibility)\s*=\s*JavaVersion\.VERSION_(\d+)',
            content(p))
        if occurrences != [tool["java_source"], tool["java_source"]]:
            fail(f"{p}: Java source/target mismatch: {occurrences}")
        else:
            checked.append(f"{p} Java source/target")
    exactly("cardboard-adapter/build.gradle", r'^\s*ndkVersion\s*=\s*"([^"]+)"',
            tool["android_ndk"], "Cardboard NDK")
    contains("build.gradle", '-DCARDBOARDSDK_RENDERING_VULKAN=OFF')
    contains("build.gradle", '-DCARDBOARDSDK_RENDERING_GLESv3=OFF')
    for abi in app["abis"]:
        for p in ("app/build.gradle", "cardboard-adapter/build.gradle"):
            contains(p, f'"{abi}"', f"{p} ABI {abi}")

    # No changing or unreviewed direct Maven dependencies.
    deps = {}
    for p in ("app/build.gradle", "cardboard-adapter/build.gradle",
              "controller-app/build.gradle"):
        src = content(p)
        for group, name, ver in re.findall(
                r'\b(?:implementation|testImplementation|api|compileOnly)\s+'
                r'["\x27]([\w.-]+):([\w.-]+):([^"\x27]+)["\x27]',
                src):
            key = f"{group}:{name}"
            if key in deps and deps[key] != ver:
                fail(f"conflicting direct dependency version: {key}")
            deps[key] = ver
    if deps != coords["runtime_and_test"]:
        fail(f"direct Maven coordinates drift: expected {coords['runtime_and_test']}, found {deps}")
    else:
        checked.append("complete direct Maven dependency allowlist")
    exactly("cardboard-adapter/build.gradle",
            r'artifact\s*=\s*"com\.google\.protobuf:protoc:([^"]+)"',
            coords["build_codegen"]["com.google.protobuf:protoc"], "protoc codegen")

    # Immutable source pins: align all fetch and verification paths.
    contains(".gitmodules", f'url = {sources["cardboard"]["repo"]}')
    contains("BUILDING.md", sources["cardboard"]["git_commit"])
    contains("third_party/README.md", sources["cardboard"]["git_commit"])
    contains("scripts/verify-build-bootstrap.sh",
             sources["cardboard"]["git_commit"])
    contains("scripts/fetch-dosbox-pure.sh",
             f'PIN="{sources["dosbox_pure"]["git_commit"]}"')
    contains("scripts/fetch-dosbox-pure.sh",
             f'REPO_URL="{sources["dosbox_pure"]["repo"]}"')
    contains("scripts/verify-build-bootstrap.sh",
             sources["dosbox_pure"]["git_commit"])
    contains("third_party/dosbox-pure/README.md",
             sources["dosbox_pure"]["git_commit"])
    contains("third_party/openktg/UPSTREAM.md",
             sources["openktg"]["git_commit"])
    for path in ("third_party/openktg/LICENSE",
                 "app/src/main/assets/models/daydream/THIRD_PARTY.md",
                 "app/src/main/assets/models/daydream/UPSTREAM-LICENSE-GPL-3.0.txt",
                 "docs/records/STEAM-CONTROLLER-MESH-PROVENANCE.md"):
        if not (ROOT / path).is_file():
            fail(f"third-party attribution material missing: {path}")
        else:
            checked.append(path)
    for item in baseline["licensing"]["records"]:
        content(item["record"])
    if not baseline["licensing"]["reverievr_original_code"].startswith("MIT;"):
        fail("owner-approved original software license differs from MIT baseline")
    else:
        notice = content("LICENSE")
        if "MIT License" not in notice or "2026 ReverieVR contributors" not in notice:
            fail("root MIT license missing or unexpected")
        else:
            checked.append("scoped owner MIT grant")
    contains("REUSE.toml", 'SPDX-License-Identifier = "MIT"',
             "first-party SPDX/REUSE source classification")
    contains("docs/licensing/ASSET_REGISTRY.json",
             '"license": "LicenseRef-Pending-Asset-Rights"',
             "unverified-asset rights registry")

    for name, item in (("cardboard", sources["cardboard"]),
                       ("dosbox_pure", sources["dosbox_pure"])):
        directory = ROOT / item["location"]
        if (directory / ".git").exists():
            run = subprocess.run(["git", "-C", str(directory), "rev-parse", "HEAD"],
                                 capture_output=True, text=True, check=False)
            if run.returncode or run.stdout.strip() != item["git_commit"]:
                fail(f"{name} checkout does not match approved commit")
            else:
                checked.append(f"{name} checked-out source commit")
        elif args.require_full_build:
            fail(f"{name} pinned source not checked out for full release")

    # The exact packaged byte streams must not move silently.
    restricted = assets["steam_controller_archive"]
    restricted_path = restricted["path"]
    if restricted.get("in_current_tree") is not False:
        fail("baseline must explicitly declare restricted Valve assets absent")
    if (ROOT / restricted_path).exists():
        fail(f"restricted CC BY-NC-SA asset would be bundled in APK: {restricted_path}")
    else:
        checked.append("restricted Valve CAD mesh excluded")
    contains("app/build.gradle",
             'restrictedSteamControllerArchive.isFile()',
             "Gradle rejects accidental Valve CAD asset restore")
    item = assets["daydream_controller_obj"]
    git_blob(item["path"], item["git_blob_sha1"])
    doom = assets["doom_shareware"]
    contains("scripts/fetch-doom-shareware.sh", doom["sha256"])
    contains("scripts/fetch-doom-shareware.sh", doom["source"])
    if (ROOT / doom["location"]).is_file():
        hash_file(doom["location"], doom["sha256"], "DOOM shareware byte hash")
    elif args.require_full_build:
        fail("full release requires pinned unmodified DOOM shareware")

    # Offline tooling never becomes a surprise app runtime dependency.
    requirements = content("tools/steam_controller/requirements.txt")
    found = dict(re.findall(r'^([A-Za-z0-9_-]+)==([A-Za-z0-9_.]+)$',
                           requirements, re.M))
    if found != baseline["offline_asset_construction"]["python_requirements"]:
        fail(f"offline Python tools drift: {found}")
    else:
        checked.append("exact offline Python tool versions")
    lock = json.loads(content("tools/steam_controller/package-lock.json"))
    expected_npm = baseline["offline_asset_construction"]
    package = lock.get("packages", {}).get("node_modules/meshoptimizer", {})
    if (package.get("version") != expected_npm["npm_requirements"]["meshoptimizer"]
            or package.get("integrity") != expected_npm["npm_lock_integrity"]
            or lock.get("packages", {}).get("", {}).get("dependencies", {})
            != expected_npm["npm_requirements"]):
        fail("npm offline meshoptimizer package version/integrity drift")
    else:
        checked.append("offline meshoptimizer npm tarball integrity")

    # The ONE authorized release workflow contains two jobs: a lightweight
    # gate and the signed APK builder. Check every job's exact runner pin;
    # never accept additional jobs or a mixed runner family silently.
    workflow = content(".github/workflows/phone-test-release.yml")
    workflow_jobs = workflow.split("\njobs:\n", 1)
    if len(workflow_jobs) != 2:
        fail("GitHub APK workflow is missing the top-level jobs section")
    else:
        jobs_source = workflow_jobs[1]
        boundaries = list(re.finditer(
            r"^  ([a-z][a-z0-9_-]*):[ \t]*$",
            jobs_source, re.M))
        job_names = [match.group(1) for match in boundaries]
        if job_names != ["gate", "build-and-release"]:
            fail(f"GitHub APK workflow jobs changed: {job_names}")
        for index, match in enumerate(boundaries):
            next_start = (boundaries[index + 1].start()
                          if index + 1 < len(boundaries)
                          else len(jobs_source))
            body = jobs_source[match.end():next_start]
            runners = re.findall(r"^    runs-on:[ \t]*([^\s]+)[ \t]*$",
                                 body, re.M)
            if runners != [ci["runner"]]:
                fail(f"GitHub runner for {match.group(1)}: "
                     f"expected {ci['runner']!r}, found {runners!r}")
            else:
                checked.append(f"GitHub runner family ({match.group(1)})")
    for name, sha in ci["actions"].items():
        contains(".github/workflows/phone-test-release.yml",
                 f"uses: {name}@{sha}", f"workflow action pin {name}")
    contains(".github/workflows/phone-test-release.yml",
             f'"platforms;android-{tool["android_sdk_platform"]}"')
    contains(".github/workflows/phone-test-release.yml",
             f'"build-tools;{tool["android_build_tools"]}"')
    contains(".github/workflows/phone-test-release.yml",
             f'"ndk;{tool["android_ndk"]}"')
    contains(".github/workflows/phone-test-release.yml",
             f'"cmake;{tool["cmake"]}"')
    contains(".github/workflows/phone-test-release.yml",
             'java-version: "17"')
    # Workflow modification is separately authorized; this verifier does
    # not pretend it has been inserted into the existing CI process.
    checked.append("existing workflow selectors checked (NOT immutable SHA pins)")

    if args.check_toolchain:
        run = subprocess.run(["java", "-version"], capture_output=True,
                             text=True, check=False)
        java_info = run.stderr + run.stdout
        if run.returncode or not re.search(r'\b(?:version\s+")?17(?:\.|"|\s)', java_info):
            fail(f"installed Java is not JDK 17: {java_info[:180]!r}")
        else:
            checked.append("installed JDK major 17")
        sdk = Path(os.environ.get("ANDROID_SDK_ROOT")
                   or os.environ.get("ANDROID_HOME") or "")
        if not sdk.is_dir() or str(sdk) == ".":
            fail("ANDROID_SDK_ROOT/ANDROID_HOME missing or invalid")
        else:
            for package in (
                f"platforms/android-{tool['android_sdk_platform']}",
                f"build-tools/{tool['android_build_tools']}",
                f"ndk/{tool['android_ndk']}",
                f"cmake/{tool['cmake']}",
            ):
                if not (sdk / package).is_dir():
                    fail(f"installed Android SDK component missing: {package}")
                else:
                    checked.append(f"installed {package}")

    if errors:
        print("ReverieVR dependency baseline FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print(f"ReverieVR baseline {baseline['baseline']} OK: {len(checked)} pins checked.")
    print("Scope: exact direct pins and packaged asset bytes; transitive Gradle"
          " artifacts, JDK patch and entire runner image still need separate locks.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
