#!/usr/bin/env python3
"""Reproduce Red Ledger's two owner-authored *static* civilian RLBP models.

Requires numpy, trimesh, Pillow. Original GLBs stay in private Beckenburg
Drive. No game asset is inferred or generated when source bytes mismatch.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import numpy as np
import trimesh
from PIL import Image

SOURCES={
    "patron_gopnick": (
        "gopnickboss character 3d model.glb",
        "e305464fed1d32411310cb49f1527086e2e75535916c379d6d18cd0a17a520a8",
        "cf4d3415f7e87d748c43513caf6f903203afc62af658d616be47ea197659aeb0"),
    "patron_pink_dress": (
        "woman in pink dress 3d model.glb",
        "a58c41fe485a67a7b6d05f081fb083991ade8db97b8d5b3ae9d10042ebdcd51d",
        "4e0fa322d3c8e9f4aef9c8d51cc7c0609f6fc6fe2d1aaaf98d70e32bd3a874de")
}
PITCH=.025
UV_PITCH=.07
DIMS=(.82,1.56,.42)
MAX_TRIANGLES=14000

def digest(data):
    return hashlib.sha256(data).hexdigest()

def convert(path):
    original=path.read_bytes()
    if original[:4]!=b"glTF" or struct.unpack_from("<I",original,4)[0]!=2:
        raise ValueError("GLB version/signature mismatch")
    jlen=struct.unpack_from("<I",original,12)[0]
    document=json.loads(original[20:20+jlen])
    if document.get("skins") or document.get("animations"):
        raise ValueError("Source rigging or animation changed: manual review required")
    image_view=document["bufferViews"][document["images"][0]["bufferView"]]
    image_offset=20+jlen+8+image_view.get("byteOffset",0)
    image=Image.open(io.BytesIO(
        original[image_offset:image_offset+image_view["byteLength"]])).convert("RGB")
    mesh=trimesh.load(path,force="mesh",process=False)
    positions=np.asarray(mesh.vertices,dtype=np.float64)
    faces=np.asarray(mesh.faces,dtype=np.int64)
    uv=np.asarray(mesh.visual.uv,dtype=np.float64)
    if len(uv)!=len(positions) or not np.isfinite(positions).all() or (
        not np.isfinite(uv).all()):
        raise ValueError("Source UV/geometry missing or nonfinite")
    # Compact vertices in spatial/UV clusters, retaining distinct source
    # seams. Real source face winding is preserved, duplicate triangles drop.
    keys=np.column_stack([
        np.round(positions/PITCH).astype(np.int32),
        np.round(uv/UV_PITCH).astype(np.int32)])
    _,groups=np.unique(keys,axis=0,return_inverse=True)
    denom=np.bincount(groups)
    centers=np.column_stack([
        np.bincount(groups,weights=positions[:,i])/denom for i in range(3)])
    mapped_uv=np.column_stack([
        np.bincount(groups,weights=uv[:,i])/denom for i in range(2)])
    triangles=groups[faces]
    valid=(triangles[:,0]!=triangles[:,1]) & (
        triangles[:,1]!=triangles[:,2]) & (triangles[:,2]!=triangles[:,0])
    triangles=triangles[valid]
    _,keep=np.unique(np.sort(triangles,axis=1),axis=0,return_index=True)
    triangles=triangles[np.sort(keep)]
    if not 200<=len(triangles)<=MAX_TRIANGLES:
        raise ValueError("Source model exceeds accepted RLBP triangle budget")
    lower,upper=positions.min(axis=0),positions.max(axis=0)
    if np.any(upper-lower<=1e-8):
        raise ValueError("Degenerate source mesh")
    scaled=(centers-(upper+lower)*.5)/(upper-lower)
    xyz=np.column_stack([
        scaled[:,1]*DIMS[0],
        scaled[:,2]*DIMS[1],
        scaled[:,0]*DIMS[2]])
    tris=xyz[triangles]
    norms=np.cross(tris[:,1]-tris[:,0],tris[:,2]-tris[:,0])
    norm=np.linalg.norm(norms,axis=1)
    shade=np.clip(.86+.14*np.divide(norms[:,1],np.maximum(norm,1e-9)),.72,1.)
    verts=np.zeros((3*len(triangles),10),dtype="<f4")
    verts[:,:3]=tris.reshape(-1,3)
    uv_faces=mapped_uv[triangles].reshape(-1,2)
    verts[:,3]=np.clip(uv_faces[:,0],0,1)
    verts[:,4]=1.-np.clip(uv_faces[:,1],0,1)
    verts[:,5:8]=np.repeat(shade,3)[:,None]
    atlas=image.resize((64,64),Image.Resampling.LANCZOS).resize(
        (256,256),Image.Resampling.NEAREST)
    pixels=np.asarray(atlas,dtype=np.uint16)
    rgb565=((pixels[:,:,0]>>3)<<11)|((pixels[:,:,1]>>2)<<5)|(pixels[:,:,2]>>3)
    data=struct.pack("<4sIII",b"RLBP",1,len(verts),256)+(
        verts.tobytes())+rgb565.astype("<u2").tobytes()
    if len(data)!=16+40*len(verts)+256*256*2:
        raise ValueError("Invalid RLBP output length")
    return data, len(faces), len(triangles)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    for name,(filename,sha,expected) in SOURCES.items():
        path=args.source/filename
        if not path.is_file() or digest(path.read_bytes())!=sha:
            raise ValueError("Unverified owner GLB source: "+filename)
        output,source_faces,target_faces=convert(path)
        converted_sha=digest(output)
        if converted_sha!=expected:
            raise ValueError("Source-derived geometry changed for "+name+
                             "; manual review required: "+converted_sha)
        destination=args.output/(name+".rlprop")
        destination.write_bytes(output)
        print(name,source_faces,target_faces,len(output),converted_sha,"PASS")

if __name__=="__main__":
    main()
