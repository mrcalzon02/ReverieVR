#!/usr/bin/env python3
"""Sitka VR physical vessel blockouts — pure stdlib, meters, Y-up, +Z bow.

Six separate physical models for each of three Sitka boat tiers: skiff,
coastal fishing boat and commercial crabber.  Planning-only assets, not registered
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

# Six physical Tier 1 rough vessels reconstructed from the USER-APPROVED
# skiff concept board. These are NEW low-poly prototypes, not preexisting
# imported or registered native game hull meshes.
TIER1 = (
    Spec('sitka.boat_tier1.00', 'Shore Dinghy', 2.4, 1.2, 'tier1', 0,
         'Bare open dinghy with planked benches, simple bow', ('open_hull','wood_benches','bow_cleat')),
    Spec('sitka.boat_tier1.01', 'Utility Skiff', 3.6, 1.5, 'tier1', 1,
         'Longer green utility skiff, carrying storage and bucket', ('open_hull','wood_benches','cooler')),
    Spec('sitka.boat_tier1.02', 'Reinforced Harbor Skiff', 4.2, 1.7, 'tier1', 2,
         'Banded aluminum hull, stern outboard and insulated crate', ('open_hull','outboard','crate','cleats')),
    Spec('sitka.boat_tier1.03', 'Coastal Fishing Skiff', 5.2, 2.1, 'tier1', 3,
         'Side rails, simple console, rod holders and motor', ('console','rod_holders','side_rails')),
    Spec('sitka.boat_tier1.04', 'Deepwater Aluminum Skiff', 6.1, 2.3, 'tier1', 4,
         'Reinforced outer hull, sonar, rod rack and outboard', ('console','sonar','rod_holders','outboard')),
    Spec('sitka.boat_tier1.05', 'Master Angler Skiff', 6.8, 2.5, 'tier1', 5,
         'Premium open fishing deck, T-top, sonar and extra rods', ('console','t_top','sonar','rod_holders')),
)
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
ALL = TIER1 + TIER2 + TIER3

@dataclass(frozen=True)
class HullSection:
    z:float
    top:float
    chine:float
    keel:float
    sheer_y:float
    chine_y:float
    keel_y:float
    deck_y:float
    inner:float


def hull_sections(length:float,beam:float,trawler:bool,skiff:bool=False)->tuple[HullSection,...]:
    """Source-independent low-poly hull shape; not naval engineering CAD.

    +Z is bow, -Z transom. Exterior chine steps inward to a shallow V keel.
    Eleven tailored stations approximate curvature using flat PSX-sized polygons.
    """
    if length<=2 or beam<=1:
        raise ValueError('invalid proposed boat blockout dimensions')
    # Fractions of overall length, full half-beam, chine and keel half-beam.
    # The three class profiles differ; a single stretched generic box profile
    # makes a commercial crabber look like an enlarged dinghy. The extra
    # stations round the corners of both ends without subdivision-heavy CAD.
    if skiff:
        profile=(
            (-.49,.60,.48,.26,.025),(-.465,.68,.55,.28,.010),
            (-.42,.82,.68,.31,.000),(-.32,.94,.79,.34,-.020),
            (-.18,.99,.84,.34,-.018),(0.,1.,.85,.33,.000),
            (.18,.95,.79,.29,.025),(.32,.81,.64,.23,.075),
            (.42,.52,.37,.14,.145),(.47,.26,.17,.07,.19),
            (.497,.045,.029,.012,.235))
    elif trawler:
        profile=(
            (-.49,.64,.52,.31,.035),(-.465,.75,.64,.34,.015),
            (-.42,.87,.75,.39,-.010),(-.32,.95,.82,.41,-.020),
            (-.18,.99,.86,.43,-.012),(0.,1.,.87,.43,.005),
            (.18,.97,.85,.40,.030),(.32,.91,.77,.35,.080),
            (.42,.77,.60,.29,.145),(.47,.52,.36,.17,.215),
            (.495,.12,.070,.031,.28))
    else:
        profile=(
            (-.49,.58,.47,.25,.04),(-.465,.70,.59,.30,.02),
            (-.42,.83,.70,.34,.0),(-.32,.93,.79,.37,-.020),
            (-.18,.99,.84,.38,-.020),(0.,1.,.86,.39,.0),
            (.18,.97,.83,.36,.025),(.32,.87,.72,.30,.08),
            (.42,.66,.48,.21,.145),(.47,.37,.24,.095,.21),
            (.495,.065,.039,.018,.25))
    top_base=.23 if trawler else .19
    depth=1.58 if trawler else (min(.79,.52 + length*.045) if skiff else 1.12)
    rim_depth=.26 if trawler else (.14 if skiff else .19)
    sections=[]
    for zf,wf,cf,kf,sheer in profile:
        top=beam*.5*wf
        chine=beam*.5*cf
        keel=beam*.5*kf
        top_y=top_base+sheer
        inner=max(top*.69,top-(.115 if trawler else .085))
        sec=HullSection(
            length*zf,top,chine,keel,top_y,
            top_y-(.62 if trawler else .44),
            top_y-depth,
            top_y-rim_depth,inner)
        if not 0<keel<chine<top or not 0<inner<top:
            raise ValueError('malformed hull cross section')
        sections.append(sec)
    return tuple(sections)


def section_at(stations:tuple[HullSection,...],z:float)->HullSection:
    """Linearly interpolate model surfaces so fixtures never float or cut decks."""
    if not stations[0].z<=z<=stations[-1].z:
        raise ValueError('deck attachment beyond actual vessel stations')
    for left,right in zip(stations,stations[1:]):
        if z<=right.z:
            t=(z-left.z)/(right.z-left.z)
            return HullSection(z,*(getattr(left,k)*(1-t)+getattr(right,k)*t
                       for k in HullSection.__dataclass_fields__ if k!='z'))
    return stations[-1]

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
    def hull(self,length,beam,trawler,skiff=False):
        """Eleven fair tapered stations, framed gunwale, inner wall, and contour-fit deck.

        The exterior, inner rim and deck are generated from the SAME station
        profile. We do not insert a rectangular deck box into a pointed hull.
        Minimal polygons are intentional for PSX-style rendering.
        """
        stations=hull_sections(length,beam,trawler,skiff)
        self.sections=stations
        exterior=[]
        for sec in stations:
            exterior.extend([
                (-sec.top,sec.sheer_y,sec.z),
                (-sec.chine,sec.chine_y,sec.z),
                (-sec.keel,sec.keel_y,sec.z),
                (+sec.keel,sec.keel_y,sec.z),
                (+sec.chine,sec.chine_y,sec.z),
                (+sec.top,sec.sheer_y,sec.z),
            ])
        faces=[]
        for station in range(len(stations)-1):
            for k in range(5):
                a=station*6+k;b=(station+1)*6+k
                c=b+1;d=a+1
                # Outward port/starboard and down-facing keel normals.
                faces.extend([(a,c,b),(a,d,c)])
        # Planar transom + pointed bow cap. The central fan point is safely
        # inside the convex cross section; there is no giant deck cap.
        for index,reverse in ((0,True),(len(stations)-1,False)):
            sec=stations[index]
            center=len(exterior)
            exterior.append((0.,sec.chine_y,sec.z))
            around=[index*6+k for k in range(6)]
            for k,a in enumerate(around):
                b=around[(k+1)%6]
                faces.append((center,b,a) if reverse else (center,a,b))
        self.add('authored_hull_shell','hull_dark' if trawler else 'hull',exterior,faces)

        # Each deck quad follows two adjacent sections, tapering naturally
        # at the bow and stern. No deck vertex can cross the outside plating.
        deck=[]
        for sec in stations:
            deck.extend([(-sec.inner,sec.deck_y,sec.z),
                         ( sec.inner,sec.deck_y,sec.z)])
        faces=[]
        for i in range(len(stations)-1):
            a=2*i;b=a+1;c=2*(i+1)+1;d=2*(i+1)
            faces.extend([(a,c,b),(a,d,c)])
        self.add('walkable_deck','deck',deck,faces)

        # The top rim actually has thickness. Both the gunwale cap and the
        # inner bulwark share the identical height/profile as the outer shell.
        for side in (-1,1):
            verts=[]
            for sec in stations:
                verts.extend([(side*sec.top,sec.sheer_y,sec.z),
                              (side*sec.inner,sec.sheer_y,sec.z),
                              (side*sec.inner,sec.deck_y,sec.z)])
            rim=[];inside=[]
            for i in range(len(stations)-1):
                a=i*3;b=(i+1)*3
                if side==1:
                    rim.extend([(a,b+1,b),(a,a+1,b+1)])
                    inside.extend([(a+1,b+2,b+1),(a+1,a+2,b+2)])
                else:
                    rim.extend([(a,b,b+1),(a,b+1,a+1)])
                    inside.extend([(a+1,b+1,b+2),(a+1,b+2,a+2)])
            self.add('gunwale_cap','steel' if trawler else 'hull',verts,rim)
            self.add('inner_bulwark','hull',verts,inside)
        for side in (-1,1):
            for i in range(len(stations)-1):
                a=stations[i];b=stations[i+1]
                self.tube('hull_rub_rail','rubber',
                          (side*a.top,a.sheer_y,a.z),
                          (side*b.top,b.sheer_y,b.z),
                          .052 if trawler else .034,6)
        return stations
    def deck_box(self,name,mat,x,z,width,height,depth):
        """Four-corner bottom seats on longitudinal deck contour, no floating."""
        if min(width,height,depth)<=0:raise ValueError('bad deck fixture '+name)
        back=z-depth*.5;front=z+depth*.5
        stern=section_at(self.sections,back)
        bow=section_at(self.sections,front)
        # Every actual corner is inside the inward-facing bulwark.
        # The footprint may pass through intermediate cross-sections that
        # are narrower than both end points. Verify ALL crossed stations,
        # not just the four corners, so a cooler cannot puncture a chine.
        sample_z=[back,front]+[sec.z for sec in self.sections
                                 if back<sec.z<front]
        if any(abs(x)+width*.5>section_at(self.sections,zz).inner-.035
               for zz in sample_z):
            raise ValueError('fixture outside shaped deck: '+name)
        y0=stern.deck_y;y1=bow.deck_y
        ytop=max(y0,y1)+height
        w=width*.5
        verts=[(x-w,y0,back),(x+w,y0,back),
               (x+w,y1,front),(x-w,y1,front),
               (x-w,ytop,back),(x+w,ytop,back),
               (x+w,ytop,front),(x-w,ytop,front)]
        faces=[(0,3,2),(0,2,1),(4,5,6),(4,6,7),
               (0,1,5),(0,5,4),(3,7,6),(3,6,2),
               (0,4,7),(0,7,3),(1,2,6),(1,6,5)]
        self.add(name,mat,verts,faces)
        return ytop

    def deck_pod(self,name,mat,x,z,width,height,depth):
        """True eight-sided tapered equipment case with bevelled corners.

        Its lower polygon follows the deck profile rather than plunging a
        rectangular slab through the curved topsides. Use for wheel stations,
        holds, chests, lockers, icebox, bait preparation and machinery housing.
        """
        if min(width,height,depth)<=0:raise ValueError('invalid pod '+name)
        back=z-depth*.5;front=z+depth*.5
        crossed=[back,front]+[sec.z for sec in self.sections if back<sec.z<front]
        if any(abs(x)+width*.5>section_at(self.sections,zz).inner-.035
               for zz in crossed):
            raise ValueError('pod breaches inner bulwark: '+name)
        w=width*.5;corner=min(width,depth)*.17
        # Point ordering is clockwise when seen from above.
        plan=[(-w+corner,back),(w-corner,back),(w,back+corner),
              (w,front-corner),(w-corner,front),(-w+corner,front),
              (-w,front-corner),(-w,back+corner)]
        ytop=max(section_at(self.sections,zz).deck_y for zz in crossed)+height
        lower=[(x+px,section_at(self.sections,pz).deck_y,pz) for px,pz in plan]
        upper=[(x+px*.89,ytop,pz+(z-pz)*.10-.025) for px,pz in plan]
        verts=lower+upper+[(x,ytop+.015,z-.025)]
        faces=[]
        for i in range(8):
            k=(i+1)%8
            faces.extend(((i,k,8+k),(i,8+k,8+i),(16,8+i,8+k)))
        # Open bottom hidden by the genuinely walkable deck.
        self.add(name,mat,verts,faces)
        return ytop

    def canopy(self,name,mat,x,y,z,width,depth,thickness=.07):
        """Octagonal sheer roof with a ridge and real thin edge thickness."""
        if min(width,depth,thickness)<=0:raise ValueError(name)
        w=width*.5;d=depth*.5;corner=min(width,depth)*.14
        points=[(-w+corner,-d),(w-corner,-d),(w,-d+corner),
                (w,d-corner),(w-corner,d),(-w+corner,d),
                (-w,d-corner),(-w,-d+corner)]
        bottom=[(x+px,y-thickness,z+pz) for px,pz in points]
        top=[(x+px,y+.065*(1.-abs(px)/w),z+pz) for px,pz in points]
        verts=bottom+top+[(x,y+.11,z)]
        faces=[]
        for i in range(8):
            j=(i+1)%8
            faces.extend(((i,j,8+j),(i,8+j,8+i),(16,8+i,8+j)))
        self.add(name,mat,verts,faces)

    def outboard_cowl(self,name,x,y,z,width,height,depth):
        """Tapered eight-face cast outboard cover, not a square motor brick.

        Outboards must mount beyond the real transom. Only this intentionally
        external part is exempt from walking-deck footprint restrictions.
        """
        half=width*.5;d=depth*.5
        corner=min(width,depth)*.22
        plan=[(-half+corner,-d),(half-corner,-d),(half,-d+corner),
              (half,d-corner),(half-corner,d),(-half+corner,d),
              (-half,d-corner),(-half,-d+corner)]
        lo=[(x+px*.86,y-height*.5,z+pz*.93) for px,pz in plan]
        hi=[(x+px,y+height*.5,z+pz-.045) for px,pz in plan]
        v=lo+hi+[(x,y+height*.58,z-.045)]
        faces=[]
        for i in range(8):
            j=(i+1)%8
            faces.extend(((i,j,8+j),(i,8+j,8+i),(16,8+i,8+j)))
        self.add(name,'engine',v,faces)
        # narrow real band around crown edge, not flat square panel
        for a,b in zip(hi,hi[1:]+hi[:1]):
            self.tube('outboard_cowl_seam','rubber',a,b,.015,5)

    def wheelhouse(self,name,mat,z,width,length,height):
        """Eight-sided, raked, faceted maritime wheelhouse; no box body.

        Chamfered forward/aft shoulders, inward-sloping sidewalls, window
        panels flush to their actual planar facets, raked glass and a
        crowned low-poly roof. All floor points attach to the deck.
        """
        back=z-length*.5;front=z+length*.5
        crossed=[back,front]+[sec.z for sec in self.sections if back<sec.z<front]
        if any(width*.5>section_at(self.sections,zz).inner-.09 for zz in crossed):
            raise ValueError('wheelhouse wider than deck: '+name)
        half=width*.5;bevel=min(.23*width,.18*length)
        outline=[(-half+bevel,back),(half-bevel,back),
                 (half,back+bevel),(half,front-bevel),
                 (half-bevel,front),(-half+bevel,front),
                 (-half,front-bevel),(-half,back+bevel)]
        roof=max(section_at(self.sections,zz).deck_y for zz in crossed)+height
        floor=[(x,section_at(self.sections,zz).deck_y,zz) for x,zz in outline]
        upper=[(x*.86,roof,zz-.08-.045*(zz>z)) for x,zz in outline]
        faces=[]
        for i in range(8):
            j=(i+1)%8
            faces.extend(((i,j,8+j),(i,8+j,8+i)))
        # Eight wall facet strips + an octagonal top; *not* an 8-vertex box.
        faces.extend((8,8+i,8+i+1) for i in range(1,7))
        self.add(name,mat,floor+upper,faces)

        def onwall(k,t):
            a=floor[k];b=upper[k]
            return tuple(a[j]+(b[j]-a[j])*t for j in range(3))
        def panel(name,k,j,s0,s1,t0=.48,t1=.84):
            def at(s,t):
                a=onwall(k,t);b=onwall(j,t)
                return tuple(a[i]*(1-s)+b[i]*s for i in range(3))
            v=[at(s0,t0),at(s1,t0),at(s1,t1),at(s0,t1)]
            # Offset outwards < 1 cm, enough to avoid z-fighting without
            # looking like a poster flying off the hull.
            cx=sum(q[0] for q in v)*.25
            cz=sum(q[2] for q in v)*.25
            nx=1. if cx>half*.2 else (-1. if cx<-half*.2 else 0.)
            nz=1. if cz>z else (-1. if cz<z else 0.)
            v=[(a+nx*.009,b,c+nz*.009) for a,b,c in v]
            self.add(name,'window',v,[(0,1,2),(0,2,3)])
        # Front window has TWO individually faceted panes, with real
        # central pillar. Starboard/port panes follow raked side walls.
        panel('raked_windshield_port',4,5,.06,.47)
        panel('raked_windshield_starboard',4,5,.53,.94)
        panel('wheelhouse_side_window',2,3,.13,.80)
        panel('wheelhouse_side_window',6,7,.13,.80)
        # Smaller aft windows complete a credible pilothouse silhouette.
        panel('aft_wheelhouse_window',0,1,.19,.44,.52,.82)
        panel('aft_wheelhouse_window',0,1,.56,.81,.52,.82)
        self.canopy('wheelhouse_roof','steel',0.,roof+.085,z-.08,
                    width*1.10,length+.23,.09)
        return roof

    def upper_bridge(self,x,z,roof,width,length):
        """Faceted lookout housing sitting ON the lower bridge roof."""
        half=width*.5
        back=z-length*.5;front=z+length*.5
        bevel=min(width*.16,length*.19)
        plan=[(-half+bevel,back),(half-bevel,back),
              (half,back+bevel),(half,front-bevel),
              (half-bevel,front),(-half+bevel,front),
              (-half,front-bevel),(-half,back+bevel)]
        lower=[(x+px,roof+.13,pz) for px,pz in plan]
        upper=[(x+px*.87,roof+.78,pz-.06) for px,pz in plan]
        faces=[]
        for i in range(8):
            j=(i+1)%8
            faces.extend(((i,j,8+j),(i,8+j,8+i)))
        self.add('raised_bridge_faceted_shell','cabin',lower+upper,faces)
        # Glazing on the actual raked forward face, not an extruded cube.
        p=[lower[k] for k in (4,5)];q=[upper[k] for k in (4,5)]
        v=[]
        for u,t in ((.12,.38),(.88,.38),(.88,.83),(.12,.83)):
            a=tuple(p[0][i]*(1-u)+p[1][i]*u for i in range(3))
            b=tuple(q[0][i]*(1-u)+q[1][i]*u for i in range(3))
            v.append(tuple(a[i]*(1-t)+b[i]*t+(0.009 if i==2 else 0.) for i in range(3)))
        self.add('upper_bridge_raked_glazing','window',v,[(0,1,2),(0,2,3)])
        self.canopy('raised_bridge_crown_roof','steel',x,roof+.84,z-.04,
                    width*1.07,length+.12,.08)

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

def rim_rail(m,stations,trawler=False):
    """Every rail post joins the actual curved gunwale, not an arbitrary box."""
    L=stations[-1].z-stations[0].z
    side_z=(stations[2].z,stations[5].z)
    for side in (-1,1):
        coords=[]
        for z in side_z:
            sec=section_at(stations,z)
            x=side*(sec.inner-.03)
            y=sec.sheer_y
            m.tube('handrail_post','steel',(x,y,z),(x,y+.77,z),.032)
            coords.append((x,y+.77,z))
        m.tube('side_safety_rail','steel',coords[0],coords[1],.030)
        bow=stations[-2]
        m.tube('bow_rail','steel',coords[1],
               (side*bow.inner,bow.sheer_y+.64,bow.z),.028)
    # End stanchions are omitted from the narrow pointed bow, rather than
    # sticking through the outer shell as the old constant-width rail did.

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

def rod_holders(m,zs):
    for n,(side,z) in enumerate((side,z) for side in (-1,1) for z in zs):
        sec=section_at(m.sections,z)
        x=side*(sec.inner-.034)
        y=sec.sheer_y-.11
        m.tube(f'rod_holder_{n}','steel',(x,y,z),(x,y+.38,z),.041)
        if n%2==0:
            m.tube(f'rod_stub_{n}','wood',(x,y+.38,z),
                   (x,y+1.25,z+.17),.024)

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
    m=Mesh()
    L,B=s.length,s.beam
    trawler=s.class_name=='tier3'
    skiff=s.class_name=='tier1'
    stations=m.hull(L,B,trawler,skiff)
    deck=lambda z:section_at(stations,z).deck_y
    if skiff:
        # All fixtures originate INSIDE the actual skiff gunwale contour.
        # Tier 1 keeps a completely open bow; no pasted hull silhouette.
        for zz in (-L*.19,L*.15):
            m.deck_pod('wooden_cross_bench','wood',0.,zz,B*.43,.09,L*.085)
        if s.variant>=1:
            m.deck_pod('small_gear_box','cabin',0.,-L*.07,B*.37,.22,L*.15)
            m.box('fishing_bucket','steel',0.,deck(-L*.31)+.11,-L*.31,
                  min(.28,B*.2),.22,.26)
        if s.variant>=2:
            m.outboard_cowl('stern_outboard_streamlined_cowl',0.,.29,-L*.51,.34,.44,.33)
            m.tube('propeller_shaft','engine',
                (0.,.09,-L*.535),(0.,-.40,-L*.58),.06,6)
            m.deck_pod('insulated_catch_box','cabin',0.,L*.02,B*.35,.24,L*.14)
        if s.variant>=3:
            rim_rail(m,stations,False)
            m.deck_pod('helm_console','cabin',0.,-L*.06,B*.27,.63,.35)
            helm(m,0.,deck(-L*.06),-L*.06)
            rod_holders(m,(-L*.23,))
        if s.variant>=4:
            sonar(m,0.,deck(-L*.06)+1.22,-L*.055)
            m.deck_pod('forward_equipment_hold','steel',0.,L*.26,B*.44,.12,L*.15)
        if s.variant>=5:
            zz=-L*.05
            xx=min(.36,B*.27)
            for side in (-1,1):
                m.tube('t_top_support','steel',
                    (side*xx,deck(zz),zz),
                    (side*xx,deck(zz)+1.71,zz),.035)
            m.canopy('t_top_sun_cover','cabin',0.,deck(zz)+1.75,zz,
                     min(1.32,B*.58),1.03)
            rod_holders(m,(-L*.31,-L*.12))
    else:
        rim_rail(m,stations,trawler)
    if not trawler and not skiff:
        console=-L*.04
        if s.variant<3:
            # Open helm stands ON the deck; later tiers replace it with an
            # enclosed wheelhouse rather than adding a duplicate console.
            m.deck_pod('helm_console','cabin',0.,console,.76,.84,.73)
            helm(m,0.,deck(console)+.02,console,full=s.variant>=2)
        m.deck_pod('aft_bench','wood',0.,-L*.29,B*.56,.20,.46)
        # The propeller deliberately hangs aft of the transom and below
        # the keel: it is not a misplaced deck-mounted cargo box.
        m.outboard_cowl('outboard_streamlined_cowl',0.,.46,-L*.50,.54,.63,.62)
        m.tube('outboard_leg','engine',(0.,.14,-L*.53),(0.,-.64,-L*.58),.13,8)
        if s.variant>=1:
            m.deck_pod('bow_storage','cabin',0.,L*.28,B*.59,.34,.59)
            m.deck_pod('stern_cooler','cabin',-B*.23,-L*.24,.48,.38,.52)
            rod_holders(m,(-L*.26,-L*.10))
        if s.variant==2:
            for side in (-1,1):
                x=side*.46
                m.tube('t_top_leg','steel',(x,deck(console-.26),console-.26),
                       (x,2.29,console-.26),.045)
            m.canopy('light_t_top','cabin',0.,2.35,console,.99,1.22)
            radar(m,0.,2.38,console)
            sonar(m,.32,1.25,console-.12)
        if s.variant>=3:
            cablen=L*(.38 if s.variant<5 else .41)
            z=L*.045
            roof=m.wheelhouse('wheelhouse_body','cabin',z,B*.60,cablen,1.76)
            # Door recess, a dark face ON the stern wall, not a thick box.
            m.box('aft_door','wood',0.,deck(z-cablen*.48)+.75,
                  z-cablen*.51,.56,1.50,.028)
            helm(m,0.,deck(z+.15)+.03,z+.15,full=True)
            sonar(m,.31,deck(z+.12)+1.31,z+.12)
        if s.variant>=4:
            m.deck_pod('fishing_cooler','cabin',B*.18,-L*.24,.58,.45,.65)
            m.deck_pod('livewell','nav',-B*.20,-L*.27,.60,.32,.62)
            workz=-L*.31
            winch(m,-B*.26,deck(workz)+.18,workz,.46)
            rod_holders(m,(-L*.34,-L*.16))
        if s.variant>=5:
            radar(m,0.,roof+.07,z)
            crane_z=-L*.25
            crane(m,B*.30,deck(crane_z)+.01,crane_z,1.90,.92)
            m.deck_pod('second_hold','steel',0.,L*.32,.76,.28,.70)
    elif trawler:
        # Real fore bridge keeps the stern open for handling crab pots.
        zhouse=L*.16
        cablen=L*(.29 if s.variant<5 else .34)
        roof=m.wheelhouse('bridge_lower','cabin',zhouse,B*.64,cablen,2.14)
        m.box('bridge_access_door','wood',0.,deck(zhouse-cablen*.48)+.76,
              zhouse-cablen*.51,.61,1.49,.03)
        helm(m,0.,deck(zhouse+.13)+.04,zhouse+.13,full=True)
        matz=-L*.23
        m.deck_box('workdeck_crab_workmat','deck',0.,matz,B*.50,.06,L*.33)
        gearz=-L*.22
        crabbing_gear(m,s.variant,0.,deck(gearz)+.015,gearz)
        rod_holders(m,(-L*.21,-L*.07))
        radar(m,0.,roof+.06,zhouse)
        if s.variant>=1:
            m.deck_pod('forward_anchor_box','engine',0.,L*.36,B*.34,.32,.55)
            for side in (-1,1):
                z=-L*.30
                x=side*(section_at(stations,z).inner-.14)
                m.deck_pod('reinforced_rail_socket','steel',x,z,.18,.53,.39)
            m.deck_pod('aft_fish_crate','orange',-B*.26,-L*.25,.70,.51,.80)
        if s.variant>=2:
            rackz=-L*.34
            m.deck_pod('empty_pot_rack','steel',B*.15,rackz,.84,.29,.88)
            crab_pot(m,B*.15,deck(rackz)+.29,rackz,.62)
        if s.variant>=3:
            rackz=-L*.33
            m.deck_pod('multiple_pot_storage','steel',-B*.20,rackz,.84,.25,.90)
            crab_pot(m,-B*.20,deck(rackz)+.25,rackz-.02,.63)
            m.deck_pod('powered_buoy_line_storage','orange',-B*.31,-L*.20,
                       .39,.31,.49)
        if s.variant>=4:
            m.deck_pod('ice_hold_hatch','cabin',0.,-L*.05,B*.37,.25,.86)
            m.deck_pod('crab_sorting_table','steel',B*.24,-L*.30,.92,.73,1.20)
            m.deck_pod('hold_cold_box','cabin',-B*.26,-L*.37,.81,.48,.87)
            z=-L*.38
            sct=section_at(stations,z)
            for side in (-1,1):
                x=side*(sct.inner-.17)
                m.tube('gantry_upright','steel',(x,deck(z),z),
                    (x,deck(z)+1.72,z),.07)
            span=sct.inner-.17
            m.tube('aft_gantry_crossbeam','steel',
                   (-span,deck(z)+1.72,z),(span,deck(z)+1.72,z),.075)
        if s.variant>=5:
            # Upper bridge is an intentional step on the existing roof.
            m.upper_bridge(0.,zhouse+.15,roof,B*.59,cablen*.58)
            z=-L*.26
            crane(m,-B*.30,deck(z)+.01,z,3.22,2.24)
            m.deck_pod('extra_cold_hold','cabin',B*.24,-L*.22,.87,.52,.81)
            m.deck_pod('stacked_crab_pot_storage','steel',-B*.19,-L*.32,
                       .81,.41,.81)
            m.deck_pod('bulk_bait_prep_station','cabin',B*.30,-L*.08,
                       .72,.50,.59)
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
                          hull_station_count=len(mesh.sections),
                          deck_inset_min_metres=round(min(sec.top-sec.inner
                              for sec in mesh.sections),4),
                          deck_freeboard_min_metres=round(min(
                              sec.sheer_y-sec.deck_y for sec in mesh.sections),4),
                          hull_shape_contract='continuous class-specific 11-station chine/keel and inset deck',
                          planned_parts=spec.parts,production_status='ROUGH_RESHAPED_ONLY'))
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
    print('Sitka boat models: '+str(len(index))+' vessel OBJ/MTLs (six per three classes) + six crabbing rig OBJ/MTLs; '+str(sum(x['triangles'] for x in index))+' vessel triangles')
    return index

if __name__=='__main__':main()