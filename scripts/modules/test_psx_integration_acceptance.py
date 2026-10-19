#!/usr/bin/env python3
"""Use real checked-in Breakwater PSX mesh bytes for offline content acceptance.

The source mesh blob IDs were independently read back from main on 2026-10-18.
Never confuse passing these tests with a signed pack, APK or VR render proof.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
GAME="breakwater-battery"
BWM={
    "ammo_box_mp_1.bwm":"06f34332b7c28f3dcf3e8018eba1d5d911900cee",
    "concrete_block_1.bwm":"d4c689a9de820698d29fbc2cb4189788c67b1e93",
    "wooden_crate_1.bwm":"2c7e79a71ab856f09dff09f9eb5bea90f2d59a4d",
    "owner_boat_rowboat.bwm":"089a616c268188f5f37ea68c5cdd3fe6689b3889",
}

def tool(name):
    spec=importlib.util.spec_from_file_location(name.replace("-","_"),HERE/name)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

BUILDER=tool("build-content-pack.py")
VERIFY=tool("verify-content-pack.py")

def source():
    return ROOT/"app/src/main/assets/native/breakwater"

def malformed(source_pack,destination,target):
    with zipfile.ZipFile(source_pack) as initial,zipfile.ZipFile(destination,"w") as edited:
        for entry in initial.infolist():
            name=entry.filename
            data=initial.read(entry)
            if name.endswith("ammo_box_mp_1.bwm"):
                if target=="path":
                    name="assets/native/red-ledger/beckenburg/ammo_box_mp_1.bwm"
                elif target=="bytes":
                    data=data[:-1]
            edited.writestr(name,data)

class PSXIntegrationAcceptance(unittest.TestCase):
    def test_four_checked_in_geometry_assets_have_known_blob_hashes(self):
        for name,blob_sha in BWM.items():
            data=(source()/"beckenburg"/name).read_bytes()
            self.assertEqual(
                hashlib.sha1(b"blob "+str(len(data)).encode()+bytes([0])+data).hexdigest(),
                blob_sha,
                "Source mesh changed; re-review original Beckenburg provenance and render")
            self.assertEqual(data[:4],b"BWM1")
            self.assertEqual(len(data),8+struct.unpack_from("<I",data,4)[0]*27)
        triangles=sum(struct.unpack_from("<I",(source()/"beckenburg"/name).read_bytes(),4)[0]
                      for name in BWM)
        self.assertEqual(triangles,1560)

    def test_pack_and_independent_verify_are_reproducible(self):
        with tempfile.TemporaryDirectory(prefix="psx-authentic-") as folder:
            a=Path(folder)/"one.rvpack"
            b=Path(folder)/"two.rvpack"
            first=BUILDER.build(GAME,source(),a,1,"0.1.0")
            second=BUILDER.build(GAME,source(),b,1,"0.1.0")
            validated=VERIFY.validate(a)
            self.assertEqual(a.read_bytes(),b.read_bytes())
            self.assertEqual(first["sha256"],second["sha256"])
            self.assertEqual(first["sha256"],validated["sha256"])
            self.assertEqual(validated["assets"],4)
            self.assertEqual(validated["triangles"],1560)
            with zipfile.ZipFile(a) as archive:
                manifest=json.loads(archive.read("module.json"))
                self.assertEqual(len(manifest["files"]),3)
                self.assertEqual({row["path"].rsplit("/",1)[-1]
                                  for row in manifest["files"]},set(BWM))
                for row in manifest["files"]:
                    self.assertEqual(hashlib.sha256(archive.read(row["path"])).hexdigest(),
                                     row["sha256"])

    def test_mutated_real_bytes_or_foreign_game_namespace_are_rejected(self):
        with tempfile.TemporaryDirectory(prefix="psx-negative-") as folder:
            a=Path(folder)/"original.rvpack"
            BUILDER.build(GAME,source(),a,1,"0.1.0")
            path=Path(folder)/"foreign.rvpack"
            malformed(a,path,"path")
            with self.assertRaisesRegex(ValueError,"Unapproved game asset"):
                VERIFY.validate(path)
            malformed(a,path,"bytes")
            with self.assertRaisesRegex(ValueError,"Invalid BWM1 model triangle count"):
                VERIFY.validate(path)

    def test_corrupt_private_room_models_do_not_enter_release(self):
        with tempfile.TemporaryDirectory(prefix="psx-rlbp-") as folder:
            assets=Path(folder)/"red"
            assets.mkdir()
            src=struct.pack("<4sIII",b"RLBP",1,3,256)+bytes(3*40+256*256*2)
            file=assets/"wall.rlprop"
            file.write_bytes(src)
            out=Path(folder)/"room.rvpack"
            BUILDER.build("between-deliveries-red-ledger",assets,out,1,"0.1.0")
            self.assertEqual(VERIFY.validate(out)["triangles"],1)
            damaged=bytearray(src)
            struct.pack_into("<f",damaged,16,float("nan"))
            file.write_bytes(damaged)
            with self.assertRaisesRegex(ValueError,"Nonfinite"):
                BUILDER.build("between-deliveries-red-ledger",assets,
                              Path(folder)/"nan.rvpack",2,"0.1.1")
            struct.pack_into("<f",damaged,16,0.0)
            struct.pack_into("<f",damaged,16+3*4,1.25)
            file.write_bytes(damaged)
            with self.assertRaisesRegex(ValueError,"invalid UV/color"):
                BUILDER.build("between-deliveries-red-ledger",assets,
                              Path(folder)/"uv.rvpack",2,"0.1.1")

if __name__=="__main__":
    unittest.main()
