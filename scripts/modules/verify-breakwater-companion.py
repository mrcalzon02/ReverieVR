#!/usr/bin/env python3
"""Verify actual independently built Breakwater Battery APK bytes, not claims."""
import argparse
from pathlib import Path
import struct
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[2]
PACKAGE="io.github.mrcalzon02.reverievr.game.breakwater"
LIB="libreverie_companion_breakwater.so"
JAVA="Java_"+PACKAGE.replace(".","_")+"_GameVrActivity_"
METHODS=(
    "nativeCreate","nativeSurfaceCreated","nativeSurfaceReleased",
    "nativeResume","nativePause","nativeUpdate","nativeRenderEye","nativeDestroy"
)
MODELS=("ammo_box_mp_1.bwm","concrete_block_1.bwm","wooden_crate_1.bwm")
TARGETS={"device":{"armeabi-v7a":(1,40),"arm64-v8a":(2,183)},
         "emulator":{"x86_64":(2,62)}}

def verify(apk,target):
    if not apk.is_file() or apk.stat().st_size<10000:
        raise ValueError("Missing genuine Breakwater APK")
    expected=TARGETS[target]
    with zipfile.ZipFile(apk) as z:
        if z.testzip() is not None:raise ValueError("Corrupt Android APK ZIP")
        files=set(z.namelist())
        present={s.split("/")[1] for s in files
                 if s.startswith("lib/") and s.endswith("/"+LIB)}
        if present!=set(expected):
            raise ValueError(f"Wrong independent game ABIs: {present} not {expected}")
        if "classes.dex" not in files or "AndroidManifest.xml" not in files:
            raise ValueError("Missing real Android app Activity bytecode/manifest")
        if not any(s.startswith("assets/native/breakwater/beckenburg/")
                   for s in files):
            raise ValueError("Game has no bundled authentic Beckenburg models")
        for name in MODELS:
            path="native/breakwater/beckenburg/"+name
            source=ROOT/"app/src/main/assets"/path
            member="assets/"+path
            if member not in files or z.read(member)!=source.read_bytes():
                raise ValueError("APK did not preserve approved actual BWM1 "+name)
        for abi,(klass,machine) in expected.items():
            entry="lib/"+abi+"/"+LIB
            elf=z.read(entry)
            if len(elf)<1024 or elf[:5]!=b"\x7fELF"+bytes([klass]) or (
                    struct.unpack_from("<H",elf,18)[0]!=machine):
                raise ValueError("Wrong native ELF class/CPU for "+abi)
            missing=[name for name in METHODS if (JAVA+name).encode() not in elf]
            if missing:raise ValueError("JNI bridge missing methods "+str(missing))
            if b"reverie_native_module_entry_v1" not in elf:
                raise ValueError("Actual authoritative game code absent from "+abi)
    print("PASS: standalone Breakwater APK, "+",".join(expected)+
          ", all JNI lifecycle functions, 3 unmodified game meshes")

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--apk",type=Path,required=True)
    ap.add_argument("--target",choices=sorted(TARGETS),default="device")
    args=ap.parse_args()
    try:verify(args.apk,args.target)
    except (ValueError,KeyError,zipfile.BadZipFile,OSError) as error:
        print("BREAKWATER APK REJECTED:",error,file=sys.stderr)
        raise SystemExit(1)
