#!/usr/bin/env python3
"""Sitka VR physical vessel blockouts — pure stdlib, meters, Y-up, +Z bow.

Six separate rough models each for Tier 2 (coastal cabin fishing boat)
and Tier 3 (commercial trawler).  Planning-only assets, not registered
runtime meshes or approved nautical engineering dimensions.

Usage: python3 build_boat_rough_models.py [output-dir]
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import json
import math
import sys

MATERIALS = {
    'hull': (0.13, 0.36, 0.48),
    'hull_dark': (0.10, 0.24, 0.31),
    'steel': (0.65, 0.70, 0.70),
    'deck': (0.58, 0.60, 0.55),
    'cabin': (0.86, 0.86, 0.79),
    'window': (0.26, 0.46, 0.55),
    'rubber': (0.15, 0.17, 0.18),
    'engine': (0.20, 0.22, 0.27),
    'wood': (0.46, 0.32, 0.19),
    'orange': (0.90, 0.39, 0.13),
    'yellow': (0.86, 0.67, 0.13),
    'nav': (0.18, 0.60, 0.69),
    'net': (0.31, 0.43, 0.34),
}

@dataclass(frozen=True)
class Spec:
    item_id: str
    title: str
    length: float
    beam: float
    class_name: str
    variant: int
    visual_focus: str
    parts: tuple[str, ...]

TIER2 = (
    Spec('sitka.boat_tier2.00', 'Center-Console Fisher', 6.2, 2.3, 'tier2', 0,
         'Open walkaround deck, wheel and compass, outboard', ('open_deck', 'console', 'outboard', 'rail')),
    Spec('sitka.boat_tier2.01', 'Walkaround Utility Fisher', 6.5, 2.5, 'tier2', 1,
         'Raised bow locker, flush rod holders, deck coolers', ('open_deck', 'console', 'bow_locker', 'rod_holders', 'cooler', 'outboard')),
    Spec('sitka.boat_tier2.02', 'Weather-Shelter Fisher', 6.7, 2.5, 'tier2', 2,
         'T-top over low helm, antenna, depth screen', ('console', 'shelter_arch', 'sonar', 'antenna', 'outboard')),
    Spec('sitka.boat_tier2.03', 'Enclosed-Cabin Fisher', 7.0, 2.6, 'tier2', 3,
         'Walkable weatherproof pilothouse, forward windows', ('cabin', 'windows', 'helm', 'aft_deck', 'outboard')),
    Spec('sitka.boat_tier2.04', 'Extended Workdeck Fisher', 7.6, 2.8, 'tier2', 4,
         'Reinforced stern, pot hauler, livewell, rod rack', ('cabin', 'winch', 'cooler', 'livewell', 'rod_holders', 'aft_deck')),
    Spec('sitka.boat_tier2.05', 'Regional Expedition Fisher', 8.0, 2.9, 'tier2', 5,
         'Roof radar, compact deck crane, two holds and working stern', ('cabin', 'radar', 'crane', 'two_holds', 'winch', 'aft_deck')),
)
TIER3 = (
    Spec('sitka.boat_tier3.00', 'Compact Commercial Trawler', 10.8, 3.8, 'tier3', 0,
         'Displacement hull, bridge and very slow hand-haul crab pot', ('steel_hull', 'bridge', 'working_stern', 'handline', 'crab_pot', 'buoy')),
    Spec('sitka.boat_tier3.01', 'Reinforced Coastal Workboat', 11.4, 4.0, 'tier3', 1,
         'Manual geared puller, pot bait cabinet and safer side rail', ('steel_hull', 'bridge', 'aft_lockers', 'hand_crank', 'crab_pot', 'buoy')),
    Spec('sitka.boat_tier3.02', 'Hydraulic Hauler Trawler', 12.0, 4.2, 'tier3', 2,
         'Simple line puller, folding davit, safe pot racks', ('steel_hull', 'bridge', 'davit', 'line_puller', 'pot_racks')),
    Spec('sitka.boat_tier3.03', 'Powered Pot Fleet Workboat', 12.6, 4.3, 'tier3', 3,
         'Powered pot hauler and three buoyed pot positions', ('steel_hull', 'bridge', 'powered_hauler', 'pot_racks', 'buoys')),
    Spec('sitka.boat_tier3.04', 'Hydraulic Crab Sorting Trawler', 13.4, 4.6, 'tier3', 4,
         'Hydraulic gantry, crab sorting deck and refrigerated hold', ('steel_hull', 'bridge', 'ice_hold', 'crab_sorting', 'hydraulic_gantry')),
    Spec('sitka.boat_tier3.05', 'Expedition Crab Workboat', 14.4, 4.8, 'tier3', 5,
         'Automated crab crane, bait station, cold hold, five-pot rack', ('steel_hull', 'bridge', 'upper_helm', 'crab_crane', 'bait_station', 'five_pot_rack')),
)
ALL = TIER2 + TIER3

class Mesh:
    def __init__(self):
        self.v = []
        self.parts = []  # (name, material, [triangles of 1-based OBJ indices])
    def add(self, name, mat, verts, faces):
        start = len(self.v)
        self.v += [tuple(map(float, p)) for p in verts]
        self.parts.append((name, mat, [tuple(start + i + 1 for i in tri) for tri in faces]))
    def box(self, name, mat, x, y, z, width, height, depth):
        if min(width, height, depth) <= 0: raise ValueError(name)
        a,b,c=width*.5,height*.5,depth*.5
        v=[(x+sx*a,y+sy*b,z+sz*c) for sx,sy,sz in
           [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        quads=((0,3,2,1),(4,5,6,7),(0,1,5,4),(3,7,6,2),(0,4,7,3),(1,2,6,5))
        self.add(name,mat,v,[tri for a,b,c,d in quads for tri in [(a,b,c),(a,c,d)]])
    def tube(self, name, mat, a, b, r, sides=6):
        ax,ay,az=a; bx,by,bz=b
        dx,dy,dz=bx-ax,by-ay,bz-az
        length=math.sqrt(dx*dx+dy*dy+dz*dz)
        if length<.001:raise ValueError('tube zero length: '+name)
        tx,ty,tz=dx/length,dy/length,dz/length
        if abs(ty)<.9: ux,uy,uz=-tz,0.,tx
        else: ux,uy,uz=1.,0.,0.
        s=math.sqrt(ux*ux+uy*uy+uz*uz); ux/=s;uy/=s;uz/=s
        vx,vy,vz=ty*uz-tz*uy,tz*ux-tx*uz,tx*uy-ty*ux
        verts=[]
        for p in (a,b):
            for i in range(sides):
                angle=2*math.pi*i/sides
                c,d=math.cos(angle)*r,math.sin(angle)*r
                verts.append((p[0]+c*ux+d*vx,p[1]+c*uy+d*vy,p[2]+c*uz+d*vz))
        faces=[]
        for i in range(sides):
            j=(i+1)%sides
            faces.extend([(i,j,sides+j),(i,sides+j,sides+i)])
        for i in range(1,sides-1):
            faces.extend([(0,i+1,i),(sides,sides+i,sides+i+1)])
        self.add(name,mat,verts,faces)
    def hull(self,length,beam,trawler):
        zvals=(-length*.49,-length*.34,0.,length*.32,length*.49)
        width_factors=(.70,.91,1.,.87,.31)
        top_y=.23 if trawler else .19
        depth=1.55 if trawler else 1.08
        verts=[]
        for y in (top_y,top_y-depth):
            for z,wx in zip(zvals,width_factors):
                for sign in (-1,1):verts.append((sign*wx*beam*.5,y,z))
        faces=[]
        for i in range(4):
            for side in (0,1):
                a=2*i+side;b=2*(i+1)+side
                c=b+10;d=a+10
                if side==1:faces += [(a,b,c),(a,c,d)]
                else:faces += [(a,c,b),(a,d,c)]
        # Bottom closing and framed bow/stern; no solid top across usable deck.
        for i in range(1,9):faces += [(10,10+i,10+i+1)]
        faces.extend([(0,1,11),(0,11,10),(8,18,19),(8,19,9)])
        self.add('authored_hull_shell','hull_dark' if trawler else 'hull',verts,faces)
        # Walkable deck, fixed to hull's local transform.
        self.box('walkable_deck','deck',0.,top_y-.15,0.,beam*.77,.11,length*.82)
        # Rub rail and upper side safety edges.
        for side in (-1,1):
            for i in range(4):
                x0=side*beam*.5*width_factors[i];x1=side*beam*.5*width_factors[i+1]
                self.tube('hull_rub_rail','rubber',(x0,top_y,zvals[i]),(x1,top_y,zvals[i+1]),.064 if trawler else .045)
    def write_obj(self,path:Path):
        mtl=path.with_suffix('.mtl')
        with mtl.open('w',encoding='utf8') as out:
            for name,rgb in MATERIALS.items():
                out.write(f'newmtl {name}\nKd {rgb[0]:.4f} {rgb[1]:.4f} {rgb[2]:.4f}\nKa .18 .18 .18\nKs .08 .08 .08\nNs 8\n\n')
        with path.open('w',encoding='utf8') as out:
            out.write('# Sitka rough physical boat model; Y up, +Z bow, meters\n')
            out.write(f'mtllib {mtl.name}\n')
            for x,y,z in self.v:out.write(f'v {x:.5f} {y:.5f} {z:.5f}\n')
            for name,mat,faces in self.parts:
                out.write(f'o {name}\nusemtl {mat}\n')
                for a,b,c in faces:out.write(f'f {a} {b} {c}\n')
    def bounds(self):
        return tuple((min(p[i] for p in self.v),max(p[i] for p in self.v)) for i in range(3))
    def tris(self):return sum(len(f) for _,_,f in self.parts)


def box_pair(m,name,mat,x,y,z,w,h,d):m.box(name,mat,x,y,z,w,h,d)

def rim_rail(m,length,beam,top,trawler=False):
    if trawler: side_z=(-length*.36,length*.31)
    else: side_z=(-length*.26,length*.30)
    for side in (-1,1):
        x=side*(beam*.37)
        for z in side_z:
            m.tube('handrail_post','steel',(x,top,z),(x,top+.80,z),.034)
        m.tube('side_safety_rail','steel',(x,top+.80,side_z[0]),(x,top+.80,side_z[1]),.032)
    for x in (-beam*.37,beam*.37):
        m.tube('bow_rail','steel',(x,top+.80,length*.29),(0.,top+.66,length*.44),.029)

def helm(m,x,y,z,full=False):
    m.box('steering_column','steel',x,y+.44,z,.19,.82,.18)
    m.tube('steering_spindle','steel',(x,y+.88,z),(x,y+.89,z-.18),.05)
    # A six-sided physical wheel, not a screen or painted texture.
    for i in range(8):
        a=i*2*math.pi/8;b=(i+1)*2*math.pi/8
        m.tube('wheel_rim','rubber',
            (x+math.cos(a)*.27,y+1.00+math.sin(a)*.27,z-.19),
            (x+math.cos(b)*.27,y+1.00+math.sin(b)*.27,z-.19),.024)
    m.box('compass','nav',x,y+1.05,z+.13,.20,.13,.19)
    if full:m.box('nav_display','nav',x+.34,y+1.08,z+.20,.36,.25,.08)

def rod_holders(m,beam,zs,y):
    for n,(side,z) in enumerate([(side,z) for side in (-1,1) for z in zs]):
        m.tube(f'rod_holder_{n}','steel',(side*(beam*.34),y,z),(side*(beam*.34),y+.38,z),.047)
        if n%2==0:m.tube(f'rod_stub_{n}','wood',(side*(beam*.34),y+.38,z),(side*(beam*.34),y+1.33,z+.17),.026)

def sonar(m,x,y,z):
    m.box('sonar_screen_body','engine',x,y,z,.40,.30,.12)
    m.box('sonar_screen_surface','nav',x,y,z-.068,.31,.21,.014)

def winch(m,x,y,z,length=.9):
    m.box('winch_mount','steel',x,y-.09,z,length+.2,.18,.36)
    m.tube('winch_drum','yellow',(x-length*.5,y+.18,z),(x+length*.5,y+.18,z),.20,10)
    for sign in (-1,1):m.box('winch_end_bracket','engine',x+sign*(length*.5+.04),y+.15,z,.1,.43,.42)

def crane(m,x,y,z,height,boom):
    m.box('crane_base','steel',x,y+.17,z,.39,.35,.43)
    m.tube('crane_pedestal','yellow',(x,y+.15,z),(x,y+height*.61,z),.11,8)
    m.tube('crane_upper_arm','yellow',(x,y+height*.61,z),(x+.25,y+height,z+boom*.4),.09,8)
    m.tube('crane_boom','yellow',(x+.25,y+height,z+boom*.4),(x+.25,y+height*.79,z+boom),.07,8)
    m.tube('crane_hook_line','rubber',(x+.25,y+height*.79,z+boom),(x+.25,y+height*.40,z+boom),.017)
    m.box('crane_hook','engine',x+.25,y+height*.39,z+boom,.13,.16,.12)

def radar(m,x,y,z):
    m.tube('nav_mast','steel',(x,y,z),(x,y+.64,z),.039)
    m.box('radar_antenna','cabin',x,y+.67,z,.83,.14,.19)


def crab_pot(m,x,y,z,size=.72):
    """True low-poly open pot cage, bait can and escape-ring cues, not a solid cube."""
    half=size*.5
    for sx in (-1,1):
        for sz in (-1,1):
            m.tube('crab_pot_vertical_frame','steel',
                (x+sx*half,y,z+sz*half),
                (x+sx*half,y+size*.56,z+sz*half),.026,6)
    for height in (y,y+size*.56):
        for zz in (-half,half):
            m.tube('crab_pot_cross_frame','steel',
                   (x-half,height,z+zz),(x+half,height,z+zz),.025,6)
        for xx in (-half,half):
            m.tube('crab_pot_long_frame','steel',
                   (x+xx,height,z-half),(x+xx,height,z+half),.025,6)
    m.box('crab_pot_bait_canister','orange',x,y+.20,z,.16,.27,.17)
    m.tube('crab_pot_central_lift_eye','steel',
        (x,y+size*.56,z),(x,y+size*.70,z),.035)

def crab_buoy(m,x,y,z):
    m.tube('crab_buoy_float','orange',(x,y,z),(x,y+.24,z),.13,8)
    m.tube('crab_buoy_flagstaff','steel',(x,y+.18,z),(x,y+.68,z),.018,6)
    m.box('crab_buoy_identification_flag','yellow',x+.11,y+.57,z,.22,.14,.022)

def crabbing_gear(m,stage,x,y,z):
    """Modular, named geometry used both aboard vessels and as a loose shop object."""
    crab_pot(m,x,y,z-.47, .70)
    crab_buoy(m,x+.52,y+.08,z-.48)
    m.box('fish_bait_supply_box','cabin',x-.48,y+.19,z-.43,.35,.37,.45)
    if stage==0:
        m.tube('painfully_slow_handline','rubber',
            (x-.20,y+.08,z+.04),(x+.20,y+.12,z+.06),.050)
    else:
        m.box('crab_puller_mount','steel',x,y+.25,z+.52,.64,.20,.42)
        winch(m,x,y+.33,z+.52,.45 if stage<3 else .78)
        if stage==1:
            m.tube('hand_crank_lever','wood',
                (x+.29,y+.44,z+.54),(x+.42,y+.72,z+.54),.039)
        if stage>=2:
            crane(m,x+.38,y+.30,z+.28,1.20+stage*.26,.58+stage*.18)
        if stage>=3:
            m.box('powered_pot_roller','engine',x-.25,y+.40,z+.30,.22,.41,.54)
        if stage>=4:
            m.box('hydraulic_actuator','yellow',x+.41,y+.60,z+.44,.15,.51,.19)
        if stage>=5:
            m.box('automatic_bait_feeder','steel',x-.49,y+.45,z+.23,.46,.28,.43)
            m.box('crab_sorting_bin','cabin',x-.62,y+.19,z+.75,.55,.37,.64)

def build_crab_rig(stage:int)->Mesh:
    if not 0<=stage<6:raise ValueError('unknown crab gear stage')
    m=Mesh()
    crabbing_gear(m,stage,0.,0.,0.)
    return m

def build(s:Spec)->Mesh:
    m=Mesh();L,B=s.length,s.beam
    is_trawler=s.class_name=='tier3'
    m.hull(L,B,is_trawler)
    deck=.19 if not is_trawler else .23
    rim_rail(m,L,B,deck,is_trawler)
    if not is_trawler:
        # Boat origin deliberately stays at the central helm pivot.
        console=-L*.04
        m.box('helm_console','cabin',0.,.69,console,.76,.84,.73)
        helm(m,0.,.70,console,full=s.variant>=2)
        m.box('aft_bench','wood',0.,.35,-L*.29,B*.56,.18,.46)
        m.box('outboard_head','engine',0.,.46,-L*.50,.54,.63,.62)
        m.tube('outboard_leg','engine',(0.,.14,-L*.53),(0.,-.64,-L*.58),.13,8)
        if s.variant>=1:
            m.box('bow_storage','cabin',0.,.35,L*.28,B*.59,.30,.59)
            m.box('stern_cooler','cabin',-B*.23,.39,-L*.24,.48,.38,.52)
            rod_holders(m,B,(-L*.26,-L*.10),deck)
        if s.variant==2:
            for side in (-1,1):
                x=side*.46
                m.tube('t_top_leg','steel',(x,.26,console-.26),(x,2.29,console-.26),.045)
            m.box('light_t_top','cabin',0.,2.35,console,.99,.10,1.22)
            radar(m,0.,2.38,console)
            sonar(m,.32,1.25,console-.12)
        if s.variant>=3:
            # Real cabin *structure* with windows, roof, door and safe walkaround.
            cablen=L*.38 if s.variant<5 else L*.41
            z=L*.045
            m.box('wheelhouse_body','cabin',0.,1.08,z,B*.62,1.75,cablen)
            m.box('wheelhouse_roof','steel',0.,2.00,z,B*.71,.13,cablen+ .35)
            m.box('forward_windshield','window',0.,1.49,z+cablen*.48,B*.52,.66,.04)
            for side in (-1,1):
                m.box('side_window','window',side*(B*.31+.024),1.48,z,.04,.62,cablen*.65)
            # Aft door/helm entry is represented by inset dark slab in draft.
            m.box('aft_door','wood',0.,1.10,z-cablen*.51,.60,1.45,.035)
            helm(m,0.,.70,z+.15,full=True)
            sonar(m,.32,1.38,z+.12)
        if s.variant>=4:
            m.box('fishing_cooler','cabin',B*.18,.47,-L*.24,.58,.45,.65)
            m.box('livewell','nav',-B*.20,.42,-L*.27,.60,.32,.62)
            winch(m,-B*.26,.60,-L*.31,.46)
            rod_holders(m,B,(-L*.34,-L*.16),deck)
        if s.variant>=5:
            radar(m,0.,2.19,L*.05)
            crane(m,B*.32,.34,-L*.25,1.90,.92)
            m.box('second_hold','steel',0.,.27,L*.32,.76,.28,.70)
    else:
        # Commercial helm sits high forward, preserving full stern work deck.
        zhouse=L*.16
        cablen=L*(.29 if s.variant<5 else .34)
        m.box('bridge_lower','cabin',0.,1.23,zhouse,B*.67,2.14,cablen)
        m.box('bridge_roof','steel',0.,2.35,zhouse,B*.74,.17,cablen+.25)
        m.box('bridge_front_glass','window',0.,1.82,zhouse+cablen*.49,B*.59,.81,.045)
        for side in (-1,1):
            m.box('bridge_side_glass','window',side*B*.34,1.79,zhouse,.04,.77,cablen*.64)
        helm(m,0.,.81,zhouse+.13,full=True)
        m.box('bridge_access_door','wood',0.,1.20,zhouse-cablen*.52,.62,1.52,.045)
        m.box('workdeck_crab_workmat','deck',0.,.26,-L*.23,B*.50,.14,L*.33)
        crabbing_gear(m,s.variant,0.,.35,-L*.22)
        rod_holders(m,B,(-L*.21,-L*.07),deck)
        radar(m,0.,2.51,zhouse)
        if s.variant>=1:
            m.box('forward_anchor_box','engine',0.,.40,L*.36,B*.34,.32,.55)
            for side in (-1,1):
                m.box('reinforced_rail_socket','steel',side*B*.41,.54,-L*.30,.18,.53,.39)
            m.box('aft_fish_crate','orange',-B*.26,.48,-L*.25,.70,.51,.80)
        if s.variant>=2:
            m.box('empty_pot_rack','steel',B*.26,.32,-L*.39,.95,.29,1.14)
            crab_pot(m,B*.26,.54,-L*.39,.62)
        if s.variant>=3:
            m.box('multiple_pot_storage','steel',-B*.24,.35,-L*.40,.93,.25,1.38)
            crab_pot(m,-B*.24,.55,-L*.42,.63)
            m.box('powered_buoy_line_storage','orange',-B*.31,.47,-L*.20,.39,.31,.49)
        if s.variant>=4:
            m.box('ice_hold_hatch','cabin',0.,.37,-L*.05,B*.37,.25,.86)
            m.box('crab_sorting_table','steel',B*.24,.66,-L*.30,.92,.73,1.20)
            m.box('hold_cold_box','cabin',-B*.26,.47,-L*.37,.81,.48,.87)
            m.tube('aft_gantry_crossbeam','steel',(-B*.34,2.05,-L*.38),
                (B*.34,2.05,-L*.38),.08)
            for side in (-1,1):
                m.tube('gantry_upright','steel',(side*B*.34,.37,-L*.38),
                    (side*B*.34,2.05,-L*.38),.07)
        if s.variant>=5:
            m.box('raised_bridge_cap','cabin',0.,2.35,zhouse+.15,B*.66,.60,cablen*.62)
            m.box('upper_bridge_window','window',0.,2.47,zhouse+cablen*.42,B*.54,.36,.04)
            crane(m,-B*.32,.40,-L*.26,3.22,2.24)
            m.box('extra_cold_hold','cabin',B*.24,.47,-L*.22,.87,.52,.81)
            m.box('stacked_crab_pot_storage','steel',-B*.30,.45,-L*.40,.92,.41,.89)
            m.box('bulk_bait_prep_station','cabin',B*.30,.48,-L*.08,.72,.50,.59)
    return m


def main():
    output=Path(sys.argv[1] if len(sys.argv)>1 else 'rough_boats')
    output.mkdir(parents=True,exist_ok=True)
    index=[]
    for spec in ALL:
        mesh=build(spec)
        if mesh.tris()>5500:raise ValueError('triangle budget exceeded')
        if any(not all(math.isfinite(n) for n in v) for v in mesh.v):
            raise ValueError('invalid vertex')
        short=spec.item_id.split('.')[-2]+'_'+spec.item_id.split('.')[-1]
        mesh.write_obj(output/(short+'.obj'))
        index.append(dict(id=spec.item_id,name=spec.title,stage=spec.variant,
                          class_name=spec.class_name,length_metres=spec.length,
                          beam_metres=spec.beam,triangles=mesh.tris(),
                          source_vertices=len(mesh.v),components=[n for n,_,_ in mesh.parts],
                          bounds=mesh.bounds(),visual_focus=spec.visual_focus,
                          planned_parts=spec.parts,production_status='ROUGH_PROTOTYPE_ONLY'))
    rig=[]
    for stage in range(6):
        mesh=build_crab_rig(stage)
        key=f'crab_gear_{stage:02d}'
        mesh.write_obj(output/(key+'.obj'))
        rig.append({'asset_id':f'sitka.crab_rig.{stage:02d}',
                    'stage':stage,'triangles':mesh.tris(),
                    'components':sorted(set(n for n,_,_ in mesh.parts)),
                    'production_status':'ROUGH_PROTOTYPE_ONLY'})
    (output/'crab_gear_manifest.json').write_text(json.dumps(rig,indent=2)+'\n',encoding='utf8')
    (output/'rough_boat_manifest.json').write_text(json.dumps(index,indent=2)+'\n',encoding='utf8')
    print('Sitka boat models: '+str(len(index))+' vessel OBJ/MTLs + six crabbing rig OBJ/MTLs; '+str(sum(x['triangles'] for x in index))+' vessel triangles')
    return index

if __name__=='__main__':main()