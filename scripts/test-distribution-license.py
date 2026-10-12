#!/usr/bin/env python3
"""Deterministic, SDK-free tests of the real distribution-license auditor."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location(
    "distribution_license", ROOT / "scripts/verify-distribution-license.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class DistributionLicenseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.registry = json.loads(
            (ROOT / "docs/licensing/ASSET_REGISTRY.json").read_text())

    def test_restricted_valve_archive_is_rejected(self):
        kind, value = audit.classify(
            "assets/steam_controller_2015/handoff.zip",
            self.registry, companion=False)
        self.assertEqual(kind, "BLOCKED")
        self.assertIsNone(value)

    def test_registered_core_sources_are_separate(self):
        self.assertEqual(audit.classify(
            "lib/arm64-v8a/libretro.so", self.registry,
            companion=False)[1]["license"], "GPL-2.0-or-later")
        self.assertEqual(audit.classify(
            "lib/armeabi-v7a/libcardboard_sdk_jni.so", self.registry,
            companion=False)[1]["license"], "Apache-2.0")
        self.assertEqual(audit.classify(
            "lib/arm64-v8a/libreverie_native_host.so", self.registry,
            companion=False)[1]["license"], "MIT")

    def test_new_assets_and_libraries_are_not_silently_licensed(self):
        for name in ("assets/unapproved-controller.glb",
                     "res/raw/unreviewed_music.ogg",
                     "lib/arm64-v8a/libunknown_emulator.so"):
            _, record = audit.classify(name, self.registry, companion=False)
            self.assertIsNone(record, name)

    def make_apk(self, entries):
        temp = tempfile.TemporaryDirectory()
        file = Path(temp.name) / "test.apk"
        with zipfile.ZipFile(file, "w") as archive:
            for name, data in entries.items():
                archive.writestr(name, data)
        return temp, file

    def minimal_registry(self):
        registry = dict(self.registry)
        registry["apk_assets"] = {
            key: dict(value, required=False)
            for key, value in self.registry["apk_assets"].items()
        }
        return registry

    def test_actual_apk_sha_and_review_flag(self):
        temp, file = self.make_apk({
            "assets/models/daydream/vr_controller_daydream.obj": b"synthetic",
            "lib/arm64-v8a/libretro.so": b"synthetic native test",
        })
        with temp:
            errors = []
            report, pending = audit.audit_apk(
                file, self.minimal_registry(), companion=False,
                channel="phone-test", errors=errors)
            self.assertEqual(errors, [])
            self.assertEqual(len(report["items"]), 2)
            self.assertTrue(any(x["status"] == "review-required" for x in pending))
            self.assertEqual(report["items"][0]["sha256"], audit.sha256(b"synthetic"))

    def test_commercial_distribution_blocks_unreviewed_asset(self):
        temp, file = self.make_apk({
            "assets/models/daydream/vr_controller_daydream.obj": b"art",
        })
        with temp:
            errors = []
            audit.audit_apk(file, self.minimal_registry(), companion=False,
                            channel="open", errors=errors)
            self.assertTrue(any("blocked" in x for x in errors))

    def test_unregistered_and_restricted_payloads_fail(self):
        temp, file = self.make_apk({
            "assets/unregistered.glb": b"unknown",
            "assets/steam_controller_2015/handoff.zip": b"restricted",
        })
        with temp:
            errors = []
            audit.audit_apk(file, self.minimal_registry(), companion=False,
                            channel="phone-test", errors=errors)
            self.assertTrue(any("UNREGISTERED" in x for x in errors))
            self.assertTrue(any("restricted payload" in x for x in errors))

    def test_sha256_pin_rejects_changed_shareware(self):
        temp, file = self.make_apk({
            "assets/default-content/doom19s.zip": b"forged",
        })
        with temp:
            errors = []
            audit.audit_apk(file, self.minimal_registry(), companion=False,
                            channel="phone-test", errors=errors)
            self.assertTrue(any("SHA-256 mismatch" in x for x in errors))

    def test_main_static_record_has_no_missing_provenance(self):
        errors = []
        audit.static_audit(self.registry, audit.load(audit.BASELINE), errors)
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
