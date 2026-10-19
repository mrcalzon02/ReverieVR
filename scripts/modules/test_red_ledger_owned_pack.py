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
