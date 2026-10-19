#!/usr/bin/env python3
"""Offline provenance and full-mesh-count acceptance for Red Ledger package."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("red_psx",ROOT/"build-red-ledger-psx-pack.py")
red=importlib.util.module_from_spec(spec)
spec.loader.exec_module(red)

def model():
    return struct.pack("<4sIII",b"RLBP",1,3,256)+bytes(3*40+256*256*2)

def populate(source):
    source.mkdir()
    for name in red.EXPECTED:
        (source/(name+".rlprop")).write_bytes(model())

class RedLedgerPSXPackTest(unittest.TestCase):
    def test_exact_fourteen_real_format_models_packaged_with_provenance(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/"private-models"
            populate(source)
            out=root/"red-ledger.rvpack"
            report=red.build(source,out,3,"0.3.0")
            self.assertEqual(report["models"],14)
            self.assertEqual(report["triangles"],14)
            with zipfile.ZipFile(out) as pack:
                meta=json.loads(pack.read("module.json"))
                provenance=json.loads(pack.read("licenses/asset-provenance.json"))
                self.assertEqual(meta["id"],red.GAME)
                self.assertEqual(provenance["count"],14)
                self.assertEqual(len(provenance["files"]),14)
                self.assertTrue(all(row["derived_sha256"] for row in provenance["files"]))
                self.assertEqual(len(meta["files"]),14)
                self.assertIn("beckenburg/wall.rlprop",meta["files"][0]["path"]+
                              " ".join(x["path"] for x in meta["files"]))
    def test_missing_or_extra_asset_fails_before_creating_archive(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); source=root/"private-models"
            populate(source)
            (source/"chair.rlprop").unlink()
            with self.assertRaisesRegex(ValueError,"chair.rlprop"):
                red.build(source,root/"missing.rvpack",1,"0.1.0")
            self.assertFalse((root/"missing.rvpack").exists())
            (source/"chair.rlprop").write_bytes(model())
            (source/"unapproved.rlprop").write_bytes(model())
            with self.assertRaisesRegex(ValueError,"unapproved.rlprop"):
                red.build(source,root/"extra.rvpack",1,"0.1.0")
    def test_invalid_geometry_is_never_built(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/"private-models"
            populate(source)
            bad=bytearray(model())
            struct.pack_into("<f",bad,16,float("nan"))
            (source/"wall.rlprop").write_bytes(bad)
            with self.assertRaisesRegex(ValueError,"Nonfinite"):
                red.build(source,root/"bad.rvpack",1,"0.1.0")
            self.assertFalse((root/"bad.rvpack").exists())
    def test_flat_and_beckenburg_prefixed_inputs_are_equivalent(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/"private-models";source.mkdir()
            nested=source/"beckenburg";nested.mkdir()
            for name in red.EXPECTED:
                (nested/(name+".rlprop")).write_bytes(model())
            report=red.build(source,root/"nested.rvpack",5,"0.5.0")
            self.assertEqual(report["models"],14)
            self.assertEqual(len(red.selected_files(source)),14)

if __name__=="__main__":unittest.main()
