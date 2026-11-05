#!/usr/bin/env python3
"""Source-only no-Blender, no-SDK regression for Sitka T2 and T3 blockouts."""
from pathlib import Path
from tempfile import TemporaryDirectory
import build_boat_rough_models as boats
import json
import math

assert len(boats.TIER2)==6
assert len(boats.TIER3)==6
assert len(set(s.item_id for s in boats.ALL))==12
expected_2={'open_deck','console','cabin','sonar','winch','radar','crane'}
expected_3={'steel_hull','bridge','crab_pot','buoy','handline','hand_crank','powered_hauler','hydraulic_gantry','crab_sorting','crab_crane'}
assert expected_2.issubset(set().union(*(set(s.parts) for s in boats.TIER2)))
assert expected_3.issubset(set().union(*(set(s.parts) for s in boats.TIER3)))
for specs in (boats.TIER2,boats.TIER3):
    for i,s in enumerate(specs):
        assert s.variant==i and s.length>5. and s.beam>2.
        m=boats.build(s)
        assert 400<=m.tris()<=2500
        assert m.v and all(math.isfinite(n) for v in m.v for n in v)
        assert any(name=='walkable_deck' for name,_,_ in m.parts)
        assert any(name=='authored_hull_shell' for name,_,_ in m.parts)
        assert len(m.parts)>=30
        x,y,z=m.bounds()
        assert x[1]-x[0]>=s.beam*.5
        assert z[1]-z[0]>=s.length*.9
with TemporaryDirectory() as tmp:
    out=Path(tmp)
    records=[]
    for s in boats.ALL:
        m=boats.build(s)
        name=s.item_id.split('.')[-2]+'_'+s.item_id.split('.')[-1]
        m.write_obj(out/(name+'.obj'))
        assert (out/(name+'.obj')).is_file()
        assert (out/(name+'.mtl')).is_file()
        records.append(name)
    assert len(records)==12
    for stage in range(6):
        gear=boats.build_crab_rig(stage)
        assert 100<gear.tris()<1000
        assert any(n=='crab_pot_bait_canister' for n,_,_ in gear.parts)
        assert any(n=='crab_buoy_float' for n,_,_ in gear.parts)
        gear.write_obj(out/f'crab_gear_{stage:02d}.obj')
        assert (out/f'crab_gear_{stage:02d}.obj').is_file()
    text=(out/(records[0]+'.obj')).read_text()
    assert 'mtllib ' in text and 'usemtl hull' in text
    assert 'o walkable_deck' in text
print('Sitka two higher boat classes: 12 deterministic real OBJ/MTL rough meshes PASS')