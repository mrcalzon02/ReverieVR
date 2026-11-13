#!/usr/bin/env python3
"""NOAA ENC source polygon safety tests. Fixtures are NOT actual Sitka coast."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from pyproj import Transformer
from shapely.geometry import box, mapping
from shapely.ops import transform

from acquire_noaa_enc_land import (
    BASE, aoi, get_json, query_features, validate_land, acquire,
)

def grid():
    return {
        "schema":1,"horizontal_crs":"EPSG:32608",
        "source_vertical_datum":"UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
        "grid":{"epsg":32608,"west_grid_node_m":478720,
                "north_grid_node_m":6324480,
                "columns":2,"rows":2,"tile_intervals":128,
                "sample_spacing_m":4}
    }

def example_feature(kind="Polygon", source_id=71):
    utm=box(478850,6324100,479250,6324420)
    convert=Transformer.from_crs(32608,4326,always_xy=True)
    p=transform(convert.transform,utm)
    if kind=="LineString":
        from shapely.geometry import LineString
        p=LineString(list(p.exterior.coords)[:3])
    return {"type":"Feature","id":str(source_id),
            "properties":{"OBJECTID":source_id},"geometry":mapping(p)}

class NoaaEncLandTest(unittest.TestCase):
    def test_official_static_service_and_project_bounds(self):
        self.assertTrue(BASE.startswith("https://encdirect.noaa.gov/"))
        self.assertTrue(BASE.endswith("/MapServer/233"))
        bound,bbox=aoi(grid())
        self.assertAlmostEqual(bound.area,1024.*1024.)
        self.assertTrue(-135.6 < bbox[0] < -135.2)
        self.assertTrue(56.9 < bbox[1] < 57.3)

    def test_real_NOAA_shape_only_and_clip_bounds(self):
        source=example_feature()
        result,area=validate_land([source],[71],grid())
        self.assertEqual(result["type"],"FeatureCollection")
        self.assertEqual(len(result["features"]),1)
        self.assertTrue(0<area<1024**2)
        self.assertEqual(result["features"][0]["properties"]["source_id"],"71")
        self.assertTrue(result["features"][0]["properties"]["aoi_clip_boundary_is_not_shoreline"])
        with self.assertRaisesRegex(ValueError,"not polygonal"):
            validate_land([example_feature("LineString")],[71],grid())
        with self.assertRaisesRegex(ValueError,"no valid"):
            validate_land([{"type":"Feature","id":"99","properties":{},
                            "geometry":mapping(box(0,0,1,1))}],[99],grid())

    def test_response_queries_are_chunked_and_ids_not_duplicated(self):
        seen=[]
        def serve(p):
            seen.append(p)
            if p.get("returnIdsOnly")=="true":
                return {"objectIds":[71]}, "a"*64
            return {"type":"FeatureCollection","features":[example_feature()]},"b"*64
        with patch("acquire_noaa_enc_land.get_json",side_effect=serve):
            result,ids,digests,bbox=query_features(grid())
        self.assertEqual(ids,[71])
        self.assertEqual(len(result),1)
        self.assertEqual(len(digests),2)
        self.assertEqual(seen[0]["geometryType"],"esriGeometryEnvelope")
        self.assertEqual(seen[1]["objectIds"],"71")
        self.assertEqual(seen[1]["f"],"geojson")
        with patch("acquire_noaa_enc_land.get_json",return_value=({"objectIds":[71,71]},"c")):
            with self.assertRaisesRegex(ValueError,"duplicate"):
                query_features(grid())

    def test_acquisition_cannot_fabricate_empty_or_unattributed_land(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            config=root/"grid.json"
            config.write_text(json.dumps(grid()),encoding="utf-8")
            output=root/"noaa.geojson"
            with patch("acquire_noaa_enc_land.query_features",
                       return_value=([example_feature()],[71],["1"*64],[-135.4,57,-135.3,57.1])):
                receipt=acquire(config,output)
            self.assertTrue(output.is_file())
            self.assertIn("source_geojson_sha256",receipt)
            self.assertEqual(receipt["coast_polygons"],1)
            self.assertEqual(receipt["survey_project_AK2309_CS_N"],"NOT_VERIFIED_IN_ENC")
            self.assertTrue(receipt["NOT_FOR_NAVIGATION"])
            with self.assertRaises(FileExistsError):
                acquire(config,output)

if __name__=="__main__":
    unittest.main()
