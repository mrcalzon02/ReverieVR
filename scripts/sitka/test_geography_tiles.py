#!/usr/bin/env python3
"""Synthetic test fixture for strict tiling; fixtures are NEVER geographic Sitka."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from acquire_3dep_geography import (
    NODATA, geometry, image_url, load_grid, normalize_geotiff, sha256,
)
from build_geography_tiles import (
    approved_shoreline, compare_seams, tile,
)

class GeographyTilesTest(unittest.TestCase):
    @staticmethod
    def configure(root):
        grid = {
            "schema": 1, "horizontal_crs": "EPSG:32608",
            "source_vertical_datum": "UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
            "grid": {"epsg": 32608, "west_grid_node_m": 478720,
                     "north_grid_node_m": 6324480,
                     "columns": 2, "rows": 2,
                     "tile_intervals": 64, "sample_spacing_m": 4},
        }
        path = root / "grid.json"
        path.write_text(json.dumps(grid), encoding="utf-8")
        return path, grid

    @staticmethod
    def fixture_dem(root, cfg, *, source_nodata=-32768.):
        import numpy as np
        import rasterio
        from rasterio.io import MemoryFile
        from rasterio.transform import from_origin
        (_, _), (width, height) = geometry(cfg)
        # Smooth but nonconstant source heights, known no-data boundary:
        # synthetic only for a repeatable test of real DEM import rejection.
        yy, xx = np.mgrid[:height, :width]
        data = (4. + .05 * xx + .07 * yy).astype("float32")
        data[2, 5] = source_nodata
        g = cfg["grid"]
        step = g["sample_spacing_m"]
        transform = from_origin(g["west_grid_node_m"]-step/2.,
                                g["north_grid_node_m"]+step/2., step, step)
        with MemoryFile() as mem:
            with mem.open(driver="GTiff", width=width, height=height,
                          count=1, dtype="float32", crs="EPSG:32608",
                          transform=transform, nodata=source_nodata) as dst:
                dst.write(data, 1)
            return mem.read()

    @staticmethod
    def receipt(root, grid_path, dem):
        payload = {
            "format": "sitka-usgs-3dep-source-receipt-1",
            "source_kind": "AUTHENTIC_USGS_3DEP_INTERPOLATED_ELEVATION",
            "provider": "USGS National Map",
            "dataset": "3DEP DEM synthetic unit-test fixture",
            "license": "US Government public domain",
            "retrieved": "2026-11-13",
            "url": "https://elevation.nationalmap.gov",
            "grid_config_sha256": sha256(grid_path),
            "normalized_dem_sha256": sha256(dem),
            "horizontal_crs": "EPSG:32608",
            "vertical_datum": "UNVERIFIED_DYNAMIC_3DEP_MOSAIC",
            "ready_for_real_seafloor_merging": False,
        }
        path = root / "dem.source.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return path

    def test_exact_grid_bbox_utm_and_source_url(self):
        with tempfile.TemporaryDirectory() as d:
            path, cfg = self.configure(Path(d))
            loaded = load_grid(path)
            self.assertEqual(cfg, loaded)
            bbox, size = geometry(cfg)
            self.assertEqual(size, (129,129))
            self.assertEqual(bbox,
                             (478718.,6323966.,479234.,6324482.))
            u = image_url(cfg)
            self.assertIn("elevation.nationalmap.gov",u)
            self.assertIn("pixelType=F32",u)
            self.assertIn("imageSR=32608",u)
            self.assertNotIn("google",u.lower())

    def test_real_format_geo_tiff_is_normalized_no_fabrication(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path, cfg = self.configure(root)
            data = self.fixture_dem(root,cfg)
            dest = root / "dem.tif"
            metadata = normalize_geotiff(data,cfg,dest)
            self.assertTrue(dest.is_file())
            self.assertEqual(metadata["valid_samples"],129**2-1)
            self.assertEqual(metadata["unknown_samples"],1)
            import rasterio
            with rasterio.open(dest) as ds:
                self.assertEqual(ds.nodata,NODATA)
                self.assertEqual(ds.read(1)[2,5],NODATA)
                self.assertEqual(ds.crs.to_epsg(),32608)
            with self.assertRaises(Exception):
                normalize_geotiff(b"not-a-GeoTIFF",cfg,root/"bad.tif")

    def test_four_real_format_tiles_and_shared_seams(self):
        import rasterio
        import numpy as np
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            g, cfg=self.configure(root)
            dem=root/"dem.tif"
            normalize_geotiff(self.fixture_dem(root,cfg),cfg,dem)
            receipt=self.receipt(root,g,dem)
            dest=root/"geography"
            proof=tile(dem,receipt,g,dest)
            self.assertFalse(proof["game_runtime_ready"])
            self.assertTrue(proof["sourced_elevation_only"])
            self.assertEqual(proof["tile_count"],4)
            self.assertEqual(proof["terrain_tiles_baked"],0)
            self.assertTrue(all(x["native_mesh_file"] is None for x in proof["tiles"]))
            compare_seams(dest/"elevation",2,2,65)
            a=dest/"elevation/r00-c00.tif"
            b=dest/"elevation/r00-c01.tif"
            with rasterio.open(a) as x, rasterio.open(b) as y:
                self.assertTrue(np.array_equal(x.read(1)[:,-1],y.read(1)[:,0]))
                self.assertEqual(y.transform.c-x.transform.c,256)
            self.assertEqual(proof["tiles"][0]["elevation_sha256"],sha256(a))
            with self.assertRaises(FileExistsError):
                tile(dem,receipt,g,dest)
            altered=json.loads(receipt.read_text())
            altered["normalized_dem_sha256"]="0"*64
            receipt.write_text(json.dumps(altered))
            with self.assertRaisesRegex(ValueError,"unverified"):
                tile(dem,receipt,g,root/"tampered")

    def test_approved_land_polygon_routes_through_existing_coast_baker(self):
        from pyproj import Transformer
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            g,cfg=self.configure(root)
            dem=root/"dem.tif"
            normalize_geotiff(self.fixture_dem(root,cfg),cfg,dem)
            receipt=self.receipt(root,g,dem)
            convert=Transformer.from_crs(32608,4326,always_xy=True)
            xy=[(478700,6324200),(479100,6324200),
                (479100,6324510),(478700,6324510),(478700,6324200)]
            shore=root/"coast.geojson"
            shore.write_text(json.dumps({
                "type":"FeatureCollection",
                "crs":{"type":"name","properties":{"name":"EPSG:4326"}},
                "features":[{"type":"Feature","properties":{},
                             "geometry":{"type":"Polygon",
                                         "coordinates":[[convert.transform(x,y)
                                                         for x,y in xy]]}}]}))
            license=root/"shore-source.json"
            base={"provider":"Fixture Municipal GIS","dataset":"synthetic landmask",
                  "license":"fixture only","retrieved":"2026-11-13",
                  "url":"https://example.org/fixture",
                  "approved_for_redistribution":True,
                  "current_geography_reviewed":True}
            license.write_text(json.dumps(base),encoding="utf-8")
            result=tile(dem,receipt,g,root/"masked",shore,license)
            self.assertGreater(result["terrain_tiles_baked"],0)
            self.assertTrue(any(x["native_mesh_file"] for x in result["tiles"]))
            for e in result["tiles"]:
                if e["native_mesh_file"]:
                    blob=root/"masked"/e["native_mesh_file"]
                    self.assertEqual(blob.read_bytes()[:8],b"SITKA01\x00")
                    self.assertEqual(e["native_mesh_sha256"],sha256(blob))
            self.assertFalse(result["game_runtime_ready"])
            with self.assertRaisesRegex(ValueError,"permissions|rights"):
                approved_shoreline_path = root/"unlicensed.json"
                approved_shoreline_path.write_text(json.dumps(dict(base,approved_for_redistribution=False)))
                approved_shoreline(approved_shoreline_path)

    def test_invalid_or_unreviewed_licenses_fail_without_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            g,cfg=self.configure(root)
            dem=root/"dem.tif"
            normalize_geotiff(self.fixture_dem(root,cfg),cfg,dem)
            receipt=self.receipt(root,g,dem)
            with self.assertRaisesRegex(ValueError,"pair"):
                tile(dem,receipt,g,root/"invalid",root/"coast.geojson")
            self.assertFalse((root/"invalid").exists())
            bad=root/"bad.json"
            bad.write_text(json.dumps({"provider":"Google Earth",
                                       "approved_for_redistribution":True,
                                       "current_geography_reviewed":True}))
            with self.assertRaises(ValueError):
                approved_shoreline(bad)

if __name__=="__main__":
    unittest.main()
