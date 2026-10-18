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
import json
import wave
import hashlib

ROOT=Path(__file__).resolve().parents[2]
APPID="io.github.mrcalzon02.reverievr.game.testchamber"
NATIVE_MODULE="reverie_companion_test_chamber"
JAVA_CLASS=("io.github.mrcalzon02.reverievr.game.testchamber"
            ".GameVrActivity")
METHODS=("nativeCreate","nativeAddModelMesh","nativeSetFocusedModel","nativeSurfaceCreated","nativeSurfaceReleased",
         "nativeResume","nativePause","nativeUpdate","nativeRenderEye",
         "nativeDestroy")
ABI_REL="app/src/main/jni/native/test_chamber_module.cpp"
ASSET_ROOT=ROOT/"game-apps/test-chamber/src/main/assets"
APP_MODELS=("crate","chair","shelf","table","door")
GAME_EFFECTS=("select","wood_knock","metal_clang","coin_chime","mechanism","beacon")

def check_fixture_assets():
    total_tris=0
    for model in APP_MODELS:
        file=ASSET_ROOT/"models"/(model+".glb")
        if not file.is_file():raise ValueError("Game model missing: "+str(file))
        data=file.read_bytes()
        if len(data)<32 or data[:4]!=b"glTF":
            raise ValueError("Missing actual glTF GLB data for "+model)
        if struct.unpack_from("<II",data,4)!=(2,len(data)):
            raise ValueError("GLB version or file length mismatch for "+model)
        size,kind=struct.unpack_from("<II",data,12)
        if kind!=0x4e4f534a or size<2 or size>len(data)-28:
            raise ValueError("Invalid GLB JSON chunk for "+model)
        obj=json.loads(data[20:20+size])
        views=obj["bufferViews"]
        accessors=obj["accessors"]
        images=obj["images"]
        materials=obj["materials"]
        textures=obj["textures"]
        meshes=obj["meshes"]
        if not meshes or not images or not materials:
            raise ValueError("Not a textured game mesh: "+model)
        bin_offset=20+size
        length,kind=struct.unpack_from("<II",data,bin_offset)
        if kind!=0x004e4942 or bin_offset+8+length>len(data):
            raise ValueError("Missing GLB binary payload: "+model)
        for mesh in meshes:
            for primitive in mesh.get("primitives",[]):
                a=primitive["attributes"]
                v=accessors[a["POSITION"]]
                uv=accessors[a["TEXCOORD_0"]]
                i=accessors[primitive["indices"]]
                if (v["componentType"]!=5126 or uv["componentType"]!=5126 or
                    v["count"]!=uv["count"] or i["count"]<3 or
                    i["count"]>18000 or i["count"]%3!=0):
                    raise ValueError("Invalid GLB triangle/UV mesh "+model)
                total_tris+=i["count"]//3
                material=materials[primitive["material"]]
                tex=material["pbrMetallicRoughness"]["baseColorTexture"]["index"]
                img=images[textures[tex]["source"]]
                if img.get("mimeType")!="image/png":
                    raise ValueError("Unsupported embedded atlas for "+model)
                view=views[img["bufferView"]]
                off=bin_offset+8+view.get("byteOffset",0)
                if data[off:off+8]!=b"\x89PNG\r\n\x1a\n":
                    raise ValueError("Model atlas is not PNG: "+model)
    for effect in GAME_EFFECTS:
        file=ASSET_ROOT/"sfx"/(effect+".wav")
        if not file.is_file():raise ValueError("Missing game audio: "+effect)
        with wave.open(str(file),"rb") as clip:
            if (clip.getnchannels()!=1 or clip.getsampwidth()!=2 or
                clip.getframerate()!=22050 or
                clip.getnframes()<2000 or clip.getnframes()>22050):
                raise ValueError("Wrong audible PCM effect format: "+effect)
    return total_tris

def contents(path):
    p=ROOT/path
    if not p.is_file():
        raise ValueError("Missing authoritative source: "+str(path))
    return p.read_text(encoding="utf-8")

def required(text,needle,message):
    if needle not in text:
        raise ValueError(message+": "+needle)

