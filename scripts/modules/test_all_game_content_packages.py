#!/usr/bin/env python3
"""Release-pack builder acceptance for every supported ReverieVR game identity.

These are tiny synthetic local TEST FIXTURES, not publishable game content.
Real online updates must be based on approved game assets and signed releases.
"""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]

def tool(name):
    source=ROOT/"scripts/modules"/name
    spec=importlib.util.spec_from_file_location(name.replace("-","_"),source)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

builder=tool("build-content-pack.py")
verifier=tool("verify-content-pack.py")
matrix=json.loads((ROOT/"modules/game-identity-matrix.json").read_text("utf-8"))
games=matrix["games"]

class AllGameContentPackagesTest(unittest.TestCase):
    def test_every_registered_game_builds_independent_verified_revision(self):
        self.assertEqual(15,len(games))
        with tempfile.TemporaryDirectory(prefix="reverie-game-pack-matrix-") as temp:
            root=Path(temp)
            signatures=set()
            for game in games:
                ident=game["id"]
                source=root/ident/"assets"
                source.mkdir(parents=True)
                (source/"sample.json").write_text(
                    json.dumps({"game":ident,"revision":1}),encoding="utf-8")
                output=root/ident/"release.rvpack"
                result=builder.build(ident,source,output,1,"0.1.0")
                validated=verifier.validate(output)
                self.assertEqual(ident,validated["game"])
                self.assertEqual(1,validated["revision"])
                self.assertEqual(1,validated["assets"])
                self.assertEqual(result["sha256"],validated["sha256"])
                self.assertNotIn(result["sha256"],signatures)
                signatures.add(result["sha256"])
            self.assertEqual(len(games),len(signatures))

    def test_unknown_classic_and_unknown_native_games_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/"assets"
            source.mkdir()
            (source/"dummy.json").write_text("{}",encoding="utf-8")
            for game in ("classic-fabricated","new-unregistered-game",
                         "classic-chess-evil"):
                with self.subTest(game=game):
                    with self.assertRaisesRegex(ValueError,"not in the authoritative"):
                        builder.build(game,source,root/"invalid.rvpack",1,"0.1.0")

    def test_bad_semantic_version_rejected_before_writing(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/"assets"
            source.mkdir()
            (source/"dummy.json").write_text("{}",encoding="utf-8")
            output=root/"bad.rvpack"
            with self.assertRaisesRegex(ValueError,"semantic"):
                builder.build("classic-chess",source,output,1,"not-a-version")
            self.assertFalse(output.exists())

    def test_breakwater_real_mesh_format_is_downloadable_and_corruption_rejected(self):
        import struct
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/"assets"/"beckenburg"
            source.mkdir(parents=True)
            # One BWM1 triangle, three valid signed XYZ/color vertices.
            valid=b"BWM1"+struct.pack("<I",1)+bytes(27)
            mesh=source/"concrete_block_1.bwm"
            mesh.write_bytes(valid)
            output=root/"breakwater.rvpack"
            built=builder.build("breakwater-battery",root/"assets",
                                output,1,"0.1.0")
            checked=verifier.validate(output)
            self.assertEqual(1,checked["triangles"])
            self.assertEqual(built["sha256"],checked["sha256"])
            mesh.write_bytes(valid[:-1])
            with self.assertRaisesRegex(ValueError,"Invalid BWM1 model header"):
                builder.build("breakwater-battery",root/"assets",
                              root/"corrupt.rvpack",2,"0.1.1")

    def test_wrong_game_asset_namespace_rejected_by_verifier(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            source=root/"assets"
            source.mkdir()
            (source/"sample.json").write_text("{}",encoding="utf-8")
            output=root/"valid.rvpack"
            builder.build("classic-chess",source,output,1,"0.1.0")
            import zipfile
            import io
            corrupted=root/"wrong.rvpack"
            with zipfile.ZipFile(output) as orig, zipfile.ZipFile(corrupted,"w") as dst:
                for item in orig.infolist():
                    name=item.filename
                    if name=="assets/native/classic-chess/sample.json":
                        name="assets/native/classic-go/sample.json"
                    dst.writestr(name,orig.read(item))
            with self.assertRaisesRegex(ValueError,"Unapproved game asset"):
                verifier.validate(corrupted)

if __name__=="__main__":
    unittest.main()
