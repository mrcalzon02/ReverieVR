#!/usr/bin/env python3
"""Fail-closed provenance and seam checks for COMMITTED, real Sitka 3DEP tiles.

Corrupted data are temporary test-only copies: never substitute source
elevation or invent a coastline in production.
"""
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from acquire_3dep_geography import sha256
from build_geography_tiles import check_dem, check_receipt, compare_seams

ROOT = Path(__file__).resolve().parents[2]
GRID = ROOT / "data/sitka/geography/sitka-downtown-grid.json"
ARCHIVE = ROOT / "data/sitka/geography/generated/downtown-3dep-v1"
DEM = ARCHIVE / "authoritative-source-dem.tif"
RECEIPT = ARCHIVE / "source-receipt.json"
INDEX = ARCHIVE / "index.json"


class Archived3DEPIntegrityTest(unittest.TestCase):
    def test_real_archive_and_corrupt_receipt_dem_and_shared_edge(self):
        import numpy as np
        import rasterio

        # Positive control: actual archived USGS-derived bytes, not fabricated
        # geographic fixtures or a fresh uncontrolled service download.
        grid, proof = check_receipt(GRID, RECEIPT, DEM)
        with rasterio.open(DEM) as src:
            check_dem(src, grid)
        index = json.loads(INDEX.read_text(encoding="utf-8"))
        self.assertEqual((grid["grid"]["rows"], grid["grid"]["columns"],
                          grid["grid"]["tile_intervals"]), (4, 4, 128))
        self.assertEqual(index["tile_count"], 16)
        self.assertEqual(len(index["tiles"]), 16)
        self.assertEqual(index["source_dem_sha256"], sha256(DEM))
        self.assertEqual(index["source_receipt_sha256"], sha256(RECEIPT))
        self.assertEqual(index["grid_definition_sha256"], sha256(GRID))
        self.assertEqual(proof["normalized_dem_sha256"], sha256(DEM))
        self.assertFalse(index["game_runtime_ready"])
        self.assertTrue(index["sourced_elevation_only"])
        self.assertEqual(index["terrain_tiles_baked"], 0)
        elevation = ARCHIVE / "elevation"
        for entry in index["tiles"]:
            one = ARCHIVE / entry["elevation_file"]
            self.assertTrue(one.is_file(), entry["elevation_file"])
            self.assertEqual(sha256(one), entry["elevation_sha256"])
            self.assertIsNone(entry["native_mesh_file"])
        compare_seams(elevation, 4, 4, 129)

        with tempfile.TemporaryDirectory(prefix="sitka-3dep-negative-") as tmp:
            work = Path(tmp)
            # Source receipt substitution must not be accepted.
            forged = dict(proof, normalized_dem_sha256="0" * 64)
            bad_receipt = work / "altered-receipt.json"
            bad_receipt.write_text(json.dumps(forged), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "unverified / mismatched"):
                check_receipt(GRID, bad_receipt, DEM)

            # Even one changed source DEM byte must fail original receipt SHA.
            bad_dem = work / "altered-dem.tif"
            shutil.copyfile(DEM, bad_dem)
            with bad_dem.open("r+b") as stream:
                stream.seek(-1, 2)
                old = stream.read(1)
                stream.seek(-1, 2)
                stream.write(bytes([old[0] ^ 1]))
            with self.assertRaisesRegex(ValueError, "unverified / mismatched"):
                check_receipt(GRID, RECEIPT, bad_dem)

            # Preserve 15 real tile files by symlink; modify ONLY a private
            # copy of the eastern neighbor's shared column.
            broken = work / "elevation"
            broken.mkdir()
            for tile_file in elevation.glob("*.tif"):
                dest = broken / tile_file.name
                if tile_file.name == "r00-c01.tif":
                    shutil.copyfile(tile_file, dest)
                else:
                    dest.symlink_to(tile_file.resolve())
            self.assertEqual(len(list(broken.glob("*.tif"))), 16)
            with rasterio.open(broken / "r00-c01.tif", "r+") as dst:
                samples = dst.read(1)
                samples[32, 0] += np.float32(0.5)
                dst.write(samples, 1)
            with self.assertRaisesRegex(ValueError,
                                        "east-west tile seam mismatch 0/0"):
                compare_seams(broken, 4, 4, 129)

        compare_seams(elevation, 4, 4, 129)
        self.assertEqual(sha256(DEM), index["source_dem_sha256"])
        print("PASS real archived Sitka 3DEP DEM/receipt/16 checksums/seams; "
              "reject changed receipt, DEM and east-west seam", flush=True)


if __name__ == "__main__":
    unittest.main()
