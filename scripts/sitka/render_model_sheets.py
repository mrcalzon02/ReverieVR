#!/usr/bin/env python3
"""Lightweight orthographic PSX-style review sheets from actual blockout triangles."""
from __future__ import annotations
from pathlib import Path
from math import sqrt
from PIL import Image, ImageDraw, ImageFont
import json
import build_boat_rough_models as boats

ROOT=Path(__file__).resolve().parent
OUT=ROOT
FONTP='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
fb=lambda x:ImageFont.truetype(BOLD,x)
fr=lambda x:ImageFont.truetype(FONTP,x)
WIDTH,HEIGHT=2100,1630
BG=(239,239,233);INK=(28,43,50);BLUE=(25,62,84)

def norm(v):
    d=sqrt(sum(q*q for q in v));return tuple(q/d for q in v)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])

def render(mesh,w,h,scale_extra=1.0,top=False):
    img=Image.new('RGBA',(w,h),(0,0,0,0));draw=ImageDraw.Draw(img)
    look=norm((0.8,0.86,1.15)) if not top else norm((0.,1.,.001))
    right=norm(cross((0.,1.,0.),look))
    up=norm(cross(look,right))
    # Direction is from geometry to camera, so back triangles draw first.
    light=norm((.55,.95,.30))
    projected=[(dot(v,right),dot(v,up),dot(v,look)) for v in mesh.v]
    projx=[v[0] for v in projected];projy=[v[1] for v in projected]
    xmin,xmax=min(projx),max(projx);ymin,ymax=min(projy),max(projy)
    margin=.04
    scale=min(w*(1-margin*2)/(xmax-xmin or 1.),h*(1-margin*2)/(ymax-ymin or 1.))*scale_extra
    mx=(xmin+xmax)*.5;my=(ymin+ymax)*.5
    polygons=[]
    for name,mat,faces in mesh.parts:
        raw=boats.MATERIALS[mat]
        for face in faces:
            indices=[idx-1 for idx in face]
            v=[mesh.v[i] for i in indices]
            a,b,c=v
            n=cross(tuple(b[i]-a[i] for i in range(3)),tuple(c[i]-a[i] for i in range(3)))
            strength=norm(n) if sum(t*t for t in n)>1e-7 else (0,1,0)
            brightness=.72+.22*abs(dot(strength,light))
            rgb=tuple(max(0,min(255,round(q*255*brightness))) for q in raw)
            poly=[(w/2+(projected[i][0]-mx)*scale,h/2-(projected[i][1]-my)*scale) for i in indices]
            depth=sum(projected[i][2] for i in indices)/3
            polygons.append((depth,poly,rgb))
    for _,poly,color in sorted(polygons,key=lambda p:p[0]):
        draw.polygon(poly,fill=color)
        if not top:draw.line(poly+[poly[0]],fill=(46,54,53,125),width=1)
    return img

def sheet(specs,label,sub,filename):
    image=Image.new('RGB',(WIDTH,HEIGHT),BG)
    d=ImageDraw.Draw(image)
    d.rectangle((0,0,WIDTH,175),fill=BLUE)
    d.text((43,23),'SITKA  /  REVERIEVR',font=fb(27),fill='#9fd2d9')
    d.text((43,57),label,font=fb(54),fill='white')
    d.text((44,151),sub,font=fr(19),fill='#dcecf0',anchor='ls')
    d.text((WIDTH-38,41),'SIX PHYSICAL 3D BLOCKOUTS',font=fb(20),fill='#d8e2e8',anchor='ra')
    d.text((WIDTH-38,70),'MAIN VIEW + DECK VIEW',font=fr(17),fill='#a5c3cd',anchor='ra')
    margin=20;gap=15;cellw=(WIDTH-2*margin-2*gap)//3
    cellh=(HEIGHT-195-42)//2-12
    first_y=195
    for idx,spec in enumerate(specs):
        x=margin+(idx%3)*(cellw+gap);y=first_y+(idx//3)*(cellh+gap)
        d.rounded_rectangle((x,y,x+cellw,y+cellh),radius=9,outline='#94a4a9',width=2,fill='#fbfbf8')
        d.rectangle((x+1,y+1,x+cellw-1,y+47),fill='#d5e0e2')
        d.text((x+17,y+9),f'{idx+1:02d}. {spec.title}',font=fb(22),fill=INK)
        m=boats.build(spec)
        view=render(m,cellw-24,430)
        image.paste(view,(x+11,y+64),view)
        d=ImageDraw.Draw(image)
        d.rounded_rectangle((x+cellw-186,y+328,x+cellw-18,y+475),radius=5,fill='#ebf2f1',outline='#a1afb1')
        top=render(m,151,128,top=True)
        image.paste(top,(x+cellw-177,y+339),top)
        d=ImageDraw.Draw(image)
        d.text((x+cellw-22,y+459),'TOP / WALKABLE DECK',font=fb(10),fill='#516269',anchor='ra')
        d.text((x+15,y+500),f'Rough length {spec.length:.1f} m  /  beam {spec.beam:.1f} m',font=fb(18),fill=BLUE)
        info=spec.visual_focus
        # Dynamic split to 2 lines.
        words=info.split();rows=['']
        for word in words:
            if len(rows[-1])+len(word)>49:rows.append('')
            rows[-1]+=(' ' if rows[-1] else '')+word
        for j,row in enumerate(rows[:2]):d.text((x+17,y+535+24*j),row,font=fr(17),fill='#46565b')
        d.text((x+17,y+606),f'PHYSICAL PARTS: {len(m.parts)}  /  TRIANGLES: {m.tris()}',font=fb(14),fill='#5c6e70')
        d.text((x+17,y+632),spec.item_id,font=fr(13),fill='#6c7d80')
    d=ImageDraw.Draw(image)
    d.text((27,HEIGHT-19),'PLANNING BLOCKOUT  •  Y-UP  •  +Z BOW  •  METRIC  •  NOT APPROVED BOAT SPECS OR LIVE GAME CONTENT',font=fb(16),fill='#51646e')
    path=OUT/filename
    image.save(path,optimize=True)
    print(path,path.stat().st_size)
    return path

if __name__=='__main__':
    sheet(boats.TIER2,'TIER 2  /  COASTAL FISHING VESSEL',
          'Six progressive physical hull / bridge / workdeck configurations — from center console to enclosed expedition cabin',
          'sitka_tier2_six_vessel_model_sheet.png')
    sheet(boats.TIER3,'TIER 3  /  COMMERCIAL TRAWLER',
          'Six progressive physical working-vessel configurations — from trawler core to full offshore equipment',
          'sitka_tier3_six_vessel_model_sheet.png')