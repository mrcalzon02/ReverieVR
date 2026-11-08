#!/usr/bin/env python3
"""Host JVM regressions. Real bindings/runtime, named Android/JNI doubles.

Requires a full JDK, C++ compiler, GLES2 headers/library, and JUnit4/Hamcrest classpath. This is a
bounded diagnostic, not a replacement for APK/Android/headset validation.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = "io/github/mrcalzon02/reverievr"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--junit-classpath", required=True)
    parser.add_argument("--java-home", type=Path, required=True)
    parser.add_argument("--javac", default="javac")
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    stubs = {
        "android/content/res/AssetManager.java": "package android.content.res; public class AssetManager {}",
        PACKAGE + "/ReverieLog.java": """package io.github.mrcalzon02.reverievr;
final class ReverieLog {
static boolean isDevelopment(){return false;}
static void milestone(String a,String b){} static void dev(String a,String b){}
static void incident(String a,String b){} static void error(String a,String b,Throwable c){}
}""",
        PACKAGE + "/ReveriePreferences.java": """package io.github.mrcalzon02.reverievr;
final class ReveriePreferences { HomeAvatarProfile getHomeAvatarProfile(){return HomeAvatarProfile.DEFAULT;} }""",
        PACKAGE + "/HomeAvatarProfile.java": """package io.github.mrcalzon02.reverievr;
final class HomeAvatarProfile { static final HomeAvatarProfile DEFAULT=new HomeAvatarProfile(); int[] forNativeModules(){return new int[15];} }""",
    }
    stubpaths = []
    for name, body in stubs.items():
        path = out / "stubs" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body + "\n")
        stubpaths.append(str(path))
    classes = out / "classes"
    classes.mkdir(exist_ok=True)
    results = []

    def run(name, command):
        completed = subprocess.run(command, cwd=ROOT, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        (out / (name + ".log")).write_text(completed.stdout)
        results.append({"name": name, "command": command,
                        "returncode": completed.returncode})
        temp = out / "results.json.tmp"
        with temp.open("w") as file:
            json.dump(results, file, indent=2)
            file.write("\n"); file.flush(); os.fsync(file.fileno())
        temp.replace(out / "results.json")
        print(name + ": " + str(completed.returncode), flush=True)
        print(completed.stdout[-4000:], flush=True)
        if completed.returncode:
            raise SystemExit(completed.returncode)

    names = ["InputBindingEngine", "BindingInput", "BindingProfile",
        "BindingTransform", "InputBinding", "VirtualInputBus", "VirtualInputSnapshot",
        "VirtualKey", "VirtualOutput", "VirtualOutputKind", "BuiltInBindingProfiles",
        "BindingProfileCodec", "BindingProfileTuner", "NativeModuleRuntime",
        "RvPackInstaller", "RvPackManifest", "RvPackAssetValidator", "GameModuleRegistry",
        "SharedNativeAssetPaths", "SitkaWalkTouchpadControls", "TouchpadLocomotionGate"]
    tests = ["InputBindingEngineTest", "VirtualInputBusTest",
        "BuiltInBindingProfilesTest", "HostedInputFocusTest", "TouchpadLocomotionGateTest"]
    sources = [str(ROOT / "app/src/main/java" / PACKAGE / (n + ".java")) for n in names]
    sources += [str(ROOT / "app/src/test/java" / PACKAGE / (n + ".java")) for n in tests]
    sources += [str(ROOT / "scripts/audit/probes/NativeRuntimePauseProbe.java")]
    run("compile-java", [args.javac, "--release", "17", "-cp", args.junit_classpath,
        "-d", str(classes)] + stubpaths + sources)
    run("binding-tests", ["java", "-cp", str(classes) + os.pathsep + args.junit_classpath,
        "org.junit.runner.JUnitCore"] + [PACKAGE.replace("/", ".") + "." + n for n in tests])
    run("compile-counting-jni", [os.environ.get("CXX", "g++"), "-std=c++17",
        "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
        "-I" + str(args.java_home / "include"), "-I" + str(args.java_home / "include/linux"),
        str(ROOT / "scripts/audit/probes/native_runtime_counting_jni.cpp"),
        "-o", str(out / "libreverie_native_host.so")])
    run("runtime-pause-tests", ["java", "-Djava.library.path=" + str(out),
        "-cp", str(classes), PACKAGE.replace("/", ".") + ".NativeRuntimePauseProbe",
        str(out / "session")])
    run("compile-production-host-probe", [os.environ.get("CXX", "g++"), "-std=c++17",
        "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1",
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "-I" + str(ROOT / "app/src/main/jni/native"),
        "-I" + str(ROOT / "scripts/audit/probes/host_android_shims"),
        "-I" + str(args.java_home / "include"), "-I" + str(args.java_home / "include/linux"),
        str(ROOT / "scripts/audit/probes/native_host_save_repair_probe.cpp"),
        "-lGLESv2", "-ldl", "-pthread", "-o", str(out / "native-host-probe")])
    run("production-host-save-exit-tests", [str(out / "native-host-probe"),
        str(out / ("private-storage-" + str(time.time_ns())))])
    run("compile-gl-diagnostic-bounds", [os.environ.get("CXX", "g++"), "-std=c++17",
        "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-I" + str(ROOT / "app/src/main/jni/native"),
        str(ROOT / "app/src/test/native/native_gl_diagnostic_bounds_test.cpp"),
        "-o", str(out / "gl-diagnostic-bounds")])
    run("gl-diagnostic-bounds-tests", [str(out / "gl-diagnostic-bounds")])


if __name__ == "__main__":
    main()
