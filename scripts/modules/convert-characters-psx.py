#!/usr/bin/env python3
"""Convert ONLY explicitly selected Characters PSX FBX+PNG pairs to Red Ledger RLBP.

One mesh per character, UV-indexed FBX 7.5+ geometry; preserves triangulated
silhouettes and adapts vertex axes to ReverieVR. Source models stay private.
The output is a static REST-POSE mesh, not a skeletal animation export.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib
from PIL import Image, ImageEnhance

ATLAS_SIDE = 256
MAX_VERTICES = 50000
STRIDE = 10
# Slots are tied to existing Red Ledger simulation, not source geometry.
PATRONS = {
    'rail_worker': ('Character_Male_57', (0.35, 0.48, 0.62)),
    'night_clerk': ('Character_34_Female', (0.24, 0.43, 0.63)),
    'militia_veteran': ('Character_Male_47', (0.38, 0.44, 0.29)),
    'mechanic': ('Character_Male_34', (0.44, 0.42, 0.25)),
    'directorate_courier': ('Character_45_Female', (0.32, 0.37, 0.63)),
    'salihorsk_hauler': ('Character_Male_50', (0.43, 0.36, 0.28)),
}

class Node:
    def __init__(self, name, props, children):
        self.name, self.props, self.children = name, props, children

def children(node, name):
    return [x for x in node.children if x.name == name]

def child(node, name):
    found = children(node, name)
    if not found:
        raise ValueError('Missing FBX node ' + name)
    return found[0]

def parse_fbx(data):
    if not data.startswith(b'Kaydara FBX Binary  \0\x1a\0'):
        raise ValueError('Expected binary FBX, not ASCII FBX')
    version = struct.unpack_from('<I', data, 23)[0]
    if not 7500 <= version <= 7900:
        raise ValueError('Unsupported FBX binary version ' + str(version))
    fmt, header_size = '<QQQB', 25
    def read(pos, depth=0):
        if depth > 60: raise ValueError('FBX nesting too deep')
        end, count, prop_bytes, namelen = struct.unpack_from(fmt, data, pos)
        if not end:
            return None, pos + header_size
        if end > len(data) or end <= pos or count > 100000:
            raise ValueError('Invalid FBX node offset or property count')
        name = data[pos+header_size:pos+header_size+namelen].decode('utf-8')
        at = pos+header_size+namelen
        props = []
        for _ in range(count):
            ty = chr(data[at]); at += 1
            if ty in 'YILFDC':
                form = {'Y':'<h', 'I':'<i', 'L':'<q', 'F':'<f', 'D':'<d', 'C':'<?'}[ty]
                props.append(struct.unpack_from(form,data,at)[0]);at+=struct.calcsize(form)
            elif ty in 'SR':
                n=struct.unpack_from('<I',data,at)[0];at+=4
                raw=data[at:at+n];at+=n
                props.append(raw.decode('utf8','replace') if ty=='S' else raw)
            elif ty in 'fdilbc':
                n, enc, nbytes = struct.unpack_from('<III',data,at);at+=12
                raw=data[at:at+nbytes];at+=nbytes
                if n > 2000000:raise ValueError('Oversized FBX array')
                if enc==1:raw=zlib.decompress(raw)
                elif enc!=0:raise ValueError('Unknown FBX compression')
                form={'f':'f','d':'d','i':'i','l':'q','b':'b','c':'B'}[ty]
                if len(raw)!=n*struct.calcsize('<'+form):
                    raise ValueError('FBX array size mismatch')
                props.append(struct.unpack('<'+str(n)+form,raw))
            else:raise ValueError('Unsupported FBX field '+ty)
        if at!=pos+header_size+namelen+prop_bytes:
            raise ValueError('Invalid FBX property length in '+name)
        descend=[]
        while at<end-header_size:
            nested,at=read(at,depth+1)
            if nested is None:break
            descend.append(nested)
        return Node(name,props,descend),end
    root=[]; at=27
    while at<len(data)-header_size:
        node,at=read(at)
        if node is None:break
        root.append(node)
    return root

def vertices_from_fbx(data):
    roots=parse_fbx(data)
    objects=next((x for x in roots if x.name=='Objects'), None)
    if objects is None:raise ValueError('FBX has no Objects')
    meshes=[x for x in children(objects,'Geometry') if len(x.props)>2 and x.props[2]=='Mesh']
    if len(meshes)!=1:
        raise ValueError('Selected patron must have exactly one mesh, found '+str(len(meshes)))
    mesh=meshes[0]
    pos=child(mesh,'Vertices').props[0]
    polygon=child(mesh,'PolygonVertexIndex').props[0]
    uvlayer=child(mesh,'LayerElementUV')
    mapping=child(uvlayer,'MappingInformationType').props[0]
    reference=child(uvlayer,'ReferenceInformationType').props[0]
    if mapping!='ByPolygonVertex' or reference!='IndexToDirect':
        raise ValueError('Unsupported FBX UV mapping '+mapping+'/'+reference)
    uvs=child(uvlayer,'UV').props[0]
    uvid=child(uvlayer,'UVIndex').props[0]
    if len(pos)%3 or len(uvs)%2 or len(polygon)!=len(uvid):
        raise ValueError('Bad FBX mesh/UV indexing')
    # Normalize each character to a consistent height. Their upstream axes are
    # X-forward, Z-up; ReverieVR uses Z-forward, Y-up, X-right.
    zs=pos[2::3]
    floor=min(zs);height=max(zs)-floor
    if not (0.5<=height<=3.5):raise ValueError('Unexpected character height')
    scale=1.68/height
    all_vertices=[];poly=[]
    for corner,value in enumerate(polygon):
        end=value<0
        source_index=-value-1 if end else value
        ui=uvid[corner]
        if not (0<=source_index<len(pos)//3 and 0<=ui<len(uvs)//2):
            raise ValueError('FBX index out of range')
        sx,sy,sz=pos[source_index*3:source_index*3+3]
        u,v=uvs[ui*2:ui*2+2]
        if not all(math.isfinite(n) for n in [sx,sy,sz,u,v]):
            raise ValueError('Nonfinite geometry/texture coordinates')
        if not (-0.0001<=u<=1.0001 and -0.0001<=v<=1.0001):
            raise ValueError('UV outside atlas')
        # Desired foot position local to the current existing patron anchor:
        # -0.78 + (-0.47) = -1.25 world; head at +0.43 world.
        x=sy*scale; y=(sz-floor)*scale-0.78; z=sx*scale
        poly.append((x,y,z,max(0.,min(1.,u)),max(0.,min(1.,v)),1.,1.,1.,0.,0.))
        if end:
            if len(poly)<3:raise ValueError('Degenerate polygon')
            for tri in range(1,len(poly)-1):
                all_vertices.extend(poly[0]);all_vertices.extend(poly[tri]);all_vertices.extend(poly[tri+1])
            poly.clear()
    if poly:raise ValueError('Unclosed FBX polygon')
    count=len(all_vertices)//STRIDE
    if count==0 or count>MAX_VERTICES or count%3:raise ValueError('Too many/few triangles')
    return all_vertices, {'source_mesh_vertices':len(pos)//3,'triangles':count//3,
                          'height_m':1.68,'source_height_m':height}

def retexture(source, target):
    # Reuse approved UV placement; generate game-specific faction-tinted cloth
    # without painting over exposed skin, eyes, faces or other skin-toned parts.
    im=Image.open(source).convert('RGB').resize((ATLAS_SIDE,ATLAS_SIDE),Image.Resampling.BOX)
    px=im.load();rgb=[]
    for y in range(ATLAS_SIDE-1,-1,-1): # GL pixel row zero is the bottom row
        for x in range(ATLAS_SIDE):
            r,g,b=px[x,y]
            high=max(r,g,b); low=min(r,g,b)
            warm=(r>g*1.12 and r>b*1.22 and r>=72 and (high-low)>18)
            # Preserve skin and dark near-neutral detail. Cloth gets a
            # controlled chroma lift, preserving light/dark shading.
            if not warm and high>22:
                lum=(r*0.24+g*0.61+b*0.15)/255.0
                saturation=(high-low)/max(1,high)
                strength=0.60 if saturation>0.12 else 0.22
                r=int(max(0,min(255,r*(1-strength)+255*target[0]*lum*strength)))
                g=int(max(0,min(255,g*(1-strength)+255*target[1]*lum*strength)))
                b=int(max(0,min(255,b*(1-strength)+255*target[2]*lum*strength)))
            rgb.append(((r>>3)<<11)|((g>>2)<<5)|(b>>3))
    return struct.pack('<'+str(len(rgb))+'H',*rgb)

def convert(source,texture,output,tint):
    fb=source.read_bytes();geom,meta=vertices_from_fbx(fb)
    tex=retexture(texture,tint)
    count=len(geom)//STRIDE
    result=struct.pack('<4sIII',b'RLBP',1,count,ATLAS_SIDE)
    result+=struct.pack('<'+str(len(geom))+'f',*geom)+tex
    if len(result)!=16+count*40+256*256*2:
        raise ValueError('Incorrect RLBP length')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(result)
    meta.update({'fbx_sha256':hashlib.sha256(fb).hexdigest(),
                 'source_texture_sha256':hashlib.sha256(texture.read_bytes()).hexdigest(),
                 'output_sha256':hashlib.sha256(result).hexdigest(),
                 'bytes':len(result),'path':output.name})
    return meta

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True,help='PRIVATE folder containing ONLY the six selected FBX and PNG pairs')
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    manifest={'schema':1,'format':'RLBP-1','license':'Commercial game use allowed; redistribution of source asset collections prohibited',
              'static_pose_only':True,'source_url':'https://elbolilloduro.itch.io/characters-psx', 'characters':[]}
    for role,(stem,tint) in PATRONS.items():
        fbx=args.input/(stem+'.fbx.bin')
        if not fbx.exists():fbx=args.input/(stem+'.fbx')
        png=args.input/(stem+'.png')
        if not fbx.is_file() or not png.is_file():raise ValueError('Missing selected private pair '+stem)
        out=args.output/('patron_'+role+'.rlprop')
        meta=convert(fbx,png,out,tint)
        meta.update({'role':role,'original_stem':stem})
        manifest['characters'].append(meta)
    (args.output/'selection.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
    print(json.dumps(manifest,indent=2))
if __name__=='__main__':main()
