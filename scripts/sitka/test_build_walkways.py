#!/usr/bin/env python3
"""Synthetic tests only; no fake downtown Sitka map ships with the game."""
import hashlib
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from build_waterfront import HEADER,MAGIC
from build_walkways import bake

ROOT=Path(__file__).resolve().parents[2]


class WalkwaysTest(unittest.TestCase):
    def setUp(self):
        import numpy as np
        import rasterio
        from rasterio.transform import from_origin
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.dem=self.root/"dem.tif"
        self.terrain=self.root/"terrain.sitkam"
        self.manifest=self.root/"terrain.json"
        self.walkways=self.root/"pedestrian.geojson"
        self.ledger=self.root/"review.json"
        self.fish=self.root/"fish.txt"
        self.output=self.root/"paths.sitkaw"
        with rasterio.open(self.dem,"w",driver="GTiff",height=12,width=12,
              count=1,dtype="float32",crs="EPSG:32608",nodata=-9999,
              transform=from_origin(480000,6300000,10,10)) as dem:
            dem.write(np.full((12,12),11.5,dtype=np.float32),1)
        self.terrain.write_bytes(HEADER.pack(MAGIC,3,1,480005,6299995))
        sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
        self.manifest.write_text(json.dumps({
            "format":"sitka-static-terrain-1","mesh":"terrain.sitkam",
            "mesh_sha256":sha(self.terrain),
            "elevation_sha256":sha(self.dem),
            "east_origin":480005,"north_origin":6299995,
            "coordinate_system":"EPSG:32608"}),encoding="utf-8")
        evidence={"provider":"Synthetic Test Fixture","dataset":"Synthetic Test",
                  "license":"TEST ONLY","retrieved":"2026-11-04",
                  "url":"https://example.org/not-real-sitka"}
        self.provenance={"walkways":evidence,"fishing_access":evidence,
                         "geometry_reviewed":True}
        self.geo={
            "type":"FeatureCollection",
            "crs":{"type":"name","properties":{"name":"EPSG:32608"}},
            "features":[
                {"type":"Feature","properties":{
                    "feature_id":"test_street","source_feature_id":"fixture:way:1",
                    "physical_status":"existing","public_access":True,
                    "geometry_reviewed":True,"role":"walkway",
                    "sitka_kind":"sidewalk","width_m":2.4},
                 "geometry":{"type":"LineString","coordinates":[
                     [480010,6299990],[480020,6299990],[480030,6299990]]}},
                {"type":"Feature","properties":{
                    "feature_id":"test_shore","source_feature_id":"fixture:way:2",
                    "physical_status":"existing","public_access":True,
                    "geometry_reviewed":True,"role":"walkway",
                    "sitka_kind":"seawalk","width_m":2.0},
                 "geometry":{"type":"LineString","coordinates":[
                     [480030,6299990],[480030,6299980]]}},
                {"type":"Feature","properties":{
                    "feature_id":"public_start","source_feature_id":"fixture:node:1",
                    "physical_status":"existing","public_access":True,
                    "geometry_reviewed":True,"role":"start",
                    "sourced_elevation_m":11.5},
                 "geometry":{"type":"Point","coordinates":[480010,6299990]}},
                {"type":"Feature","properties":{
                    "feature_id":"fish_bank","source_feature_id":"fixture:node:2",
                    "physical_status":"existing","public_access":True,
                    "geometry_reviewed":True,"role":"fishing_stance",
                    "spot_id":"fish_a","radius_m":1.4,
                    "sourced_elevation_m":11.5},
                 "geometry":{"type":"Point","coordinates":[480030,6299980]}}
            ]}
        self.fish.write_text("SITKA-FISH-SITES-1\nfish_a water1 30 -15 6 8\n")
        self.emit()

    def emit(self):
        self.walkways.write_text(json.dumps(self.geo),encoding="utf-8")
        self.ledger.write_text(json.dumps(self.provenance),encoding="utf-8")

    def run_bake(self):
        return bake(self.manifest,self.dem,self.walkways,self.ledger,
                    self.fish,self.output)

    def test_geo_source_deterministic_round_trip_into_native_graph(self):
        report=self.run_bake()
        self.assertEqual(report["node_count"],4)
        self.assertEqual(report["edge_count"],3)
        self.assertEqual(report["stance_count"],1)
        self.assertEqual(report["layout_sha256"],self.run_bake()["layout_sha256"])
        text=self.output.read_text()
        self.assertIn("SITKA-WALK-1\n",text)
        self.assertIn("|seawalk|2.000\n",text)
        self.assertIn("F|fish_bank|",text)
        compiler=shutil.which("g++") or shutil.which("clang++")
        self.assertTrue(compiler,"C++17 compiler required for GIS-to-runtime test")
        source=self.root/"roundtrip.cpp"
        source.write_text("""#include "sitka_walk_graph.h"
#include <fstream>
#include <iterator>
#include <cassert>
int main(int argc,char**argv){
 assert(argc==2);
 std::ifstream stream(argv[1]);
 const std::string text((std::istreambuf_iterator<char>(stream)),
                          std::istreambuf_iterator<char>());
 auto g=reverie::sitka::WalkGraph::Parse(text,480005.,6299995.);
 assert(g.edges.size()==3 && g.stances.size()==1);
 assert(g.Surface(15, -5).valid);
 assert(g.Surface(25, -10).valid);
 assert(g.CanCastFrom(25,-15,30,-15,
      {{"fish_a","water1",30,-15,6,8}}));
 return 0;
}
""",encoding="utf-8")
        exe=self.root/"roundtrip"
        subprocess.run([compiler,"-std=c++17","-O2","-Wall","-Wextra",
            "-Werror","-pedantic","-I",str(ROOT/"app/src/main/jni/native"),
            str(source),"-o",str(exe)],check=True)
        subprocess.run([str(exe),str(self.output)],check=True)

    def test_access_permissions_fail_closed(self):
        self.geo["features"][0]["properties"]["public_access"]=False
        self.emit()
        with self.assertRaisesRegex(ValueError,"public access"):
            self.run_bake()
        self.geo["features"][0]["properties"]["public_access"]=True
        self.provenance["walkways"]["provider"]="Google Street View"
        self.emit()
        with self.assertRaisesRegex(ValueError,"license/source"):
            self.run_bake()

    def test_spawn_and_bank_must_be_actual_junction_vertices(self):
        self.geo["features"][3]["geometry"]["coordinates"]=[480029,6299980]
        self.emit()
        with self.assertRaisesRegex(ValueError,"exact walkway vertex"):
            self.run_bake()
        self.geo["features"][3]["geometry"]["coordinates"]=[480030,6299980]
        self.geo["features"][3]["properties"]["spot_id"]="unknown"
        self.emit()
        with self.assertRaisesRegex(ValueError,"existing surveyed fishing"):
            self.run_bake()

    def test_bridge_walk_requires_authored_heights(self):
        self.geo["features"][0]["properties"]["sitka_kind"]="bridge_walk"
        self.emit()
        with self.assertRaisesRegex(ValueError,"deck elevations"):
            self.run_bake()
        self.geo["features"][0]["properties"]["sourced_elevations_m"]=[11.5,11.5,11.5]
        self.geo["features"][0]["properties"]["elevation_source"]="Synthetic Test"
        self.emit()
        self.assertEqual(self.run_bake()["edge_count"],3)

    def test_terrain_mismatch_and_unreviewed_geometry_rejected(self):
        self.provenance["geometry_reviewed"]=False
        self.emit()
        with self.assertRaisesRegex(ValueError,"review"):
            self.run_bake()
        self.provenance["geometry_reviewed"]=True
        self.emit()
        self.terrain.write_bytes(b"broken")
        with self.assertRaisesRegex(ValueError,"SHA-256"):
            self.run_bake()


if __name__=="__main__":
    unittest.main()
