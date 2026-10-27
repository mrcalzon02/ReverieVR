#!/usr/bin/env python3
"""Synthetic geometry fixture for exporter correctness; never ship fixture as Sitka geography."""
import json
import struct
import tempfile
import unittest
from pathlib import Path
from build_coast import bake, HEADER, MAGIC

class CoastExportTest(unittest.TestCase):
    def test_export_and_provenance(self):
        import numpy as np
        import rasterio
        from rasterio.transform import from_origin
        from pyproj import Transformer
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            dem = root/'dem.tif'; coast = root/'coast.geojson'; prov = root/'sources.json'
            # Artificial metric 40x40m land patch, deliberately NOT real Sitka.
            with rasterio.open(dem,'w',driver='GTiff',height=5,width=5,count=1,dtype='float32',
                                crs='EPSG:32608',transform=from_origin(480000,6300000,10,10),nodata=-9999) as ds:
                ds.write(np.full((5,5),35,dtype=np.float32),1)
            t=Transformer.from_crs('EPSG:32608','EPSG:4326',always_xy=True)
            xy=[(480000,6299950),(480030,6299950),(480030,6300000),(480000,6300000),(480000,6299950)]
            points=[list(t.transform(x,y)) for x,y in xy]
            coast.write_text(json.dumps({'type':'FeatureCollection','features':[
                {'type':'Feature','properties':{},'geometry':{'type':'Polygon','coordinates':[points]}}]}))
            source={'provider':'test-only','dataset':'synthetic','license':'test','retrieved':'2026-10-26','url':'https://example.org/test'}
            prov.write_text(json.dumps({'elevation':source,'shoreline':source}))
            dest=root/'manifest.json'
            m=bake(dem,coast,prov,dest,1)
            self.assertGreater(m['triangle_count'],0)
            self.assertEqual(m['vertex_count'],25)
            mesh=dest.with_suffix('.sitkam').read_bytes()
            self.assertEqual(HEADER.unpack_from(mesh)[:3],(MAGIC,25,m['triangle_count']))
            self.assertEqual(m['mesh_sha256'],bake(dem,coast,prov,dest,1)['mesh_sha256'])
            prov.write_text(json.dumps({'elevation':dict(source,provider='Google Earth'),'shoreline':source}))
            with self.assertRaisesRegex(ValueError, 'Google'):
                bake(dem,coast,prov,dest,1)
            prov.write_text('{}')
            with self.assertRaisesRegex(ValueError,'provenance'):
                bake(dem,coast,prov,dest,1)

if __name__=='__main__': unittest.main()
