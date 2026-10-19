#!/usr/bin/env python3
"""Offline candidate release feed provenance, version and downgrade tests."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

target=Path(__file__).with_name("prepare-catalog-candidate.py")
spec=importlib.util.spec_from_file_location("reverie_catalog_candidate",target)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def receipt(game="breakwater-battery",version="0.1.0",revision=2):
    return {
        "repository":"mrcalzon02/ReverieVR",
        "tag":f"module-{game}-v{version}","name":"BreakwaterBattery.apk",
        "game_id":game,"game_version":version,"game_kind":"companion-apk",
        "game_revision":revision,"size_bytes":44444,"sha256":"a"*64,
        "source_revision":"b"*40,"draft":False,"catalog_ready":True
    }

class CatalogCandidateTests(unittest.TestCase):
    def prepare(self,entry,previous=None,output_exists=False):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            published=root/"catalog.json"
            published.write_text(json.dumps(previous or {
                "schema":1,"catalog_revision":1,"modules":[]}))
            output=root/"candidate.json"
            if output_exists:output.write_text("old candidate")
            data=mod.prepare(entry,published,output,"testing","0.1.0",
                             ["arm64-v8a"])
            self.assertEqual(data,json.loads(output.read_text()))
            return data

    def test_published_receipt_creates_next_unsigned_revision(self):
        data=self.prepare(receipt())
        self.assertEqual(data["catalog_revision"],2)
        self.assertEqual(data["modules"][0]["id"],"breakwater-battery")
        self.assertEqual(data["modules"][0]["revision"],2)

    def test_draft_cannot_be_advertised_as_published(self):
        item=receipt();item["draft"]=True
        with self.assertRaisesRegex(ValueError,"not remotely published"):
            self.prepare(item)

    def test_unready_receipt_cannot_be_advertised(self):
        item=receipt();item["catalog_ready"]=False
        with self.assertRaisesRegex(ValueError,"not remotely published"):
            self.prepare(item)

    def test_cross_game_tag_blocked(self):
        item=receipt();item["tag"]="module-omega-front-v0.1.0"
        with self.assertRaisesRegex(ValueError,"tag mismatch"):
            self.prepare(item)

    def test_new_catalog_never_overwrites_existing_file(self):
        with self.assertRaisesRegex(ValueError,"already exists"):
            self.prepare(receipt(),output_exists=True)

    def test_existing_game_revision_must_be_strictly_older(self):
        old=self.prepare(receipt())["modules"][0]
        previous={"schema":1,"catalog_revision":2,"modules":[old]}
        with self.assertRaisesRegex(ValueError,"not newer"):
            self.prepare(receipt(),previous)

    def test_operator_cannot_publish_unregistered_game(self):
        item=receipt("classic-fabricated")
        with self.assertRaisesRegex(ValueError,"unregistered"):
            self.prepare(item)

    def test_registered_unbuilt_apk_refused(self):
        item=receipt("classic-chess")
        with self.assertRaisesRegex(ValueError,"no proven build"):
            self.prepare(item)

if __name__=="__main__":
    unittest.main()
