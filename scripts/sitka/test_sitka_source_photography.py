#!/usr/bin/env python3
"""Real archival source files must never masquerade as current Sitka."""
import json
import math
import unittest
from pathlib import Path


class SitkaPhotographicSourceTest(unittest.TestCase):
    def test_archived_downtown_real_footprints_are_review_only(self):
        root=Path(__file__).resolve().parents[2]
        data=json.loads((root/"data/sitka/research/bridge-downtown-2017.geojson").read_text())
        self.assertEqual(data["type"],"FeatureCollection")
        m=data["metadata"]
        self.assertEqual(m["state"],"ARCHIVE_ONLY_NOT_INSTALLED")
        self.assertEqual(m["exact_upstream_git_blob_sha"],
                         "b4f33e1758fe1596e7eb9964bc3809147af81b85")
        self.assertEqual(len(data["features"]),96)
        ids=set()
        for f in data["features"]:
            self.assertNotIn(f["id"],ids)
            ids.add(f["id"])
            p=f["properties"]
            self.assertEqual(p["physical_status"],"needs_verification")
            self.assertEqual(p["sitka_kind"],"unclassified")
            self.assertEqual(f["geometry"]["type"],"Polygon")
            self.assertEqual(len(f["geometry"]["coordinates"]),1)
            ring=f["geometry"]["coordinates"][0]
            self.assertEqual(ring[0],ring[-1])
            for lon,lat in ring:
                self.assertTrue(math.isfinite(lon) and math.isfinite(lat))
                self.assertTrue(-135.37 < lon < -135.32 and 57.03 < lat < 57.065)

    def test_oconnell_engineering_facts_have_provenance(self):
        root=Path(__file__).resolve().parents[2]
        d=json.loads((root/"data/sitka/research/oconnell-bridge-facts.json").read_text())
        self.assertEqual(d["status"],
                         "SOURCE_MEASUREMENTS_ONLY_NO_APPROVED_GEODETIC_ALIGNMENT")
        for item in ("total_length","main_span","reported_deck_width","tower_height"):
            pair=d["measurements"][item]
            self.assertAlmostEqual(pair["meters"],pair["feet"]*.3048,places=5)
        self.assertEqual(d["measurements"]["tower_configuration"]["sets"],2)
        self.assertEqual(d["measurements"]["stay_cables"]["pattern"],"harp")
        self.assertGreaterEqual(len(d["sources"]),2)
        self.assertGreaterEqual(len(d["unresolved"]),4)

    def test_photo_references_do_not_bundle_copyrighted_imagery(self):
        root=Path(__file__).resolve().parents[2]
        p=json.loads((root/"data/sitka/research/photo-source-audit.json").read_text())
        self.assertGreaterEqual(len(p["sources"]),5)
        self.assertEqual(next(s["usage"] for s in p["sources"]
                              if s["id"]=="google_maps"),
                         "visual_only_no_offline_extraction")
        for source in p["sources"]:
            self.assertTrue(source["url"].startswith("https://"))


if __name__=="__main__":
    unittest.main()