def check_scene_transform_contract():
    """Gaze hitboxes must use precisely the native GLES2 placement matrix."""
    java=contents("game-apps/test-chamber/src/main/java/io/github/mrcalzon02/"
                  "reverievr/game/testchamber/TestChamberTargeting.java")
    renderer=contents("game-apps/test-chamber/src/main/jni/"
                      "companion_showcase_renderer.cpp")
    def extract(source, marker):
        start=source.find(marker)
        if start<0:
            raise ValueError("Missing model placement table: "+marker)
        end=source.find("};",start)
        if end<0:
            raise ValueError("Unterminated model placement table: "+marker)
        segment=source[start:end]
        rows=[]
        for raw in re.findall(r"\\{\\s*([-+\\d.fF,\\s]+)\\s*\\}",segment):
            fields=[x.strip().rstrip("fF") for x in raw.split(",") if x.strip()]
            if len(fields)==4:
                rows.append(tuple(float(x) for x in fields))
        if len(rows)!=5:
            raise ValueError("Expected five render/target transforms: "+marker)
        return rows
    gaze=extract(java,"TRANSFORMS={")
    draw=extract(renderer,"locations[5][4]={")
    if gaze!=draw:
        raise ValueError("Java gaze bounds do not match native GLES2 placements")

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
    required(mk,"companion_showcase_renderer.cpp","Real model renderer not linked")
    required(gradle,'noCompress += ["wav"]',"PCM game audio must be seekable")
    required(gradle,'if(hasSigner) signingConfig = signingConfigs.publisher',
             "Companion cannot use the hub's signing identity")
    required(gradle,'dependsOn("requirePublisherSigner")',
             "Companion release does not enforce signing")
    required(java,"TestChamberModels.load(","Approved GLB loader not invoked")
    required(java,"new TestChamberSounds(","No packaged game sounds loaded")
    required(bridge,"showcase.Draw(","Stereo game mesh renderer not called")
    required(java,"TestChamberTargeting.pick(headView)","Gaze pointing not wired")
    required(java,"TestChamberTargeting.includePrimitive(",
             "Imported GLB geometry is not registered for gaze picking")
    check_scene_transform_contract()
    required(java,"isActivationKey(","Gamepad input dispatch missing")
    required(java,"onGenericMotionEvent(","Bluetooth/controller pointer handling missing")
    required(java,"new TestChamberSounds(this,getAssets())",
             "Android audio focus must receive Activity Context")
    sounds=contents("game-apps/test-chamber/src/main/java/"+
        JAVA_CLASS.replace(".","/").replace("GameVrActivity","TestChamberSounds")+".java")
    required(sounds,"AudioFocusRequest.Builder","Sound focus lifecycle unavailable")
    required(sounds,"manager.requestAudioFocus(focusRequest)",
             "Audio cannot request focus on resume")
    required(sounds,"manager.abandonAudioFocusRequest(focusRequest)",
             "Audio focus not abandoned on pause")
    required(sounds,"maybeWelcome()","Audible diagnostics cannot prove readiness")
    required(java,"nativeSetFocusedModel(instance,target)","Gaze focus not reaching renderer")
    required(contents("game-apps/test-chamber/src/main/java/"+
        JAVA_CLASS.replace(".","/").replace("GameVrActivity","TestChamberSounds")+".java"),
        "TestChamberTargeting.cueFor(","Sound effects not mapped to physical models")
    required(contents("game-apps/test-chamber/src/main/java/"+
        JAVA_CLASS.replace(".","/").replace("GameVrActivity","TestChamberTargeting")+".java"),
        "static int pick(float[] view)","Headless gaze selector unavailable")
    for relative in (ABI_REL,"third_party/openktg/src/gentexture.cpp"):
        required(mk,relative,"Missing actual native module linkage")
    required(native,"reverie_native_module_entry_v1","Native game entry missing")
    required(header,"#define REVERIE_NATIVE_MODULE_ABI_VERSION 1u",
             "Native ABI version changed")
    for method in METHODS:
        required(java,"native "+("boolean" if method in (
            "nativeSurfaceCreated","nativeRenderEye","nativeAddModelMesh") else
            "long" if method=="nativeCreate" else "void")+
            " "+method+"(","JNI declaration missing")
        required(bridge,"_"+method+"(","JNI implementation missing")
    for callback in ("onNewFrame(","onDrawEye(","onSurfaceCreated(",
                     "onRendererShutdown(","onBackPressed("):
        required(java,callback,"Missing companion game lifecycle callback")
    triangles=check_fixture_assets()
    print("PASS: five embedded Beckenburg GLB assets, %d triangles, "
          "six PCM sound effects, stereo GLES2/JNI lifecycle" % triangles)
    print("PASS: Gradle module, package identity, Android visibility, "
          "JNI declarations, native ABI module and Cardboard callbacks")

def verify_apk(path):
    if not path.is_file():
        raise ValueError("APK missing: "+str(path))
    with zipfile.ZipFile(path) as archive:
        for model in APP_MODELS:
            name="assets/models/"+model+".glb"
            if name not in archive.namelist():
                raise ValueError("APK omitted owned 3D model: "+name)
            actual=hashlib.sha256(archive.read(name)).digest()
            expected=hashlib.sha256(
                (ASSET_ROOT/"models"/(model+".glb")).read_bytes()).digest()
            if actual!=expected:
                raise ValueError("Packaged 3D model differs from verified source: "+name)
        for cue in GAME_EFFECTS:
            name="assets/sfx/"+cue+".wav"
            info=archive.getinfo(name) if name in archive.namelist() else None
            if info is None:
                raise ValueError("APK omitted audible effect: "+name)
            if info.compress_type!=zipfile.ZIP_STORED:
                raise ValueError("APK compressed PCM WAV; SoundPool openFd will fail: "+name)
            if hashlib.sha256(archive.read(name)).digest()!=hashlib.sha256(
                    (ASSET_ROOT/"sfx"/(cue+".wav")).read_bytes()).digest():
                raise ValueError("Packaged WAV differs from verified source: "+name)
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
                for method in METHODS:
                    required(proc.stdout,"GameVrActivity_"+method,
                             "Compiled native JNI entry missing for "+method)
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
