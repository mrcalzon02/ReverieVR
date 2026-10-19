#!/usr/bin/env python3
"""Offline pre-upload identity and APK publisher certificate gate regressions."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile
from unittest.mock import patch

def load(name):
    path=Path(__file__).with_name(name)
    spec=importlib.util.spec_from_file_location(name.replace("-","_"),path)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

identity=load("release-package-identity.py")
publisher=load("publish-module-release.py")
DIGEST="ab"*32
OTHER="cd"*32

class ReleasePackageIdentityTest(unittest.TestCase):
    def pack(self,root,id="classic-chess",version="0.1.0",
             revision=1,extra=None):
        path=Path(root)/"game.rvpack"
        manifest={"schema":1,"kind":"content","id":id,
                  "version":version,"revision":revision}
        with zipfile.ZipFile(path,"w") as z:
            z.writestr("module.json",json.dumps(manifest))
            z.writestr("assets/native/classic-chess/probe.json","{}")
            if extra:z.writestr("module.json",json.dumps(extra))
        return path

    def test_actual_registered_game_and_version_accepted(self):
        with tempfile.TemporaryDirectory() as root:
            receipt=identity.validate("module-classic-chess-v0.1.0",
                                      self.pack(root))
            self.assertEqual((receipt["game"],receipt["revision"]),
                             ("classic-chess",1))

    def test_other_games_tag_cannot_take_valid_game_payload(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError,"disagrees"):
                identity.validate("module-classic-go-v0.1.0",self.pack(root))

    def test_tag_version_must_match_manifest_version(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError,"disagrees"):
                identity.validate("module-classic-chess-v0.2.0",self.pack(root))

    def test_unknown_game_even_with_self_consistent_tag_is_blocked(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError,"registered game"):
                identity.validate("module-classic-fabricated-v0.1.0",self.pack(root))

    def test_duplicate_manifest_not_accepted(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError,"absent or duplicated"):
                identity.validate("module-classic-chess-v0.1.0",
                                  self.pack(root,extra={"id":"classic-go"}))

    def test_apk_requires_known_companion_target(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk"
            path.write_bytes(b"test APK fixture; metadata comes from aapt")
            with self.assertRaisesRegex(ValueError,"no proven companion"):
                identity.validate("module-classic-chess-v0.1.0",path,DIGEST)

    def test_apk_must_have_trusted_signer_digest(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk";path.write_bytes(b"x")
            with self.assertRaisesRegex(ValueError,"certificate digest"):
                identity.validate("module-breakwater-battery-v0.1.0",path)

    def test_wrong_companion_package_is_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk";path.write_bytes(b"x")
            def fake(command):
                return ("package: name='io.github.mrcalzon02.reverievr.game.testchamber' "
                        "versionCode='14' versionName='0.1.0'")
            with self.assertRaisesRegex(ValueError,"APK identity"):
                identity.validate("module-breakwater-battery-v0.1.0",
                                  path,DIGEST,fake)

    def test_wrong_companion_version_is_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk";path.write_bytes(b"x")
            def fake(command):
                return ("package: name='io.github.mrcalzon02.reverievr.game.breakwater' "
                        "versionCode='14' versionName='0.1.1'")
            with self.assertRaisesRegex(ValueError,"APK identity"):
                identity.validate("module-breakwater-battery-v0.1.0",
                                  path,DIGEST,fake)

    def test_correct_apk_identity_signed_by_other_key_is_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk";path.write_bytes(b"x")
            def fake(command):
                if command[0]=="aapt":
                    return ("package: name='io.github.mrcalzon02.reverievr.game.breakwater' "
                            "versionCode='14' versionName='0.1.0'\n")
                return "Signer #1 certificate SHA-256 digest: "+OTHER
            with self.assertRaisesRegex(ValueError,"signer differs"):
                identity.validate("module-breakwater-battery-v0.1.0",
                                  path,DIGEST,fake)

    def test_correct_signed_companion_package_is_accepted(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/"game.apk";path.write_bytes(b"x")
            def fake(command):
                if command[0]=="aapt":
                    return ("package: name='io.github.mrcalzon02.reverievr.game.breakwater' "
                            "versionCode='14' versionName='0.1.0'\n")
                return "Signer #1 certificate SHA-256 digest: "+DIGEST
            receipt=identity.validate("module-breakwater-battery-v0.1.0",
                                      path,DIGEST,fake)
            self.assertEqual(receipt["revision"],14)
            self.assertEqual(receipt["kind"],"companion-apk")
            self.assertEqual(receipt["publisher_cert_sha256"],DIGEST)

    def test_publisher_main_wires_identity_preflight_before_network(self):
        body=Path(publisher.__file__).read_text()
        first=body.index("release_identity=identity.validate(")
        upload=body.index("release=ensure_draft(args.tag")
        self.assertLess(first,upload)
        self.assertIn('"game_id":release_identity["game"]',body)

if __name__=="__main__":
    unittest.main()
