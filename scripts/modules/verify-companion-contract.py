#!/usr/bin/env python3
"""Local static + optional APK acceptance guard for the native companion probe.

This does not create/publish a release. Checks that one independently built
Android app consumes the REAL native Test Chamber ABI-v1 module. Use --apk
only after a Gradle build; physical device rendering remains a separate gate.
"""
import argparse
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[2]
APPID="io.github.mrcalzon02.reverievr.game.testchamber"
NATIVE_MODULE="reverie_companion_test_chamber"
JAVA_CLASS=("io.github.mrcalzon02.reverievr.game.testchamber"
            ".GameVrActivity")
METHODS=("nativeCreate","nativeSurfaceCreated","nativeSurfaceReleased",
         "nativeResume","nativePause","nativeUpdate","nativeRenderEye",
         "nativeDestroy")
ABI_REL="app/src/main/jni/native/test_chamber_module.cpp"

def contents(path):
    p=ROOT/path
    if not p.is_file():
        raise ValueError("Missing authoritative source: "+str(path))
    return p.read_text(encoding="utf-8")

def required(text,needle,message):
    if needle not in text:
        raise ValueError(message+": "+needle)

def check_source():
    settings=contents("settings.gradle")
    gradle=contents("game-apps/test-chamber/build.gradle")
    manifest=contents("game-apps/test-chamber/src/main/AndroidManifest.xml")
    hub_manifest=contents("app/src/main/AndroidManifest.xml")
    java=contents("game-apps/test-chamber/src/main/java/"+
                  JAVA_CLASS.replace(".","/")+".java")
    bridge=contents("game-apps/test-chamber/src/main/jni/"+
                    "companion_test_chamber_bridge.cpp")
    mk=contents("game-apps/test-chamber/src/main/jni/Android.mk")
    native=contents(ABI_REL)
    registry=contents("app/src/main/java/io/github/mrcalzon02/"+
                      "reverievr/GameModuleRegistry.java")
    header=contents("app/src/main/jni/native/reverie_native_module.h")
    required(settings,'include(":companion-test-chamber")',"Gradle module not registered")
    required(gradle,'applicationId = "'+APPID+'"',"Wrong APK application ID")
    required(gradle,'implementation project(":cardboard")',"Missing VR adapter")
    required(manifest,'android:name=".GameVrActivity"',"Missing game Activity")
    required(manifest,'android:exported="true"',"Hub cannot open companion")
    required(hub_manifest,'<package android:name="'+APPID+'" />',
             "Android cannot see installed game")
    required(registry,'"'+APPID+'"',"Game registry not connected")
    required(mk,"LOCAL_MODULE := "+NATIVE_MODULE,"Wrong native ELF")
    for relative in (ABI_REL,"third_party/openktg/src/gentexture.cpp"):
        required(mk,relative,"Missing actual native module linkage")
    required(native,"reverie_native_module_entry_v1","Native game entry missing")
    required(header,"#define REVERIE_NATIVE_MODULE_ABI_VERSION 1u",
             "Native ABI version changed")
    for method in METHODS:
        required(java,"native "+("boolean" if method in (
            "nativeSurfaceCreated","nativeRenderEye") else
            "long" if method=="nativeCreate" else "void")+
            " "+method+"(","JNI declaration missing")
        required(bridge,"_"+method+"(","JNI implementation missing")
    for callback in ("onNewFrame(","onDrawEye(","onSurfaceCreated(",
                     "onRendererShutdown(","onBackPressed("):
        required(java,callback,"Missing companion game lifecycle callback")
    print("PASS: Gradle module, package identity, Android visibility, "
          "JNI declarations, native ABI module and Cardboard callbacks")

def verify_apk(path):
    if not path.is_file():
        raise ValueError("APK missing: "+str(path))
    with zipfile.ZipFile(path) as archive:
        for arch,elf_class,machine in (
            ("armeabi-v7a",1,40),("arm64-v8a",2,183)):
            filename="lib/"+arch+"/lib"+NATIVE_MODULE+".so"
            try:data=archive.read(filename)
            except KeyError:
                raise ValueError("Missing native APK ABI payload: "+filename)
            if len(data)<64 or data[:4]!=b"\x7fELF" or data[4]!=elf_class:
                raise ValueError("Invalid native ELF header: "+filename)
            end="<" if data[5]==1 else ">"
            if struct.unpack_from(end+"H",data,18)[0]!=machine:
                raise ValueError("Wrong ELF machine architecture: "+filename)
            tool=shutil.which("readelf") or shutil.which("llvm-readelf")
            if tool is None:
                raise ValueError("readelf/llvm-readelf required for native symbol proof")
            import tempfile
            with tempfile.TemporaryDirectory() as td:
                binary=Path(td)/"library.so"
                binary.write_bytes(data)
                proc=subprocess.run([tool,"--dyn-syms","--wide",str(binary)],
                                    text=True,capture_output=True)
                if proc.returncode:
                    raise ValueError("ELF symbol tool failed: "+proc.stderr[:300])
                required(proc.stdout,"reverie_native_module_entry_v1",
                         "Game did not export native ABI")
                required(proc.stdout,"GameVrActivity_nativeCreate",
                         "Game JNI bridge missing")
            print("PASS:",arch,"native ABI entry and JNI bridge are packaged")
    print("APK static checks passed. Install, stereo GL and S9 tests still required.")

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--apk",type=Path,
                        help="Optional independently built Test Chamber APK")
    args=parser.parse_args()
    try:
        check_source()
        if args.apk is not None:verify_apk(args.apk)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        print("FAIL:",error,file=sys.stderr)
        sys.exit(1)

if __name__=="__main__":
    main()
