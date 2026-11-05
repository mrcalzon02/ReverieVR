#!/usr/bin/env python3
"""Synthetic OSM fixture: identity and geographic fidelity guardrails."""
import json
import tempfile
import unittest
from pathlib import Path
from prepare_osm_waterfront import bounds, prepare


SAMPLE = """<?xml version="1.0"?>
<osm version="0.6" generator="test">
<node id="1" lat="57.048" lon="-135.342"/>
<node id="2" lat="57.048" lon="-135.341"/>
<node id="3" lat="57.049" lon="-135.341"/>
<node id="4" lat="57.049" lon="-135.342"/>
<node id="5" lat="57.050" lon="-135.343"/>
<way id="100" timestamp="2026-10-20T10:00:00Z" version="3">
<nd ref="1"/><nd ref="2"/><nd ref="3"/><nd ref="4"/><nd ref="1"/>
<tag k="building" v="yes"/><tag k="building:levels" v="2"/>
</way>
<way id="200">
<nd ref="2"/><nd ref="3"/><nd ref="5"/><nd ref="2"/>
<tag k="building" v="yes"/><tag k="demolished:building" v="yes"/>
</way>
</osm>"""


class TestPrepareOsm(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.xml = Path(self.tmp.name) / "city.osm"
        self.out = Path(self.tmp.name) / "review.geojson"
        self.xml.write_text(SAMPLE, encoding="utf-8")
        self.extent = "-135.35,57.045,-135.33,57.055"

    def test_source_coordinates_ids_and_review_only_status(self):
        report = prepare(self.xml, self.extent, self.out)
        self.assertEqual(report["feature_count"], 1)
        self.assertEqual(report["license"], "ODbL 1.0")
        f = json.loads(self.out.read_text())["features"][0]
        self.assertEqual(f["properties"]["source_feature_id"], "osm:way:100")
        self.assertEqual(f["properties"]["physical_status"], "needs_verification")
        self.assertEqual(f["properties"]["sitka_kind"], "unclassified")
        self.assertEqual(f["properties"]["building:levels"], 2.)
        self.assertEqual(f["geometry"]["coordinates"][0][0], [-135.342,57.048])
        self.assertEqual(prepare(self.xml,self.extent,self.out),report)

    def test_bounds_and_missing_buildings_rejected(self):
        for s in ("0,0,0,1","nan,0,1,2","0,0,10,1","nonsense","0,0,1"):
            with self.assertRaises(ValueError): bounds(s)
        with self.assertRaisesRegex(ValueError,"no closed"):
            prepare(self.xml,"-135.45,57.045,-135.40,57.055",self.out)

    def test_corruption_and_repeated_ring_nodes_rejected(self):
        self.xml.write_text(SAMPLE.replace('lat="57.049"','lat="nan"',1))
        with self.assertRaisesRegex(ValueError,"coordinates invalid"):
            prepare(self.xml,self.extent,self.out)
        self.xml.write_text(SAMPLE.replace('<nd ref="4"/>','<nd ref="2"/>'))
        with self.assertRaisesRegex(ValueError,"repeated vertices"):
            prepare(self.xml,self.extent,self.out)

    def test_unknown_height_remains_unknown(self):
        self.xml.write_text(SAMPLE.replace(
            '<tag k="building:levels" v="2"/>',''))
        r=prepare(self.xml,self.extent,self.out)
        self.assertEqual(r["missing_height_count"],1)
        f=json.loads(self.out.read_text())["features"][0]["properties"]
        self.assertNotIn("height_m",f)
        self.assertNotIn("building:levels",f)


if __name__ == "__main__":
    unittest.main()
