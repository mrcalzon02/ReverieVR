#!/usr/bin/env python3
"""Synthetic raster fixtures, never approved geographic assets for Sitka."""
import json
from pathlib import Path
import struct
import tempfile
import unittest

from build_bathymetry import bake, check_provenance, datum_id, HEADER, PIXEL, MAGIC

class BathymetryBakerTest(unittest.TestCase):
    def source(self):
        return {
            "provider": "NOAA NCEI TEST FIXTURE",
            "dataset": "artificial numeric grid ONLY",
            "license": "synthetic test",
            "retrieved": "2026-11-04",
            "url": "https://example.org/fake",
            "vertical_datum": "NAVD88",
            "terrain_vertical_datum": "NAVD88",
            "encoding": "positive_down_depth",
            "quality": "surveyed",
        }

    def test_strict_provenance_before_raster_io(self):
        source=self.source()
        self.assertNotEqual(datum_id("NAVD88"),datum_id("MLLW"))
        self.assertEqual(check_provenance(source),datum_id("NAVD88"))
        changed=dict(source,terrain_vertical_datum="MLLW")
        with self.assertRaisesRegex(ValueError,"datum mismatch"):
            check_provenance(changed)
        changed=dict(source,provider="Google Earth")
        with self.assertRaisesRegex(ValueError,"Google"):
            check_provenance(changed)
        changed=dict(source,encoding="auto")
        with self.assertRaisesRegex(ValueError,"encoding"):
            check_provenance(changed)
        changed=dict(source,quality="imagined")
        with self.assertRaisesRegex(ValueError,"quality"):
            check_provenance(changed)

    def test_geo_tiff_binary_tiling_and_no_data(self):
        import numpy as np
        import rasterio
        from rasterio.transform import from_origin
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp)
            raster=base/"seafloor.tif"
            provenance=base/"provenance.json"
            output=base/"baked"
            source=self.source()
            provenance.write_text(json.dumps(source))
            samples=np.full((6,6),8.,dtype=np.float32)
            samples[1,1]=-9999.
            samples[5,5]=14.
            with rasterio.open(raster,"w",driver="GTiff",height=6,width=6,count=1,
                                dtype="float32",crs="EPSG:32608",
                                transform=from_origin(480000,6300000,10,10),
                                nodata=-9999.) as writer:
                writer.write(samples,1)
            result=bake(raster,provenance,output,3)
            self.assertEqual(result["format"],"sitka-bathymetry-1")
            self.assertEqual(result["tile_count"],4)
            self.assertEqual(result["vertical_datum"],"NAVD88")
            first=(output/"bathymetry-r00-c00.sitkab").read_bytes()
            m,w,h,e,n,dx,dz,datum,reserved=HEADER.unpack_from(first)
            self.assertEqual((m,w,h,datum,reserved),
                             (MAGIC,4,4,datum_id("NAVD88"),0))
            self.assertAlmostEqual(e,480005.)
            self.assertAlmostEqual(n,6299995.)
            self.assertEqual((dx,dz),(10.,10.))
            missing=HEADER.size+(1*w+1)*PIXEL.size
            self.assertEqual(PIXEL.unpack_from(first,missing),(0.,0))
            self.assertEqual(PIXEL.unpack_from(first,HEADER.size),
                             (-8.,1))
            self.assertIn("SITKA-BATHY-INDEX-2\n", (output/"index.txt").read_text())
            self.assertIn("tiles 4\n", (output/"index.txt").read_text())
            self.assertIn("bathymetry-r00-c00.sitkab 480005 6299995 10 10 4 4\n",
                          (output/"index.txt").read_text())
            self.assertEqual(result["index_schema"],2)
            self.assertEqual(result["resident_tile_target"],2)
            self.assertEqual(result["tiles"][0]["valid_samples"],15)
            checksum=result["tiles"][0]["sha256"]
            self.assertEqual(bake(raster,provenance,output,3)["tiles"][0]["sha256"],
                             checksum)
            source["terrain_vertical_datum"]="MLLW"
            provenance.write_text(json.dumps(source))
            with self.assertRaisesRegex(ValueError,"datum mismatch"):
                bake(raster,provenance,output,3)

    def test_many_tiles_are_exportable_but_directory_limit_is_bounded(self):
        import numpy as np
        import rasterio
        from rasterio.transform import from_origin
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp)
            raster=base/"large.tif";meta=base/"p.json"
            meta.write_text(json.dumps(self.source()))
            with rasterio.open(raster,"w",driver="GTiff",height=20,width=20,
                                count=1,dtype="float32",nodata=-9999.,
                                crs="EPSG:32608",transform=from_origin(
                                    480000,6300000,10,10)) as ds:
                ds.write(np.full((20,20),10.,dtype=np.float32),1)
            output=base/"streamed"
            result=bake(raster,meta,output,2)
            self.assertEqual(result["tile_count"],100)
            self.assertIn("tiles 100\n",(output/"index.txt").read_text())
            # The 81 files are packaged, but NOT resident until the boat
            # requests them; no per-tile load occurs in this Python exporter.
            huge=base/"huge.tif"
            with rasterio.open(huge,"w",driver="GTiff",height=100,width=100,
                                count=1,dtype="float32",nodata=-9999.,
                                crs="EPSG:32608",transform=from_origin(
                                    480000,6300000,10,10)) as ds:
                ds.write(np.full((100,100),10.,dtype=np.float32),1)
            with self.assertRaisesRegex(ValueError,"streaming directory"):
                bake(huge,meta,base/"not-created",2)
            self.assertFalse((base/"not-created").exists())

if __name__=="__main__":
    unittest.main()
