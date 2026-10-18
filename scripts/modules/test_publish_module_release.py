#!/usr/bin/env python3
"""Offline regression tests for idempotent operator GitHub release writes.

No network, repository mutations or GitHub credentials required.
Run: python3 -m unittest discover -s scripts/modules -p 'test_*.py'
"""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

SCRIPT=Path(__file__).with_name("publish-module-release.py")
spec=importlib.util.spec_from_file_location("reverie_publish_module",SCRIPT)
publisher=importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)

class TestReleaseReconciliation(unittest.TestCase):
    def test_create_timeout_after_remote_acceptance_does_not_duplicate(self):
        remote={"tagName":"module-red-ledger-v0.1.0","isDraft":True,
                "assets":[],"url":"https://github.com/test"}
        state={"published":False,"writes":0}
        def read(tag):
            return remote if state["published"] else None
        def write(argv,retries=1):
            self.assertEqual(argv[0:3],["gh","release","create"])
            state["writes"]+=1
            state["published"]=True
            raise RuntimeError("timeout after GitHub accepted draft")
        with patch.object(publisher,"inspect_release",side_effect=read), \
             patch.object(publisher,"run",side_effect=write):
            returned=publisher.ensure_draft(remote["tagName"],"abcd","name","notes")
        self.assertIs(returned,remote)
        self.assertEqual(state["writes"],1)

    def test_already_existing_draft_makes_no_create_request(self):
        remote={"isDraft":True,"assets":[]}
        with patch.object(publisher,"inspect_release",return_value=remote), \
             patch.object(publisher,"run") as write:
            self.assertIs(publisher.ensure_draft("module-red-ledger-v0.1.0",
                "abcd","name","notes"),remote)
            write.assert_not_called()

    def test_published_release_is_never_mutated(self):
        remote={"isDraft":False,"assets":[]}
        with patch.object(publisher,"inspect_release",return_value=remote):
            with self.assertRaisesRegex(RuntimeError,"already published"):
                publisher.ensure_draft("module-red-ledger-v0.1.0",
                    "abcd","name","notes")

    def test_upload_timeout_after_acceptance_rechecks_before_retry(self):
        with tempfile.TemporaryDirectory() as directory:
            f=Path(directory)/"small.rvpack"
            f.write_bytes(b"data")
            remote={"isDraft":True,"assets":[]}
            state={"calls":0}
            def write(argv,retries=1):
                state["calls"]+=1
                remote["assets"]=[{"name":f.name,"size":f.stat().st_size}]
                raise RuntimeError("connection reset after upload accepted")
            with patch.object(publisher,"inspect_release",return_value=remote), \
                 patch.object(publisher,"run",side_effect=write):
                publisher.ensure_asset("module-red-ledger-v0.1.0",f)
            self.assertEqual(state["calls"],1)

    def test_existing_name_with_wrong_size_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            f=Path(directory)/"small.rvpack"
            f.write_bytes(b"new data")
            remote={"isDraft":True,
                    "assets":[{"name":f.name,"size":3}]}
            with patch.object(publisher,"inspect_release",return_value=remote), \
                 patch.object(publisher,"run") as write:
                with self.assertRaisesRegex(RuntimeError,"size differs"):
                    publisher.ensure_asset("module-red-ledger-v0.1.0",f)
                write.assert_not_called()

    def test_transient_upload_failure_stops_after_three_attempts(self):
        with tempfile.TemporaryDirectory() as directory:
            f=Path(directory)/"small.rvpack"
            f.write_bytes(b"data")
            remote={"isDraft":True,"assets":[]}
            with patch.object(publisher,"inspect_release",return_value=remote), \
                 patch.object(publisher,"run",
                    side_effect=RuntimeError("unavailable")) as write, \
                 patch.object(publisher.time,"sleep"):
                with self.assertRaises(RuntimeError):
                    publisher.ensure_asset("module-red-ledger-v0.1.0",f)
                self.assertEqual(write.call_count,3)

if __name__=="__main__":
    unittest.main()
