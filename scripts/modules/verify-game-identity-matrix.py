#!/usr/bin/env python3
"""Fail closed when the 15-game release registry drifts from real C++/Java modules.

This does not imply every game has a separate compiled companion APK.
It verifies data-only content/release identities and bundled native binaries.
"""
import json
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
MATRIX=ROOT/"modules/game-identity-matrix.json"

def check():
    obj=json.loads(MATRIX.read_text("utf-8"))
    if obj.get("schema")!=1 or not isinstance(obj.get("games"),list):
        raise ValueError("Invalid game identity matrix schema")
    items=obj["games"]
    ids=set()
    packages=set()
    namespaces=set()
    libraries=set()
    by_id={}
    for x in items:
        if not isinstance(x,dict) or set(x)!={"id","content_namespace",
           "companion_package","native_library","development_only",
           "companion_build_target"}:
            raise ValueError("Malformed game entry")
        game=x["id"]
        if (not re.fullmatch(r"[a-z0-9][a-z0-9-]{2,79}",game) or
            not re.fullmatch(r"[a-z0-9][a-z0-9-]{2,79}",x["content_namespace"]) or
            not re.fullmatch(
                r"io\.github\.mrcalzon02\.reverievr\.game\.[a-z0-9]+",
                x["companion_package"]) or
            not re.fullmatch(r"libreverie_module_[a-z0-9_]+\.so",
                x["native_library"]) or
            type(x["development_only"]) is not bool or
            x["companion_build_target"] not in (
                None,":companion-test-chamber",":companion-breakwater")):
            raise ValueError("Malformed identity: "+str(game))
        for unique,seen in ((game,ids),(x["companion_package"],packages),
                            (x["content_namespace"],namespaces),
                            (x["native_library"],libraries)):
            if unique in seen:raise ValueError("Duplicate game identity: "+unique)
            seen.add(unique)
        actual=x["companion_build_target"]
        expected={ "procedural-test-chamber":":companion-test-chamber",
                   "breakwater-battery":":companion-breakwater" }.get(game)
        if actual!=expected:
            raise ValueError("Missing or unverified independent game target: "+game)
        by_id[game]=x
    if len(items)!=15:
        raise ValueError("Expected 15 authoritative game identities, got "+str(len(items)))

    java=(ROOT/"app/src/main/java/io/github/mrcalzon02/reverievr/"
          "GameModuleRegistry.java").read_text("utf-8")
    java_rows=re.findall(
        r'new Entry\("([^"]+)"\s*,\s*"[^"]+"\s*,\s*"([^"]+)"\s*,'
        r'\s*"([^"]+)"\s*,\s*(true|false)\)',java)
    if len(java_rows)!=len(items):
        raise ValueError("Java registry entry parsing/count mismatch")
    for game,ns,pkg,dev in java_rows:
        if (game not in by_id or
            (by_id[game]["content_namespace"],by_id[game]["companion_package"],
             by_id[game]["development_only"])!=(ns,pkg,dev=="true")):
            raise ValueError("Java registry diverges: "+game)
    if set(game for game,*_ in java_rows)!=ids:
        raise ValueError("Java catalog has duplicate or unknown games")

    native=(ROOT/"app/src/main/jni/native/reverie_native_host.cpp").read_text("utf-8")
    block=native.split("static const BuiltInModuleSpec kBuiltIns[] = {",1)[1].split(
        "\n};",1)[0]
    native_rows=re.findall(
        r'"([a-z0-9-]+)"\s*,\s*"[^"]+"\s*,\s*'
        r'"(libreverie_module_[a-z0-9_]+\.so)"',block)
    if len(native_rows)!=len(items) or set(game for game,_ in native_rows)!=ids:
        raise ValueError("Compiled native host list differs from registry")
    for game,lib in native_rows:
        if by_id[game]["native_library"]!=lib:
            raise ValueError("Wrong C++ native game binary: "+game)

    mk=(ROOT/"app/src/main/jni/Android.mk").read_text("utf-8")
    targets=re.findall(r"^LOCAL_MODULE := (reverie_module_[a-z0-9_]+)$",
                       mk,re.MULTILINE)
    expected={x["native_library"][3:-3] for x in items}
    if len(targets)!=len(items) or set(targets)!=expected:
        raise ValueError("Android.mk does not build every required native game")
    settings=(ROOT/"settings.gradle").read_text("utf-8")
    for target,path in (
            ("companion-test-chamber","game-apps/test-chamber"),
            ("companion-breakwater","game-apps/breakwater")):
        if f'include(":{target}")' not in settings:
            raise ValueError("Independent native game build missing: "+target)
        for suffix in ("build.gradle","src/main/AndroidManifest.xml",
                       "src/main/jni/Android.mk"):
            if not (ROOT/path/suffix).is_file():
                raise ValueError("Independent game has no real build source: "+
                                 target+"/"+suffix)
    print("PASS:",len(items),
          "game IDs, content namespaces, native libraries and companion "
          "package identities match across JSON, Java, C++ and Android.mk")
    return {x["id"]:x["content_namespace"] for x in items}

if __name__=="__main__":
    try:check()
    except (ValueError,KeyError,IndexError,OSError,json.JSONDecodeError) as exc:
        print("GAME MATRIX REJECTED:",exc,file=sys.stderr)
        raise SystemExit(1)
