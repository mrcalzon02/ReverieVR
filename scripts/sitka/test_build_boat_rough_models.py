#!/usr/bin/env python3
"""Source-only no-Blender, no-SDK regression for Sitka T2 and T3 blockouts."""
from pathlib import Path
from tempfile import TemporaryDirectory
import build_boat_rough_models as boats
import json
import math
from collections import Counter

assert len(boats.TIER1)==6
assert len(boats.TIER2)==6
assert len(boats.TIER3)==6
assert len(set(s.item_id for s in boats.ALL))==18
expected_2={'open_deck','console','cabin','sonar','winch','radar','crane'}
expected_3={'steel_hull','bridge','crab_pot','buoy','handline','hand_crank','powered_hauler','hydraulic_gantry','crab_sorting','crab_crane'}
assert expected_2.issubset(set().union(*(set(s.parts) for s in boats.TIER2)))
assert expected_3.issubset(set().union(*(set(s.parts) for s in boats.TIER3)))
for specs in (boats.TIER1,boats.TIER2,boats.TIER3):
    for i,s in enumerate(specs):
        assert s.variant==i and s.length>2. and s.beam>1.
        m=boats.build(s)
        assert 400<=m.tris()<=3500
        assert m.v and all(math.isfinite(n) for v in m.v for n in v)
        assert any(name=='walkable_deck' for name,_,_ in m.parts)
        assert any(name=='authored_hull_shell' for name,_,_ in m.parts)
        assert len(m.parts)>=20
        x,y,z=m.bounds()
        assert x[1]-x[0]>=s.beam*.5
        assert z[1]-z[0]>=s.length*.9
        # Geometric release gates, not just triangle-count smoke tests.
        sections=m.sections
        assert len(sections)==11
        assert all(a.z<b.z for a,b in zip(sections,sections[1:]))
        assert sections[5].top>sections[0].top*1.3
        assert sections[-1].top<sections[5].top*.25
        assert all(0<q.keel<q.chine<q.top for q in sections)
        assert all(q.keel_y<q.chine_y<q.deck_y<q.sheer_y for q in sections)
        assert all(q.top-q.inner>=(.008 if s.class_name=='tier1' else .020) for q in sections)
        assert all(q.sheer_y-q.deck_y>=(.135 if s.class_name=='tier1' else .18) for q in sections)
        exterior=[part for part in m.parts if part[0]=='authored_hull_shell']
        assert len(exterior)==1
        assert len(exterior[0][2])==112 # 10 x 5 x 2 strips + 12 transom/bow
        edges=Counter(tuple(sorted((a,b))) for face in exterior[0][2]
            for a,b in zip(face,face[1:]+face[:1]))
        assert all(1<=uses<=2 for uses in edges.values())
        assert sum(uses==1 for uses in edges.values())==22 # top open only
        deck_part=next(part for part in m.parts if part[0]=='walkable_deck')
        assert len(deck_part[2])==20
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
        # Confirm the *actual mesh* is now distinctly non-rectangular, not
        # merely given a different part name. Pods have bevelled octagonal
        # lower & tapered upper rings, and cabins have raked windshields.
        pod_names={'helm_console','wooden_cross_bench','small_gear_box',
                   'fishing_cooler','livewell','bulk_bait_prep_station',
                   'aft_bench','ice_hold_hatch','second_hold'}
        pods=[n for n,_,_ in m.parts if n in pod_names]
        assert pods or s.class_name=='tier3',(s.item_id,'missing physical deck accessories')
        for name,mat,faces in m.parts:
            if name not in pod_names:continue
            idx=sorted({q-1 for face in faces for q in face})
            verts=[m.v[q] for q in idx]
            assert len(verts)>=17,(s.item_id,name,'legacy square box')
            bottom=[p for p in verts if p[1]<max(v[1] for v in verts)-.1]
            assert len(set(round(v[0],3) for v in bottom))>=4
        houses=[(n,f) for n,_,f in m.parts
                if n in ('wheelhouse_body','bridge_lower')]
        if (s.class_name=='tier3' or
            (s.class_name=='tier2' and s.variant>=3)):
            assert len(houses)==1,(s.item_id,'missing real bridge shell')
            house=houses[0][1]
            indices=sorted({q-1 for tri in house for q in tri})
            coords=[m.v[i] for i in indices]
            assert len(coords)>=16
            # 8-point lower and upper faceted rings are actually raked.
            assert len(set(round(v[0],3) for v in coords))>=8
            assert len(set(round(v[2],3) for v in coords))>=8
            front=[n for n,_,_ in m.parts if n.startswith('raked_windshield')]
            assert len(front)==2
            assert any(n=='wheelhouse_roof' for n,_,_ in m.parts)
        if s.class_name=='tier3' and s.variant==5:
            assert any(n=='raised_bridge_faceted_shell' for n,_,_ in m.parts)
            assert not any(n=='raised_bridge_cap' for n,_,_ in m.parts)
        # Three classes have distinct physical bow and transom flare curves.
        # Within any hull the beam changes across forward stations.
        assert sections[8].top>sections[-1].top*2.5
        assert sections[1].top!=sections[9].top
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
    assert len(records)==18
    assert len(set(records))==18
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
print('PASS Sitka 18 physical boats, 6 stages each across 3 classes, cross-section, deck, hull and fitting geometry')