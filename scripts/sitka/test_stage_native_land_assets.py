#!/usr/bin/env python3
"""Real-source APK staging regression, not generated Sitka geography."""
import json
import shutil
import tempfile
import unittest
import zlib
from pathlib import Path
from stage_native_land_assets import SOURCE, write_assets, HEADER, MAGIC, digest

class StagedLandTest(unittest.TestCase):
    def test_true_noaa_16_tile_pack_and_unchanged_source(self):
        with tempfile.TemporaryDirectory() as d:
            output=Path(d)/"generated"
            result=write_assets(output)
            self.assertEqual(result["tiles"],16)
            self.assertTrue(result["not_for_navigation"])
            self.assertTrue(result["vertical_datum_unverified"])
            index=(output/"sitka/coast/index.txt").read_text()
            self.assertTrue(index.startswith("SITKA-LAND-INDEX-1\ncrs EPSG32608\n"))
            self.assertIn("datum UNVERIFIED_DYNAMIC_3DEP_MOSAIC\n",index)
            lines=index.splitlines()
            self.assertEqual(len(lines),21)
            seen=set()
            for line in lines[5:]:
                row,col,name,bytes_,crc,verts,tris,e,n=line.split()
                key=(int(row),int(col))
                self.assertNotIn(key,seen)
                seen.add(key)
                self.assertEqual((float(e),float(n)),(478720+key[1]*512,6324480-key[0]*512))
                tile=output/"sitka/coast/tiles"/name
                payload=tile.read_bytes()
                self.assertEqual(len(payload),int(bytes_))
                self.assertEqual(zlib.crc32(payload)&0xffffffff,int(crc))
                self.assertEqual(HEADER.unpack_from(payload)[0],MAGIC)
                self.assertEqual(int(verts),16641)
                self.assertGreater(int(tris),0)
            self.assertEqual(len(seen),16)
            with self.assertRaises(FileExistsError):
                write_assets(output)

    def test_tamper_ledger_blocks_all_output(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            copy=root/"source"
            shutil.copytree(SOURCE,copy)
            index=copy/"land-tiles/index.json"
            data=json.loads(index.read_text())
            data["tiles"][0]["native_mesh_sha256"]="bad"*21+"x"
            index.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError,"SHA-256"):
                write_assets(root/"out",copy)
            self.assertFalse((root/"out").exists())

    def test_missing_or_fake_polygon_identity_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            copy=root/"source"
            shutil.copytree(SOURCE,copy)
            receipt=copy/"noaa-enc-harbour-land.source.json"
            data=json.loads(receipt.read_text())
            data["geometry_source"]="SYNTHETIC_WATER_MASK"
            receipt.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError,"NOAA chart-land"):
                write_assets(root/"out",copy)
            self.assertFalse((root/"out").exists())

    def test_out_of_range_triangle_is_rejected_even_with_updated_hash(self):
        import struct
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            copy = root/"source"
            shutil.copytree(SOURCE, copy)
            manifest_path = copy/"land-tiles/index.json"
            manifest = json.loads(manifest_path.read_text())
            entry = manifest["tiles"][0]
            mesh = copy/"land-tiles"/entry["native_mesh_file"]
            payload = bytearray(mesh.read_bytes())
            _, count, triangles, _, _ = HEADER.unpack_from(payload)
            self.assertGreater(triangles, 0)
            # Change an actual face index, then update the source manifest's
            # SHA-256: a checksum-only gate would incorrectly admit this tile.
            struct.pack_into("<I", payload, HEADER.size+count*15, count)
            mesh.write_bytes(payload)
            entry["native_mesh_sha256"] = digest(payload)
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "triangle index out of vertex range"):
                write_assets(root/"out", copy)
            self.assertFalse((root/"out").exists())

    def test_degenerate_triangle_is_rejected_even_with_updated_hash(self):
        import struct
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            copy = root/"source"
            shutil.copytree(SOURCE, copy)
            manifest_path = copy/"land-tiles/index.json"
            manifest = json.loads(manifest_path.read_text())
            entry = manifest["tiles"][0]
            mesh = copy/"land-tiles"/entry["native_mesh_file"]
            payload = bytearray(mesh.read_bytes())
            _, count, triangles, _, _ = HEADER.unpack_from(payload)
            self.assertGreater(triangles, 0)
            first_index = struct.unpack_from("<I", payload, HEADER.size+count*15)[0]
            struct.pack_into("<I", payload, HEADER.size+count*15+4, first_index)
            mesh.write_bytes(payload)
            entry["native_mesh_sha256"] = digest(payload)
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "degenerate triangle"):
                write_assets(root/"out", copy)
            self.assertFalse((root/"out").exists())

if __name__=="__main__":
    unittest.main()
