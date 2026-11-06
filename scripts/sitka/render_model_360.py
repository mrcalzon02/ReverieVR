#!/usr/bin/env python3
"""Render real generated OBJ-equivalent geometry of 18 Sitka vessels.

Real geometry from the generator, static stylized illustrative water, no AI
repainting. Angles advance 15 degrees at each frame for a seamless 360 loop.
"""
from __future__ import annotations
from dataclasses import dataclass
import argparse
import math
from pathlib import Path
import numpy as np
import vtk
from vtk.util.numpy_support import vtk_to_numpy
from PIL import Image, ImageDraw, ImageFont
import build_boat_rough_models as boats

ROOT=Path(__file__).resolve().parent
OUT=ROOT/'turntables'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'


def vtk_actors(mesh):
    points=vtk.vtkPoints()
    for v in mesh.v:points.InsertNextPoint(*v)
    shared={}
    for name,material,faces in mesh.parts:
        shared.setdefault(material,[]).extend(faces)
    out=[]
    for material,faces in shared.items():
        polys=vtk.vtkCellArray()
        for a,b,c in faces:
            tri=vtk.vtkTriangle()
            for idx,vert in enumerate((a-1,b-1,c-1)):
                tri.GetPointIds().SetId(idx,vert)
            polys.InsertNextCell(tri)
        poly=vtk.vtkPolyData();poly.SetPoints(points);poly.SetPolys(polys)
        normals=vtk.vtkPolyDataNormals()
        normals.SetInputData(poly)
        normals.SetFeatureAngle(15)
        normals.SplittingOn()
        normals.SetConsistency(False)
        normals.AutoOrientNormalsOff()
        normals.Update()
        mapper=vtk.vtkPolyDataMapper();mapper.SetInputConnection(normals.GetOutputPort())
        actor=vtk.vtkActor();actor.SetMapper(mapper)
        actor.GetProperty().SetColor(*boats.MATERIALS[material])
        actor.GetProperty().SetAmbient(.32);actor.GetProperty().SetDiffuse(.65)
        actor.GetProperty().SetSpecular(.05)
        actor.GetProperty().SetInterpolationToFlat()
        actor.GetProperty().SetBackfaceCulling(False)
        out.append(actor)
    return out


def water_actor(size=32, height=-.36):
    divisions=80
    pts=vtk.vtkPoints()
    for i in range(divisions+1):
        for j in range(divisions+1):
            x=-size*.5+size*i/divisions
            z=-size*.5+size*j/divisions
            yy=height+.022*math.sin(x*2.3+z*.8)+.018*math.cos(x*1.2-z*1.7)
            pts.InsertNextPoint(x,yy,z)
    faces=vtk.vtkCellArray()
    for i in range(divisions):
        for j in range(divisions):
            a=i*(divisions+1)+j
            b=a+1;c=a+divisions+1;d=c+1
            for vertices in ((a,c,b),(b,c,d)):
                tri=vtk.vtkTriangle()
                for k,idv in enumerate(vertices):tri.GetPointIds().SetId(k,idv)
                faces.InsertNextCell(tri)
    poly=vtk.vtkPolyData();poly.SetPoints(pts);poly.SetPolys(faces)
    norm=vtk.vtkPolyDataNormals();norm.SetInputData(poly);norm.ComputePointNormalsOn();norm.Update()
    mapper=vtk.vtkPolyDataMapper();mapper.SetInputConnection(norm.GetOutputPort())
    a=vtk.vtkActor();a.SetMapper(mapper)
    a.GetProperty().SetColor(.075,.32,.45)
    a.GetProperty().SetOpacity(.64)
    a.GetProperty().SetAmbient(.58);a.GetProperty().SetDiffuse(.44)
    a.GetProperty().SetSpecular(.20);a.GetProperty().SetSpecularPower(12)
    return a


def scene(mesh, spec, width=520, height=370):
    ren=vtk.vtkRenderer();ren.SetGradientBackground(True)
    ren.SetBackground(.46,.66,.74)
    ren.SetBackground2(.82,.89,.90)
    ren.SetUseDepthPeeling(False)
    win=vtk.vtkRenderWindow();win.SetOffScreenRendering(True)
    win.SetSize(width,height);win.AddRenderer(ren)
    win.SetMultiSamples(2)
    components=vtk_actors(mesh)
    group=vtk.vtkAssembly()
    for component in components:group.AddPart(component)
    ren.AddActor(group)
    waterline= -(.21 if spec.class_name=='tier1' else .33 if spec.class_name=='tier2' else .48)
    sea=water_actor(max(27.,spec.length*2.8),waterline)
    ren.AddActor(sea)
    # Gentle environmental lighting; real hull normals and materials drive color.
    sun=vtk.vtkLight();sun.SetLightTypeToSceneLight()
    sun.SetPosition(-9,18,20);sun.SetFocalPoint(0,0,0)
    sun.SetIntensity(1.15);ren.AddLight(sun)
    fill=vtk.vtkLight();fill.SetLightTypeToSceneLight()
    fill.SetPosition(18,10,-6);fill.SetFocalPoint(0,0,0)
    fill.SetIntensity(.47);ren.AddLight(fill)
    cam=ren.GetActiveCamera()
    cam.ParallelProjectionOn();cam.SetViewUp(0,1,0)
    cam.SetPosition(spec.length*.78,spec.length*.52,spec.length*1.40)
    cam.SetFocalPoint(0,.55 if spec.class_name=='tier3' else .33,0)
    cam.SetParallelScale(max(2.0,spec.length*.54))
    ren.SetClippingRangeExpansion(.10)
    ren.ResetCameraClippingRange()
    return ren,win,group


