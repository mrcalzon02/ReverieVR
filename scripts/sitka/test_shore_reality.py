#!/usr/bin/env python3
"""Synthetic identity checks only: no guessed Sitka features are shipped."""
import json
import tempfile
import unittest
from pathlib import Path
from shore_reality import load_site_catalog, verify_feature_evidence


class ShoreRealityTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "sites.json"
        self.data = {
            "format": "sitka-shore-reality-1",
            "sites": [
                {"id": "real_pier", "name": "Source-checked synthetic pier",
                 "presence": "existing", "geometry_state": "source_checked",
                 "geometry_evidence": {
                     "source_feature_id": "test:way:42",
                     "source_checksum_sha256": "a" * 64,
                     "crs": "EPSG:32608"},
                 "references": ["https://example.org/synthetic"]},
                {"id": "historic_loss", "name": "Destroyed",
                 "presence": "demolished", "geometry_state": "not_acquired",
                 "references": ["https://example.org/historical"]},
                {"id": "not_surveyed", "name": "Confirmed but no outline",
                 "presence": "existing", "geometry_state": "not_acquired",
                 "references": ["https://example.org/identity"]},
            ],
        }
        self.save()

    def save(self):
        self.path.write_text(json.dumps(self.data), encoding="utf-8")

    def props(self, **kwargs):
        return {"source_feature_id": "test:way:42",
                "physical_status": "existing", **kwargs}

    def test_existing_real_pier_needs_matching_geometry(self):
        sites = load_site_catalog(self.path)
        self.assertEqual(verify_feature_evidence(
            self.props(site_ref="real_pier"), sites, "pier"), "real_pier")
        with self.assertRaisesRegex(ValueError, "does not match"):
            verify_feature_evidence(self.props(
                site_ref="real_pier", source_feature_id="test:way:43"),
                sites, "pier")

    def test_historical_or_unacquired_site_cannot_become_geometry(self):
        sites = load_site_catalog(self.path)
        for ref in ("historic_loss", "not_surveyed"):
            with self.assertRaises(ValueError):
                verify_feature_evidence(self.props(site_ref=ref), sites, "pier")

    def test_anonymous_sourced_building_still_requires_actual_id_and_presence(self):
        sites = load_site_catalog(self.path)
        self.assertIsNone(verify_feature_evidence(
            self.props(), sites, "storefront"))
        with self.assertRaisesRegex(ValueError, "source_feature_id"):
            verify_feature_evidence({"physical_status": "existing"}, sites,
                                    "storefront")
        with self.assertRaisesRegex(ValueError, "Distinctive"):
            verify_feature_evidence(self.props(), sites, "cannery")
        with self.assertRaisesRegex(ValueError, "No demolished"):
            verify_feature_evidence(self.props(
                physical_status="demolished"), sites, "storefront")

    def test_catalog_rejects_missing_source_evidence(self):
        self.data["sites"][0]["geometry_evidence"].pop("crs")
        self.save()
        with self.assertRaisesRegex(ValueError, "CRS and hash"):
            load_site_catalog(self.path)
        self.data["sites"][0]["geometry_evidence"]["crs"] = "EPSG:32608"
        self.data["sites"].append(dict(self.data["sites"][0]))
        self.save()
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            load_site_catalog(self.path)


if __name__ == "__main__":
    unittest.main()
