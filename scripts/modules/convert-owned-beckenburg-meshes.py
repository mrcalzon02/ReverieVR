#!/usr/bin/env python3
"""Recreate exactly the two selected BWM1 models from privately owned Beckenburg originals.
Does not download, copy, publish, or redistribute the original ZIP.
"""
import argparse, struct, json, math, pathlib, hashlib

def rnd(v):return math.floor(v+0.5)
def clustered(points,faces,grid,colors,shade_base,shade_intensity):
    clusters={};mapping=[]
    for pt in points:
        k=tuple(rnd(pt[j]/grid[j]) for j in range(3))
        if k not in clusters:clusters[k]=[len(clusters),0,[0.,0.,0.]]
        group=clusters[k];group[1]+=1
        for j in range(3):group[2][j]+=pt[j]
        mapping.append(group[0])
    centers=[None]*len(clusters)
    for idx,n,summation in clusters.values():centers[idx]=[v/n for v in summation]
    seen=set();kept=[]
    for face in faces:
        f=tuple(mapping[i] for i in face)
        if len(set(f))<3:continue
        key=tuple(sorted(f))
        if key in seen:continue
        seen.add(key);kept.append(f)
    result=bytearray(b'BWM1'+struct.pack('<I',len(kept)))
    for f in kept:
        a,b,c=[centers[i] for i in f]
        ab=[b[i]-a[i] for i in range(3)]
        ac=[c[i]-a[i] for i in range(3)]
        ny=ab[2]*ac[0]-ab[0]*ac[2]
        nn=math.hypot(ab[1]*ac[2]-ab[2]*ac[1],ny,ab[0]*ac[1]-ab[1]*ac[0])
        shade=shade_base+shade_intensity*(max(-1.,min(1.,ny/nn)) if nn>1e-8 else 0)
        color=[rnd(max(0.,min(255.,v*shade))) for v in colors]
        for pt in [a,b,c]:
            result+=struct.pack('<hhhBBB',*[rnd(v*8192) for v in pt],*color)
    return result,len(centers)

def boat(path):
    src=path.read_bytes();length=struct.unpack_from('<I',src,12)[0];doc=json.loads(src[20:20+length]);binoff=20+length+8
    ac=doc['accessors'];views=doc['bufferViews'];mesh=doc['meshes'][0]['primitives'][0]
    def arr(i,stride,fmt):
        a=ac[i];v=views[a['bufferView']];off=binoff+v.get('byteOffset',0)+a.get('byteOffset',0);return [struct.unpack_from(fmt,src,off+j*(v.get('byteStride') or stride)) for j in range(a['count'])]
    pts=arr(mesh['attributes']['POSITION'],12,'<fff');ids=arr(mesh['indices'],4,'<I');f=[tuple(ids[i+j][0] for j in range(3)) for i in range(0,len(ids),3)]
    return clustered(pts,f,[.05,.05,.05],[142,150,154],.73,.25)

def apc(path):
    pts=[];faces=[]
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith('v '):pts.append(tuple(float(x) for x in line[2:].strip().split()[:3]))
        elif line.startswith('f '):
            ids=[int(part.split('/')[0])-1 for part in line[2:].strip().split()]
            for i in range(1,len(ids)-1):faces.append((ids[0],ids[i],ids[i+1]))
    mins=[min(p[i] for p in pts) for i in range(3)];maxs=[max(p[i] for p in pts) for i in range(3)]
    mids=[(mins[i]+maxs[i])/2 for i in range(3)];scales=[n/(maxs[i]-mins[i]) for i,n in enumerate([.66,.24,.50])]
    pts=[tuple((p[i]-mids[i])*scales[i] for i in range(3)) for p in pts]
    return clustered(pts,faces,[.022,.018,.022],[84,103,76],.75,.20)

def run():
    parser=argparse.ArgumentParser(description="Rebuild selected owned meshes without redistributing source packs")
    parser.add_argument("--boat",type=pathlib.Path,required=True,help="owner's original Models/boat.glb")
    parser.add_argument("--apc",type=pathlib.Path,required=True,help="owner's original Models/APC.obj")
    parser.add_argument("--output-dir",type=pathlib.Path,required=True)
    a=parser.parse_args()
    sources=[(a.boat,"8f0217c57364afb521a46582ad857b8f3906615490bb98e85e72448481b1e53b"),
             (a.apc,"2d4b99f964198680cafc1ea802747deab93d2a1e751ee84876e0ea5c0b9c6136")]
    for path,expected in sources:
        if hashlib.sha256(path.read_bytes()).hexdigest()!=expected:
            raise ValueError("Source hash changed: review before deriving " + str(path))
    a.output_dir.mkdir(parents=True,exist_ok=True)
    for name,fcn,expected,path in [
        ("owner_boat_rowboat.bwm",boat,"089a616c268188f5f37ea68c5cdd3fe6689b3889",a.boat),
        ("owner_apc_armor.bwm",apc,"f74706ae039f4567615f27eb2633e7fdbc4b4a1d",a.apc)]:
        content,clusters=fcn(path)
        if len(content)<35 or len(content)>8+2048*27 or content[:4]!=b"BWM1":
            raise ValueError("Invalid or oversized converted BWM1")
        blob=hashlib.sha1(b"blob "+str(len(content)).encode()+bytes([0])+content).hexdigest()
        if blob!=expected:
            raise ValueError("Converted output differs from the accepted game asset: "+name+" "+blob)
        (a.output_dir/name).write_bytes(content)
        print(name,"bytes",len(content),"triangles",struct.unpack_from("<I",content,4)[0],
              "clusters",clusters,"blob_sha",blob,"PASS")

if __name__=="__main__":
    run()
