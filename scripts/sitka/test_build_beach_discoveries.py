#!/usr/bin/env python3
"""No real Sitka coordinates in fixtures; no procedural artifacts."""
from pathlib import Path
import copy
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from build_beach_discoveries import export

CATALOG=(Path(__file__).resolve().parents[2]/
  "app/src/main/jni/native/sitka_intertidal_models.h").read_text()
def data():
    return {
      "format":"SITKA-BEACH-AREAS-1",
      "provenance":{"source_id":"synthetic_test_only","sha256":"a"*64,
        "url":"https://example.org/synthetic","license":"unit test only",
        "reviewed":True,"crs":"EPSG32608"},
      "origin":[480000.,6300000.],
      "areas":[
        {"id":"rock_site","zone":"rocky","first_site_id":0,"count":5,
         "polygon":[[0,0],[10,0],[10,7],[0,7]]},
        {"id":"wrack_site","zone":"wrack","first_site_id":20,"count":4,
         "polygon":[[12,0],[22,0],[22,7],[12,7]]}
      ]}
def fails(change):
    d=data()
    change(d)
    try:export(d,CATALOG)
    except ValueError:return True
    return False
if __name__=="__main__":
    a=data()
    result=export(a,CATALOG)
    assert result==export(a,CATALOG)
    assert result.count("\nD|")==9
    assert not any(k in result for k in
       ("possible_worked_stone","possible_net_weight","possible_shell_midden"))
    a["areas"].reverse()
    assert result==export(a,CATALOG)
    for row in result.splitlines():
        if not row.startswith("D|"):continue
        p=row.split("|")
        assert len(p)==6
        x,z=float(p[3]),float(p[4])
        if p[5]=="rocky":assert 0<x<10 and 0<z<7
        else:assert 12<x<22 and 0<z<7
    assert fails(lambda d:d["provenance"].update(reviewed=False))
    assert fails(lambda d:d["provenance"].update(sha256="bad"))
    assert fails(lambda d:d["provenance"].update(crs="EPSG:4326"))
    assert fails(lambda d:d["provenance"].update(url="https://google.com/maps"))
    assert fails(lambda d:d["areas"][0].update(zone="protected_cultural"))
    assert fails(lambda d:d["areas"][0].update(first_site_id=18))
    assert fails(lambda d:d["areas"][0].update(polygon=[[0,0],[1,1],[2,2]]))
    assert fails(lambda d:d["areas"][0].update(count=0))
    print("PASS Sitka authored substrate, reproducible GAME finds, unique save IDs and cultural protection")
