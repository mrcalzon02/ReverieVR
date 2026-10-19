#!/usr/bin/env python3
"""No-key, no-network signed-catalog security and release-binding tests."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

script=Path(__file__).with_name("sign-catalog.py")
spec=importlib.util.spec_from_file_location("reverie_catalog_signer",script)
signer=importlib.util.module_from_spec(spec)
spec.loader.exec_module(signer)

def record(id="classic-chess",kind="content",version="0.1.0"):
    return {
        "id":id,"kind":kind,"channel":"testing","version":version,
        "revision":1,"size_bytes":4096,"sha256":"0"*64,
        "url":"https://github.com/mrcalzon02/ReverieVR/releases/download/"
             f"module-{id}-v{version}/chess.rvpack" if kind=="content" else
             "https://github.com/mrcalzon02/ReverieVR/releases/download/"
             f"module-{id}-v{version}/companion.apk",
        "min_hub":"0.1.0","min_android_sdk":26,"native_abi_min":1,
        "native_abi_max":1,"gles_major":2,"abis":["armeabi-v7a","arm64-v8a"]
    }

class SignedCatalogContractTests(unittest.TestCase):
    def test_valid_registered_game_tag_matches_version(self):
        self.assertEqual("classic-chess",signer.approved_release(record())["id"])

    def test_wrong_release_tag_refused(self):
        r=record()
        r["url"]=r["url"].replace("module-classic-chess-","module-classic-go-")
        with self.assertRaisesRegex(ValueError,"wrong game or version"):
            signer.approved_release(r)

    def test_wrong_release_version_refused(self):
        r=record()
        r["url"]=r["url"].replace("v0.1.0/","v0.1.1/")
        with self.assertRaisesRegex(ValueError,"wrong game or version"):
            signer.approved_release(r)

    def test_unknown_game_id_refused(self):
        r=record(id="classic-fabricated")
        with self.assertRaisesRegex(ValueError,"unregistered"):
            signer.approved_release(r)

    def test_unbuilt_companion_apk_advertisement_refused(self):
        r=record(kind="companion-apk")
        with self.assertRaisesRegex(ValueError,"no proven build"):
            signer.approved_release(r)

    def test_valid_existing_breakwater_companion_identity_accepted(self):
        r=record(id="breakwater-battery",kind="companion-apk")
        self.assertEqual(r,signer.approved_release(r))

    def sign_test(self,entries,revision=2,prior_revision=None):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            private=root/"private.pem"
            private.write_bytes(b"test-only: call is mocked, not a real key")
            source=root/"source.json"
            source.write_text(json.dumps({
                "schema":1,"catalog_revision":revision,"modules":entries}))
            target=root/"catalog.json"
            if prior_revision is not None:
                target.write_text(json.dumps({
                    "schema":1,"catalog_revision":prior_revision,"modules":[]}))
            with patch.object(signer,"call") as mocked_sign:
                try:
                    signer.sign(source,target,root/"catalog.sig",private)
                finally:
                    self.last_sign_calls=mocked_sign.call_count

    def test_duplicated_game_revision_rejected_before_signing(self):
        r=record()
        with self.assertRaisesRegex(ValueError,"Duplicate game release"):
            self.sign_test([r,r])
        self.assertEqual(0,self.last_sign_calls)

    def test_rollback_catalog_revision_rejected_before_signing(self):
        with self.assertRaisesRegex(ValueError,"exceed published"):
            self.sign_test([record()],revision=3,prior_revision=4)
        self.assertEqual(0,self.last_sign_calls)

    def test_reuse_catalog_revision_rejected_before_signing(self):
        with self.assertRaisesRegex(ValueError,"exceed published"):
            self.sign_test([record()],revision=4,prior_revision=4)
        self.assertEqual(0,self.last_sign_calls)

    def test_mismatched_catalog_url_rejected_before_signing(self):
        r=record()
        r["url"]=r["url"].replace("classic-chess","classic-go")
        with self.assertRaisesRegex(ValueError,"wrong game or version"):
            self.sign_test([r])
        self.assertEqual(0,self.last_sign_calls)

if __name__=="__main__":
    unittest.main()
