#!/usr/bin/env python3
"""Regression against the six ACTUAL checked-in owner-model payloads.

No network or fabricated asset fixtures. Does not prove an installed APK.
"""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
import json

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("red_ledger_owned",HERE/"build-red-ledger-owned-pack.py")
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
ver=mod.tool("verify-content-pack.py")
build=mod.tool("build-content-pack.py")

class OwnerRedLedgerPackAcceptance(unittest.TestCase):
    def test_real_six_sources_and_hashes(self):
        for relative in mod.FILES:
            with self.subTest(file=relative):
                path=mod.ROOT/"app/src/main/assets/native/red-ledger"/relative
                self.assertTrue(path.is_file())
                self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),
                                 mod.SHA256[relative])
        self.assertEqual(len(mod.FILES),6)
    def test_signed_ready_content_candidate_is_deterministic_and_verified(self):
        with tempfile.TemporaryDirectory(prefix="red-ledger-six-") as d:
            a,b=Path(d)/"a.rvpack",Path(d)/"b.rvpack"
            first=mod.build(a,1,"0.1.0")
            again=mod.build(b,1,"0.1.0")
            self.assertEqual(a.read_bytes(),b.read_bytes())
            self.assertEqual(first["sha256"],again["sha256"])
            self.assertEqual((first["assets"],first["triangles"]),(6,13710))
            self.assertEqual(ver.validate(a)["sha256"],first["sha256"])
            with zipfile.ZipFile(a) as archive:
                files=json.loads(archive.read("module.json"))["files"]
                self.assertEqual({row["path"] for row in files},{
                    "assets/native/red-ledger/"+relative for relative in mod.FILES})
    def test_full_room_content_merge_preserves_all_existing_private_models(self):
        # Synthetic room data tests merger behavior; they do NOT stand for
        # the actual owner's 14 private PSX source meshes.
        import struct
        with tempfile.TemporaryDirectory(prefix="red-ledger-merge-") as d:
            root=Path(d)
            private=root/"private"
            private.mkdir()
            data=struct.pack("<4sIII",b"RLBP",1,3,256)+bytes(3*40+256*256*2)
            for name in mod.ROOMS:
                (private/(name+".rlprop")).write_bytes(data)
            output=root/"full.rvpack"
            result=mod.build(output,3,"0.1.2",private)
            self.assertEqual(result["assets"],20)
            self.assertEqual(result["private_rooms_merged"],14)
            with zipfile.ZipFile(output) as archive:
                paths=archive.namelist()
                for name in mod.ROOMS:
                    self.assertIn("assets/native/red-ledger/beckenburg/"+name+".rlprop",paths)
                self.assertIn("assets/native/red-ledger/beckenburg/patron_gopnick.rlprop",paths)
            (private/"chair.rlprop").unlink()
            with self.assertRaisesRegex(ValueError,"14-model complete"):
                mod.build(root/"incomplete.rvpack",4,"0.1.3",private)

    def test_unapproved_fbx_and_damaged_owner_model_are_refused(self):
        with tempfile.TemporaryDirectory(prefix="owner-negative-") as d:
            root=Path(d)
            (root/"models").mkdir()
            wrong=root/"models/owned_unknown.fbx"
            wrong.write_bytes((mod.ROOT/"app/src/main/assets/native/red-ledger/models/owned_cigar.fbx").read_bytes())
            with self.assertRaisesRegex(ValueError,"Unapproved owner bar"):
                build.checked_assets(root,"red-ledger")
            wrong.unlink()
            model=root/"models/owned_cigar.fbx"
            raw=(mod.ROOT/"app/src/main/assets/native/red-ledger/models/owned_cigar.fbx").read_bytes()
            model.write_bytes(bytes([0])+raw[1:])
            with self.assertRaisesRegex(ValueError,"Invalid owner model FBX"):
                build.checked_assets(root,"red-ledger")
            with self.assertRaisesRegex(ValueError,"Invalid owner model FBX"):
                ver.validate(self._invalid_pack(root,model))
    @staticmethod
    def _invalid_pack(root,model):
        import zipfile,json
        data=model.read_bytes()
        name="assets/native/red-ledger/models/owned_cigar.fbx"
        pack=root/"invalid.rvpack"
        with zipfile.ZipFile(pack,"w") as arc:
            arc.writestr("module.json",json.dumps({
                "schema":1,"kind":"content","id":mod.GAME,
                "revision":1,"version":"0.1.0",
                "files":[{"path":name,"bytes":len(data),
                    "sha256":hashlib.sha256(data).hexdigest()}]}))
            arc.writestr(name,data)
        return pack

if __name__=="__main__":
    unittest.main()
