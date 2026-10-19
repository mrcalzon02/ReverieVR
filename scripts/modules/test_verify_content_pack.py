#!/usr/bin/env python3
"""No-network validation tests for game-content release ZIPs."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

script=Path(__file__).with_name("verify-content-pack.py")
spec=importlib.util.spec_from_file_location("content_validator",script)
validator=importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

class ContentReleaseVerifierTests(unittest.TestCase):
    ASSET="assets/native/red-ledger/beckenburg/wall.rlprop"
    GAME="between-deliveries-red-ledger"

    @classmethod
    def model(cls):
        return struct.pack("<4sIII",b"RLBP",1,3,256)+bytes(3*40+256*256*2)

    def archive(self,folder,*,bad_hash=False,extra=None,include_payload=True,duplicate=False):
        path=Path(folder)/"content.rvpack"
        model=self.model()
        metadata={"schema":1,"kind":"content","id":self.GAME,"version":"0.1.0","revision":1,
          "files":[{"path":self.ASSET,"bytes":len(model),"sha256":("0"*64 if bad_hash else hashlib.sha256(model).hexdigest())}]}
        with zipfile.ZipFile(path,"w") as z:
            z.writestr("module.json",json.dumps(metadata))
            if include_payload:z.writestr(self.ASSET,model)
            if extra:z.writestr(extra,b"extra")
            if duplicate:z.writestr(self.ASSET,model)
        return path

    def test_real_model_is_accepted(self):
        with tempfile.TemporaryDirectory() as d:
            result=validator.validate(self.archive(d))
            self.assertEqual((result["assets"],result["triangles"]),(1,1))

    def test_nan_position_rejected_with_matching_hash(self):
        with tempfile.TemporaryDirectory() as folder:
            original=self.archive(folder)
            with zipfile.ZipFile(original) as source:
                manifest=json.loads(source.read("module.json"))
                data=bytearray(source.read(self.ASSET))
            struct.pack_into("<f",data,16,float("nan"))
            manifest["files"][0]["sha256"]=hashlib.sha256(data).hexdigest()
            modified=Path(folder)/"nonfinite.rvpack"
            with zipfile.ZipFile(modified,"w") as z:
                z.writestr("module.json",json.dumps(manifest))
                z.writestr(self.ASSET,data)
            with self.assertRaisesRegex(ValueError,"Nonfinite"):
                validator.validate(modified)

    def test_out_of_range_uv_rejected_even_with_valid_digest(self):
        with tempfile.TemporaryDirectory() as folder:
            original=self.archive(folder)
            with zipfile.ZipFile(original) as source:
                manifest=json.loads(source.read("module.json"))
                data=bytearray(source.read(self.ASSET))
            struct.pack_into("<f",data,16+3*4,1.25)
            manifest["files"][0]["sha256"]=hashlib.sha256(data).hexdigest()
            modified=Path(folder)/"bad-uv.rvpack"
            with zipfile.ZipFile(modified,"w") as z:
                z.writestr("module.json",json.dumps(manifest))
                z.writestr(self.ASSET,data)
            with self.assertRaisesRegex(ValueError,"UV/color"):
                validator.validate(modified)

    def test_changed_manifest_hash_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaisesRegex(ValueError,"size/hash mismatch"):
                validator.validate(self.archive(d,bad_hash=True))

    def test_other_games_assets_cannot_be_smuggled_in(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaisesRegex(ValueError,"Unapproved game asset"):
                validator.validate(self.archive(d,extra="assets/native/omega-front/tank.png"))

    def test_duplicate_assets_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaisesRegex(ValueError,"duplicate ZIP entries"):
                validator.validate(self.archive(d,duplicate=True))

    def test_missing_models_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaisesRegex(ValueError,"Invalid or duplicate ZIP entries"):
                validator.validate(self.archive(d,include_payload=False))

if __name__=="__main__":unittest.main()
