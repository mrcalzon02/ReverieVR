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
# Preserve only the reviewed 16 shared PNGs, 3 shared BWM1 props and two
# game-private owner models. Never demand deleted Beckenburg copies again.
TILES=("shingle.png","quay_concrete.png","black_basalt.png",
       "oxidized_steel.png","sandbag_burlap.png","olive_enamel.png",
       "dock_planks.png","mossy_seawall.png","corrugated_roof.png",
       "camo_netting.png","diamond_plate.png","trench_soil.png",
       "reinforced_concrete.png","crate_planks.png","riveted_bulkhead.png",
       "pebble_ground.png")
SHARED_MODELS=("ammo_box_mp_1.bwm","concrete_block_1.bwm","wooden_crate_1.bwm")
OWNER_MODELS=("owner_boat_rowboat.bwm","owner_apc_armor.bwm")
REQUIRED_ART=(
    tuple("native/shared/v1/materials/coastal/"+name for name in TILES)+
    tuple("native/shared/v1/models/psx/"+name for name in SHARED_MODELS)+
    tuple("native/breakwater/beckenburg/"+name for name in OWNER_MODELS)
)
assert len(REQUIRED_ART)==21 and len(set(REQUIRED_ART))==21
def verify_art(z):
    files=[v.filename for v in z.infolist() if not v.is_dir()]
    if len(set(files))!=len(files):
        raise ValueError("Duplicate APK ZIP entry")
    present=set(files)
    missing=sorted("assets/"+p for p in REQUIRED_ART if "assets/"+p not in present)
    if missing:
        raise ValueError("Missing approved Breakwater assets "+str(missing))
    # Guard against importing the entire external asset pack or continuing
    # to ship byte-for-byte duplicate game-private shared model copies.
    actual=set(path[7:] for path in present if
               path.startswith("assets/native/shared/v1/") or
               path.startswith("assets/native/breakwater/beckenburg/"))
    unexpected=sorted(actual-set(REQUIRED_ART))
    if unexpected:
        raise ValueError("Unapproved or redundant game models "+str(unexpected))
    for path in REQUIRED_ART:
        source=ROOT/"app/src/main/assets"/path
        if not source.is_file():
            raise ValueError("Missing original approved asset "+path)
        payload=z.read("assets/"+path)
        if payload!=source.read_bytes():
            raise ValueError("APK asset does not match canonical source "+path)
        if path.endswith(".png"):
            if payload[:8]!=bytes((137,80,78,71,13,10,26,10)) or (
                len(payload)<24 or
                struct.unpack_from(">II",payload,16)!=(64,64)):
                raise ValueError("Bad 64x64 PNG asset "+path)
        else:
            if len(payload)<8 or payload[:4]!=b"BWM1":
                raise ValueError("Invalid compiled BWM1 model "+path)
            triangles=struct.unpack_from("<I",payload,4)[0]
            if triangles==0 or triangles>2048 or len(payload)!=8+27*triangles:
                raise ValueError("Wrong BWM1 model triangle count "+path)
    return len(TILES),len(SHARED_MODELS)+len(OWNER_MODELS)
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
        n_png,n_mesh=verify_art(z)
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
          f", all JNI lifecycle functions, {n_png} exact shared PNGs "
          f"and {n_mesh} approved BWM1 models")

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("--apk",type=Path,required=True)
    ap.add_argument("--target",choices=sorted(TARGETS),default="device")
    args=ap.parse_args()
    try:verify(args.apk,args.target)
    except (ValueError,KeyError,zipfile.BadZipFile,OSError) as error:
        print("BREAKWATER APK REJECTED:",error,file=sys.stderr)
        raise SystemExit(1)
