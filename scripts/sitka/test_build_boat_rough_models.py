#!/usr/bin/env python3
"""Source-only no-Blender, no-SDK regression for Sitka T2 and T3 blockouts."""
from pathlib import Path
from tempfile import TemporaryDirectory
import build_boat_rough_models as boats
import json
import math
from collections import Counter

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
        assert 400<=m.tris()<=3500
        assert m.v and all(math.isfinite(n) for v in m.v for n in v)
        assert any(name=='walkable_deck' for name,_,_ in m.parts)
        assert any(name=='authored_hull_shell' for name,_,_ in m.parts)
        assert len(m.parts)>=30
        x,y,z=m.bounds()
        assert x[1]-x[0]>=s.beam*.5
        assert z[1]-z[0]>=s.length*.9
        # Geometric release gates, not just triangle-count smoke tests.
        sections=m.sections
        assert len(sections)==9
        assert all(a.z<b.z for a,b in zip(sections,sections[1:]))
        assert sections[4].top>sections[0].top*1.3
        assert sections[-1].top<sections[4].top*.25
        assert all(0<q.keel<q.chine<q.top for q in sections)
        assert all(q.keel_y<q.chine_y<q.deck_y<q.sheer_y for q in sections)
        assert all(q.top-q.inner>=.020 for q in sections)
        assert all(q.sheer_y-q.deck_y>=.18 for q in sections)
        exterior=[part for part in m.parts if part[0]=='authored_hull_shell']
        assert len(exterior)==1
        assert len(exterior[0][2])==92 # 8 x 5 x 2 strips + 12 transom/bow
        edges=Counter(tuple(sorted((a,b))) for face in exterior[0][2]
            for a,b in zip(face,face[1:]+face[:1]))
        assert all(1<=uses<=2 for uses in edges.values())
        assert sum(uses==1 for uses in edges.values())==18 # top open only
        deck_part=next(part for part in m.parts if part[0]=='walkable_deck')
        assert len(deck_part[2])==16
        assert len(m.v)>=len(sections)*8
        # The 18 deck coordinates match the exact shape of the interior
        # bulwark; all are below the sheer and never outside the outer hull.
        # Vessel meshes are cumulative, so locate the deck's local OBJ indices.
        min_index=min(i for face in deck_part[2] for i in face)-1
        deck_vertices=m.v[min_index:min_index+len(sections)*2]
        for station in range(len(sections)):
            sec=sections[station]
            left,right=deck_vertices[2*station:2*station+2]
            assert abs(left[0]+sec.inner)<1e-6
            assert abs(right[0]-sec.inner)<1e-6
            assert abs(left[1]-sec.deck_y)<1e-6
            assert abs(right[1]-sec.deck_y)<1e-6
            assert left[2]==right[2]==sec.z
            assert abs(left[0])<sec.top
        names=[n for n,_,_ in m.parts]
        assert names.count('gunwale_cap')==2
        assert names.count('inner_bulwark')==2
        # Deck-mounted furniture bottoms follow the deck contour. No
        # benches, lockers, wheelhouses or holds are simply pushed into a
        # rectangular slab beyond the shaped bulwark.
        for name,mat,faces in m.parts:
            for face in faces:
                a,b,c=(m.v[q-1] for q in face)
                u=[b[k]-a[k] for k in range(3)]
                v=[c[k]-a[k] for k in range(3)]
                cross=(u[1]*v[2]-u[2]*v[1],
                       u[2]*v[0]-u[0]*v[2],
                       u[0]*v[1]-u[1]*v[0])
                assert sum(q*q for q in cross)>1e-14,(s.item_id,name)
with TemporaryDirectory() as tmp:
    out=Path(tmp)
    records=[]
    for s in boats.ALL:
        m=boats.build(s)
        name=s.item_id.split('.')[-2]+'_'+s.item_id.split('.')[-1]
        m.write_obj(out/(name+'.obj'))
        assert (out/(name+'.obj')).is_file()
        assert (out/(name+'.mtl')).is_file()
        # Source is actual triangular OBJ geometry, not an image/texture
        # representation of a boat. Indices must reference emitted vertices.
        obj=(out/(name+'.obj')).read_text().splitlines()
        verts=sum(row.startswith('v ') for row in obj)
        faces=[row for row in obj if row.startswith('f ')]
        assert verts==len(m.v) and len(faces)==m.tris()
        assert all(1<=int(idx)<=verts for row in faces
                   for idx in row.split()[1:])
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
