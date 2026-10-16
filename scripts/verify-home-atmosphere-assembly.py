#!/usr/bin/env python3
"""Verify VR Home atmosphere source registration and optional packaged APK.

This has no network dependency, GitHub Action, or third-party Python module.
No APK evidence is claimed unless --apk is explicitly provided.
For phoneTest/debug (not R8-obfuscated release) inspect the DEX descriptors.
"""
import argparse
import re
import sys
from pathlib import Path
from zipfile import ZipFile, BadZipFile

ROOT = Path(__file__).resolve().parent.parent
JAVA = ROOT / "app/src/main/java/io/github/mrcalzon02/reverievr"


def source(path: Path) -> str:
    if not path.is_file():
        raise AssertionError(f"Missing canonical source: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def contains(text: str, *anchors: str) -> None:
    for anchor in anchors:
        if anchor not in text:
            raise AssertionError(f"Missing assembly contract: {anchor}")


def verify_source() -> None:
    geometry = source(JAVA / "HomeAtmosphereGeometry.java")
    renderer = source(JAVA / "HomeEnvironmentRenderer.java")
    shell = source(JAVA / "VrShellRenderer.java")
    activity = source(JAVA / "VrActivity.java")
    sound = source(JAVA / "HomeAmbientAudio.java")
    environments = source(JAVA / "HomeEnvironment.java")
    tests = source(ROOT / "app/src/test/java/io/github/mrcalzon02/reverievr/HomeAtmosphereGeometryTest.java")
    gradle = source(ROOT / "app/build.gradle")
    docs = source(ROOT / "docs/project/HOME_ENVIRONMENT_ATMOSPHERE.md")

    contains(environments, "WHITE_ROOM(", "FOREST_GLADE(", "DUNE_BEACH(")
    contains(geometry, "static float[] skyBand(", "static float[] distantHills(",
             "static float[] highClouds(", "static float[] windowedRearWall(",
             "static float[] windowSky(", "static float[] windowHorizon(",
             "static float[] windowFrames(", "static float[] woodlandWildlife(",
             "static float[] shoreWildlife(", "static float[] flyingBird(")
    contains(renderer, "mesh(HomeAtmosphereGeometry.windowedRearWall())",
             "draw(windowSky,", "draw(windowHorizon,", "draw(windowFrames,",
             "drawWindowBirds();", "drawOutdoorSky(true);",
             "drawOutdoorSky(false);", "draw(forestAnimals,", "draw(beachAnimals,",
             "drawFlyingBirds(true);", "drawFlyingBirds(false);",
             "void setAmbientFrameTime(long nanos)")
    contains(shell, "homeEnvironmentRenderer.setAmbientFrameTime(frameNanos);",
             "host.onHomeAtmosphereChanged(ambientEnvironment, ambientActive);")
    contains(activity, "new HomeAmbientAudio()", "homeAmbientAudio.setEnvironment(",
             "homeAmbientAudio.resumeForLifecycle()",
             "homeAmbientAudio.pauseForLifecycle()", "homeAmbientAudio.close()")
    contains(sound, "AudioTrack.MODE_STATIC", "setLoopPoints(0,pcm.length,-1)",
             "HomeEnvironment.FOREST_GLADE", "HomeEnvironment.DUNE_BEACH",
             "void pauseForLifecycle()", "void resumeForLifecycle()")
    contains(tests, "skyIsFiniteAndInsideFortyMetreFrustum",
             "windowsHaveRealLayerDepthAndReasonableTriangleBudgets",
             "crittersAndBirdsAreBoundedAndDeterministic",
             "allAmbientBedsAreDistinctAndSeamSafe")
    contains(gradle, 'id "com.android.application"', 'testImplementation "junit:junit:4.13.2"')
    contains(docs, "candidate intake only", "handset acceptance pending")
    if "java.srcDirs" in gradle or "java.setSrcDirs" in gradle:
        raise AssertionError("Explicit Java source roots need a manual inclusion review")

    if shell.index("homeEnvironmentRenderer.setAmbientFrameTime(frameNanos);") > shell.index(
            "private void onDrawEyeInternal("):
        raise AssertionError("Bird animation must advance before either eye")

    print("PASS: atmosphere implementation, audio lifecycle and JUnit tests "
          "are wired into the normal Android Java source tree")
    print("PASS: CC0 recordings are documented intake candidates, "
          "not claimed as bundled files")


def verify_apk(path: Path) -> None:
    if not path.is_file():
        raise AssertionError(f"APK does not exist: {path}")
    try:
        with ZipFile(path) as apk:
            dex_names = [n for n in apk.namelist()
                         if re.fullmatch(r"classes(?:[2-9][0-9]*)?\.dex", n)]
            if not dex_names:
                raise AssertionError("APK has no classes.dex")
            dex = b"".join(apk.read(n) for n in dex_names)
            # phoneTest/debug have minifyEnabled=false; obfuscated release
            # requires a mapped class/feature instrumentation check instead.
            for class_name in ("HomeAtmosphereGeometry", "HomeAmbientAudio",
                               "HomeEnvironmentRenderer", "VrShellRenderer",
                               "VrActivity"):
                if class_name.encode("ascii") not in dex:
                    raise AssertionError(
                        f"DEX lacks {class_name}; use unminified phoneTest/debug APK "
                        "or separately validate R8 mapping")
    except BadZipFile as error:
        raise AssertionError(f"Not a valid APK ZIP: {path}") from error
    print("PASS: all five Home atmosphere Java types are in APK DEX (unminified)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", type=Path, help="optional unminified phoneTest/debug APK")
    args = parser.parse_args()
    try:
        verify_source()
        if args.apk:
            verify_apk(args.apk)
        else:
            print("NOT CHECKED: no APK supplied; source inclusion is not device acceptance")
    except (AssertionError, OSError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
