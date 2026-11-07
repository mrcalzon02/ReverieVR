from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageChops
import zipfile,json,struct,hashlib
P=Path(__file__).resolve().parent
frames=sorted((P/'renders').glob('frame_*.png'))
assert len(frames)==24,len(frames)
assert ImageChops.difference(Image.open(frames[0]).convert('RGB'),Image.open(frames[7]).convert('RGB')).getbbox()
ims=[]
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',17)
for f in frames:
    im=Image.open(f).convert('RGB');d=ImageDraw.Draw(im);d.rectangle((0,0,im.width,47),fill=(16,27,33))
    d.text((14,7),'SITKA | MAXIMUM HELM — ACTUAL MODEL',fill='white',font=font)
    d.text((14,28),'Texture animation / illustrative readings / no game integration',fill=(187,205,210))
    ims.append(im)
ims[0].save(P/'Maximum_Helm_Demo.gif',save_all=True,append_images=ims[1:],duration=125,loop=0)
im=Image.open(P/'renders/maximum_helm.png').convert('RGB');im.save(P/'renders/maximum_helm.jpg',quality=88)
raw=(P/'Sitka_Maximum_Helm.glb').read_bytes();magic,version,length=struct.unpack_from('<4sII',raw)
assert magic==b'glTF' and version==2 and length==len(raw)
n,typ=struct.unpack_from('<II',raw,12);data=json.loads(raw[20:20+n]);assert data['meshes']
report={'glb_bytes':len(raw),'glb_meshes':len(data['meshes']),'render_frames':len(frames),'animation_frames_differ':True,'gauge_texture_frames':len(list((P/'textures').glob('gauges_*.png'))),'blend_bytes':(P/'Sitka_Maximum_Helm.blend').stat().st_size}
(P/'validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
with zipfile.ZipFile(P/'Sitka_Maximum_Helm_Package.zip','w',zipfile.ZIP_DEFLATED) as z:
    for f in sorted(P.rglob('*')):
        if f.is_file() and f.suffix not in {'.zip','.blend1','.log'} and not f.name.startswith('frame_'):z.write(f,f.relative_to(P))
