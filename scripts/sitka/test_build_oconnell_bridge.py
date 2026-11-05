#!/usr/bin/env python3
"""Offline synthetic fixture: never an actual 2026 Sitka bridge alignment."""
import hashlib
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from build_waterfront import HEADER, MAGIC
from build_oconnell_bridge import bake

ROOT = Path(__file__).resolve().parents[2]
INCLUDE = ROOT / "app/src/main/jni/native"


class BridgeBakeTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.terrain=self.root/"terrain.sitkam"
        self.manifest=self.root/"terrain_manifest.json"
        self.geojson=self.root/"reviewed_axis.geojson"
        self.grade=self.root/"surveyed_grade.json"
        self.output=self.root/"oconnell.bridge"
        self.terrain.write_bytes(HEADER.pack(MAGIC,3,1,480005,6299995))
        self.manifest.write_text(json.dumps({
            "format":"sitka-static-terrain-1",
            "mesh":"terrain.sitkam",
            "mesh_sha256":hashlib.sha256(self.terrain.read_bytes()).hexdigest(),
            "east_origin":480005,"north_origin":6299995,
            "coordinate_system":"EPSG:32608"}), encoding="utf-8")
        self.pts=[[480010,6299990],[480010,6299930],[480010,6299870]]
        self.feature={"type":"Feature","properties":{
            "site_id":"oconnell_bridge", "source_feature_id":"osm:way:12345",
            "physical_status":"existing"},
            "geometry":{"type":"LineString","coordinates":self.pts}}
        self.grade_data={
            "format":"sitka-bridge-grade-1","site_id":"oconnell_bridge",
            "geometry_reviewed":True,
            "source_feature_id":"osm:way:12345",
            "width_m":11.5824,"deck_thickness_m":.6,
            "side_height_m":30.48,
            "deck_elevations_m":[13.0,13.5,14.0],
            "evidence":{"provider":"TEST ONLY","dataset":"TEST ONLY",
                "license":"TEST ONLY","retrieved":"2026-11-04",
                "url":"https://example.org/synthetic"}}
        self.emit()

    def emit(self):
        self.geojson.write_text(json.dumps({
            "type":"FeatureCollection",
            "crs":{"type":"name","properties":{"name":"EPSG:32608"}},
            "features":[self.feature]}),encoding="utf-8")
        self.grade_data["source_sha256"]=hashlib.sha256(
            self.geojson.read_bytes()).hexdigest()
        self.grade.write_text(json.dumps(self.grade_data),encoding="utf-8")

    def run_bake(self):
        return bake(self.manifest,self.geojson,self.grade,self.output)

    def test_exact_source_line_and_sourced_grade_compile_into_native_mesh(self):
        report=self.run_bake()
        self.assertEqual(report["axis_count"],3)
        self.assertAlmostEqual(report["length_m"],120.,places=4)
        self.assertEqual(report["source_feature_id"],"osm:way:12345")
        contents=self.output.read_text(encoding="utf-8")
        self.assertIn("axis.0=5.000,13.000,-5.000",contents)
        self.assertIn("axis.2=5.000,14.000,-125.000",contents)
        self.assertEqual(
            hashlib.sha256(self.output.read_bytes()).hexdigest(),
            report["layout_sha256"])
        self.assertEqual(report["layout_sha256"],self.run_bake()["layout_sha256"])
        compiler=shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.fail("Native C++17 compiler required for authored fixture contract")
        source=self.root/"decode.cpp"
        source.write_text("""#include "sitka_bridge_layout.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
int main(int argc,char**argv){
    assert(argc==2);
    std::ifstream f(argv[1]);
    const std::string text((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    auto mesh=reverie::sitka::ParseBridgeLayout(text,480005.,6299995.);
    assert(mesh.planes.size()==6u);
    assert(mesh.triangle_count()==12u);
    return 0;
}
""",encoding="utf-8")
        exe=self.root/"bridge_test"
        subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror",
            "-pedantic","-I",str(INCLUDE),str(source),"-o",str(exe)],
            check=True)
        subprocess.run([str(exe),str(self.output)],check=True)

    def test_missing_measured_grade_never_guesses_height(self):
        del self.grade_data["deck_elevations_m"]
        self.emit()
        with self.assertRaisesRegex(ValueError,"elevation required"):
            self.run_bake()

    def test_unverified_and_stale_source_rejected(self):
        self.grade_data["geometry_reviewed"]=False
        self.emit()
        with self.assertRaisesRegex(ValueError,"review"):
            self.run_bake()
        self.grade_data["geometry_reviewed"]=True
        self.feature["properties"]["physical_status"]="needs_verification"
        self.emit()
        with self.assertRaisesRegex(ValueError,"review"):
            self.run_bake()
        self.feature["properties"]["physical_status"]="existing"
        self.emit()
        self.grade_data["source_sha256"]="0"*64
        self.grade.write_text(json.dumps(self.grade_data))
        with self.assertRaisesRegex(ValueError,"checksum"):
            self.run_bake()

    def test_curved_or_skewed_bridge_requires_separate_verified_span(self):
        self.pts[1][0]+=2
        self.emit()
        with self.assertRaisesRegex(ValueError,"curved/skewed"):
            self.run_bake()

    def test_origin_checksum_and_source_id_fail_closed(self):
        self.grade_data["source_feature_id"]="osm:way:999"
        self.emit()
        with self.assertRaisesRegex(ValueError,"way ID"):
            self.run_bake()
        self.grade_data["source_feature_id"]="osm:way:12345"
        self.emit()
        self.manifest.write_text(self.manifest.read_text().replace(
            '"east_origin": 480005','"east_origin": 480006'))
        with self.assertRaisesRegex(ValueError,"origins differ"):
            self.run_bake()


if __name__=="__main__":
    unittest.main()
