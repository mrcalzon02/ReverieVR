#!/usr/bin/env python3
"""Synthetic authoring contract tests; never exported as real Sitka structures."""
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

from build_waterfront import HEADER, MAGIC, bake

ROOT = Path(__file__).resolve().parents[2]
CPP_INCLUDE = ROOT / 'app/src/main/jni/native'


class WaterfrontExportTest(unittest.TestCase):
    def setUp(self):
        import numpy as np
        import rasterio
        from rasterio.transform import from_origin
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.dem = self.root / 'dem.tif'
        self.terrain = self.root / 'terrain.sitkam'
        self.manifest = self.root / 'terrain_manifest.json'
        self.buildings = self.root / 'buildings.geojson'
        self.blocks = self.root / 'harbor.geojson'
        self.provenance = self.root / 'sources.json'
        self.output = self.root / 'waterfront.sitkash'
        self.catalog = self.root / 'catalog.json'
        self.catalog.write_text(json.dumps({'format': 'sitka-shore-reality-1',
            'sites': [{'id': 'test_old_building', 'name': 'Historical fixture',
                       'presence': 'demolished', 'geometry_state': 'not_acquired',
                       'references': ['https://example.org/test']}]}), encoding='utf-8')
        with rasterio.open(self.dem, 'w', driver='GTiff', height=12, width=12,
                           count=1, dtype='float32', crs='EPSG:32608',
                           transform=from_origin(480000, 6300000, 10, 10),
                           nodata=-9999) as ds:
            ds.write(np.full((12, 12), 11.5, dtype=np.float32), 1)
        self.terrain.write_bytes(HEADER.pack(MAGIC, 3, 1, 480005, 6299995))
        checksum = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
        self.manifest.write_text(json.dumps({
            'format': 'sitka-static-terrain-1', 'mesh': self.terrain.name,
            'mesh_sha256': checksum(self.terrain),
            'elevation_sha256': checksum(self.dem),
            'coordinate_system': 'EPSG:32608',
            'east_origin': 480005, 'north_origin': 6299995}), encoding='utf-8')
        source = {'provider': 'Synthetic Fixture', 'dataset': 'Synthetic Fixture',
                  'license': 'TEST ONLY', 'retrieved': '2026-11-04',
                  'url': 'https://example.org/fixture'}
        self.provenance.write_text(json.dumps({'buildings': source,
                                                'harbor_structures': source}), encoding='utf-8')
        self.building_feature = {'type': 'Feature', 'properties': {
            'feature_id': 'test_storefront_001', 'sitka_kind': 'storefront',
            'building:levels': 2,
            'source_feature_id': 'fixture:building:1',
            'physical_status': 'existing'}, 'geometry': {'type': 'Polygon', 'coordinates': [[
                [480010, 6299990], [480035, 6299990], [480035, 6299970],
                [480025, 6299970], [480025, 6299980], [480010, 6299980],
                [480010, 6299990]]]}}
        self.block_feature = {'type': 'Feature', 'properties': {
            'feature_id': 'test_pier_001', 'sitka_kind': 'pier',
            'base_m': 11.5, 'width_m': 3, 'depth_m': 14,
            'height_m': .6, 'yaw_rad': 0.0,
            'source_feature_id': 'fixture:pier:1',
            'physical_status': 'existing'},
            'geometry': {'type': 'Point', 'coordinates': [480050, 6299960]}}
        self.emit()

    def emit(self):
        crs = {'type': 'name', 'properties': {'name': 'EPSG:32608'}}
        self.buildings.write_text(json.dumps({'type': 'FeatureCollection', 'crs': crs,
                                               'features': [self.building_feature]}), encoding='utf-8')
        self.blocks.write_text(json.dumps({'type': 'FeatureCollection', 'crs': crs,
                                            'features': [self.block_feature]}), encoding='utf-8')

    def run_bake(self):
        return bake(self.manifest, self.dem, self.buildings,
                    self.blocks, self.provenance, self.output, self.catalog)

    def test_georeferenced_deterministic_source_round_trip(self):
        report = self.run_bake()
        text = self.output.read_text(encoding='utf-8')
        self.assertEqual(report['feature_count'], 2)
        self.assertEqual(report['feature_source_ids']['test_pier_001'], 'fixture:pier:1')
        self.assertEqual(report['building_height_quality']['test_storefront_001'], 'level_estimate')
        self.assertIn('P|test_storefront_001|storefront|11.500|6.000|', text)
        self.assertIn('B|test_pier_001|pier|45.000|-35.000|', text)
        self.assertIn(';20.000,-25.000;', text)
        self.assertEqual(report['layout_sha256'], self.run_bake()['layout_sha256'])
        self.assertEqual(report['layout_sha256'], hashlib.sha256(self.output.read_bytes()).hexdigest())
        # The generated text must load as actual renderer-source Sitka shoreline geometry.
        compiler = shutil.which('g++') or shutil.which('clang++')
        if not compiler:
            self.fail('C++17 compiler required to test native runtime round-trip')
        test_source = self.root/'decode.cpp'
        test_source.write_text('''#include "sitka_shore_geometry.h"\n#include <fstream>\n#include <iterator>\n#include <string>\n#include <cassert>\nint main(int argc,char** argv){\n  assert(argc==2); std::ifstream f(argv[1]);\n  std::string s((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());\n  auto features=reverie::sitka::ParseShoreLayout(s);\n  assert(features.size()==2);\n  reverie::sitka::TerrainMesh mesh;\n  reverie::sitka::AppendShoreGeometry(mesh,features);\n  assert(mesh.indices.size()>0);return 0;\n}\n''', encoding='utf-8')
        exe = self.root/'decode'
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        '-I', str(CPP_INCLUDE), str(test_source), '-o', str(exe)], check=True)
        subprocess.run([str(exe), str(self.output)], check=True)

    def test_real_outline_float_polygon_not_generic_rectangular_block(self):
        self.block_feature['properties']['sitka_kind'] = 'pontoon'
        self.block_feature['geometry'] = {
            'type': 'Polygon', 'coordinates': [[
                [480045,6299960], [480059,6299960],
                [480059,6299965], [480053,6299965],
                [480053,6299968], [480045,6299968],
                [480045,6299960]]]}
        # No inferred width, yaw or depth; exactly the source ring is used.
        for name in ('width_m','depth_m','yaw_rad'):
            self.block_feature['properties'].pop(name)
        self.emit()
        report=self.run_bake()
        self.assertEqual(report['authored_float_polygon_count'],1)
        text=self.output.read_text()
        self.assertIn('P|test_pier_001|pontoon|11.500|0.600|40.000,-35.000;',text)
        self.assertNotIn('B|test_pier_001|',text)
        self.assertIn('48.000,-27.000;',text)
        self.block_feature['geometry']['coordinates'].append([
            [480047,6299961],[480049,6299961],
            [480049,6299962],[480047,6299961]])
        self.emit()
        with self.assertRaisesRegex(ValueError,'without holes'):
            self.run_bake()

    def test_bad_source_and_topology_fail_closed(self):
        self.building_feature['geometry']['coordinates'].append([
            [480015, 6299987], [480018, 6299987], [480018, 6299985],
            [480015, 6299987]])
        self.emit()
        with self.assertRaisesRegex(ValueError, 'without holes'):
            self.run_bake()
        self.building_feature['geometry']['coordinates'].pop()
        self.building_feature['properties']['feature_id'] = 'bad/id'
        self.emit()
        with self.assertRaisesRegex(ValueError, 'feature_id'):
            self.run_bake()
        self.building_feature['properties']['feature_id'] = 'test_pier_001'
        self.emit()
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            self.run_bake()

    def test_invalid_source_metadata_or_terrain_input(self):
        bad = json.loads(self.provenance.read_text())
        bad['buildings']['provider'] = 'Google Earth'
        self.provenance.write_text(json.dumps(bad))
        with self.assertRaisesRegex(ValueError, 'Google'):
            self.run_bake()
        bad['buildings']['provider'] = 'Synthetic Fixture'
        del bad['buildings']['license']
        self.provenance.write_text(json.dumps(bad))
        with self.assertRaisesRegex(ValueError, 'attribution'):
            self.run_bake()
        bad['buildings']['license'] = 'TEST ONLY'
        self.provenance.write_text(json.dumps(bad))
        self.terrain.write_bytes(b'broken!')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            self.run_bake()

    def test_named_historical_site_and_unknown_building_height_rejected(self):
        self.building_feature['properties']['site_ref'] = 'test_old_building'
        self.emit()
        with self.assertRaisesRegex(ValueError, 'Historical'):
            self.run_bake()
        self.building_feature['properties'].pop('site_ref')
        self.building_feature['properties'].pop('building:levels')
        self.emit()
        with self.assertRaisesRegex(ValueError, 'sourced height'):
            self.run_bake()

    def test_bad_height_or_missing_block_geometry(self):
        self.block_feature['properties']['width_m'] = 0
        self.emit()
        with self.assertRaisesRegex(ValueError, 'width_m'):
            self.run_bake()
        self.block_feature['properties']['width_m'] = 3
        self.building_feature['properties']['height_m'] = -2
        self.emit()
        with self.assertRaisesRegex(ValueError, 'height_m'):
            self.run_bake()


if __name__ == '__main__':
    unittest.main()
