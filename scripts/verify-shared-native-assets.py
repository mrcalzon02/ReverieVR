#!/usr/bin/env python3
"""Offline SHA/format/reference guard for the shared native game assets."""
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/"shared-assets/catalog-v1.json").read_text())
assert manifest["schema"]=="reverie.shared-asset-catalog/1"
assert len(manifest["entries"])==9
paths=set()
for entry in manifest["entries"]:
    relative=entry["path"]
    assert relative.startswith("app/src/main/assets/native/shared/v1/")
    assert relative not in paths
    paths.add(relative)
    data=(ROOT/relative).read_bytes()
    assert len(data)==entry["size_bytes"], relative
    git_id=hashlib.sha1(("blob %d\0"%len(data)).encode()+data).hexdigest()
    assert git_id==entry["git_blob_sha1"], relative
    if entry["sha256"]:
        assert hashlib.sha256(data).hexdigest()==entry["sha256"], relative
    if entry["kind"]=="image/png":
        assert data[:8]==b"\x89PNG\r\n\x1a\n", relative
        assert struct.unpack(">II",data[16:24])==tuple(entry["dimensions"])
    else:
        assert data[:4]==b"BWM1", relative
        count=struct.unpack("<I",data[4:8])[0]
        assert 0<count<=2048 and len(data)==8+count*27, relative
    assert not (ROOT/entry["source_original"]).exists(), entry["source_original"]
# Each consumer must genuinely address the common paths, not copy the art.
authored=(ROOT/"app/src/main/jni/native/breakwater_authored_materials.h").read_text()
beckenburg=(ROOT/"app/src/main/jni/native/breakwater_beckenburg_mesh.h").read_text()
omega=(ROOT/"app/src/main/jni/native/omega_front_texture_renderer.h").read_text()
scope=(ROOT/"app/src/main/jni/native/reverie_native_shared_asset_catalog.h").read_text()
for entry in manifest["entries"]:
    assert entry["logical_path"] in scope, entry["id"]
    if entry["kind"]=="image/png":
        assert entry["logical_path"] in authored, entry["id"]
    else:
        assert entry["logical_path"] in beckenburg, entry["id"]
for name in ("quay_concrete.png","oxidized_steel.png"):
    assert ("native/shared/v1/materials/coastal/"+name) in omega
    assert not (ROOT/"app/src/main/assets/native/omega-front/shared"/name).exists()
print("PASS: 9 common assets, exact content hashes, scope and two live native consumers")