def render_frame(win, ship, angle, spec, label_font, small_font):
    ship.SetOrientation(0,angle,0)
    win.Render()
    pixels=vtk.vtkWindowToImageFilter()
    pixels.SetInput(win);pixels.SetInputBufferTypeToRGB()
    pixels.ReadFrontBufferOff();pixels.Update()
    img=pixels.GetOutput()
    w,h=img.GetDimensions()[:2]
    raw=vtk_to_numpy(img.GetPointData().GetScalars()).reshape(h,w,3)
    frame=Image.fromarray(np.flipud(raw).copy(),'RGB')
    d=ImageDraw.Draw(frame,'RGBA')
    d.rounded_rectangle((12,11,w-12,71),radius=10,fill=(7,27,40,224),outline=(138,177,184,130),width=1)
    tier=int(spec.class_name[-1]);stage=spec.variant+1
    d.text((23,15),f'TIER {tier}  •  UPGRADE {stage}/6',font=small_font,fill=(185,228,231,255))
    d.text((23,36),spec.title,font=label_font,fill=(255,255,255,255))
    d.rounded_rectangle((12,h-49,w-12,h-11),radius=9,fill=(5,31,44,210))
    d.text((23,h-43),f'ACTUAL LOW-POLY MESH   /   {int(angle):03d}° / 360°',font=small_font,fill=(231,241,236,255))
    d.text((w-155,h-43),'WATER: ILLUSTRATIVE',font=ImageFont.truetype(FONT,10),fill=(175,205,219,255))
    # Turntable progress not faked as movement/physics capture.
    d.arc((w-62,13,w-24,51),start=-90,end=-90+int(angle),fill=(255,182,97,255),width=3)
    return frame


def run(tier=None, frames_count=24, width=520,height=370, only=None):
    OUT.mkdir(exist_ok=True)
    label_font=ImageFont.truetype(BOLD,19)
    small_font=ImageFont.truetype(FONT,13)
    stills=[]
    for spec in boats.ALL:
        if tier and spec.class_name!='tier'+str(tier):continue
        if only and f'{spec.class_name}_{spec.variant:02d}' not in only:continue
        mesh=boats.build(spec)
        ren,win,group=scene(mesh,spec,width,height)
        frames=[]
        for i in range(frames_count):
            angle=360*i/frames_count
            frames.append(render_frame(win,group,angle,spec,label_font,small_font))
        out=OUT/f'{spec.class_name}_{spec.variant:02d}_360.gif'
        frames[0].save(out,save_all=True,append_images=frames[1:],duration=110,loop=0,
                       disposal=2,optimize=True)
        still=OUT/f'{spec.class_name}_{spec.variant:02d}_preview.png'
        frames[3*frames_count//24].save(still,optimize=True)
        stills.append((spec,frames[3*frames_count//24].copy()))
        print('RENDERED',out.name,out.stat().st_size,'bytes',flush=True)
        win.Finalize()
    if tier:
        targets=[(tier,[(s,f) for s,f in stills if s.class_name=='tier'+str(tier)])]
    else:
        targets=[(t,[(s,f) for s,f in stills if s.class_name=='tier'+str(t)]) for t in (1,2,3)]
    for t,subset in targets:
        if len(subset)!=6:continue
        w,h=subset[0][1].size
        sheet=Image.new('RGB',(w*3,h*2),(229,239,241))
        for i,(spec,frame) in enumerate(subset):
            sheet.paste(frame,((i%3)*w,(i//3)*h))
        sheet.save(OUT/f'tier{t}_six_level_gallery.png',optimize=True)
    if not tier and len(stills)==18:
        w,h=stills[0][1].size
        summary=Image.new('RGB',(3*w,6*h),(229,239,241))
        for i,(spec,frame) in enumerate(stills):
            summary.paste(frame,((i%3)*w,(i//3)*h))
        summary.save(OUT/'all_18_levels_contact_sheet.png',optimize=True)

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--tier',type=int,choices=(1,2,3))
    p.add_argument('--frames',type=int,default=24)
    p.add_argument('--only',nargs='*',default=None,help='incremental exact model IDs e.g. tier3_05')
    args=p.parse_args()
    run(args.tier,args.frames,only=set(args.only) if args.only else None)