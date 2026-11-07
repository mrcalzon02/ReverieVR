from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math
OUT=Path(__file__).resolve().parent/'textures'; OUT.mkdir(exist_ok=True)
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',12)
small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',10)
for frame in range(1,25):
    im=Image.new('RGB',(512,128),(24,28,30)); d=ImageDraw.Draw(im)
    for i,label in enumerate(['FUEL','TRIM','VOLTS','RPM']):
        x=i*128; d.ellipse((x+4,4,x+124,124),fill=(221,215,184),outline=(58,63,59),width=4)
        for k in range(11):
            a=math.radians(140+260*k/10)
            p=[(x+64+r*math.cos(a),64+r*math.sin(a)) for r in (45,53)]
            d.line(p,fill=(34,46,44) if k<9 else (173,50,33),width=3)
        v=[.67+.015*math.sin(frame*math.pi/12),.45+.18*math.sin(frame*math.pi/12),.7+.025*math.sin(frame*math.pi/6),.48+.30*math.sin(frame*math.pi/12)][i]
        a=math.radians(140+260*v)
        d.line([(x+64,64),(x+64+42*math.cos(a),64+42*math.sin(a))],fill=(175,49,30),width=4)
        d.ellipse((x+59,59,x+69,69),fill=(38,45,44)); d.text((x+64,92),label,font=font,anchor='mm',fill=(28,35,34))
    im.save(OUT/f'gauges_{frame:03d}.png')
for mode in ['CHART','SONAR','RADAR','CAMERA']:
    im=Image.new('RGB',(256,192),(13,35,47));d=ImageDraw.Draw(im)
    if mode=='CHART':
        for j,c in enumerate([(25,73,98),(34,103,124),(65,137,139),(139,157,91)]):
            d.polygon([(0,0),(170-j*24,0),(150-j*25,42),(187-j*27,83),(142-j*26,140),(160-j*30,192),(0,192)],fill=c)
        d.polygon([(181,89),(170,114),(190,109)],fill='white')
    elif mode=='SONAR':
        for y in range(30,180,30): d.line((0,y,256,y),fill=(35,65,77))
        pts=[(x,150+int(13*math.sin(x*.03)+7*math.sin(x*.09))) for x in range(256)]
        d.polygon(pts+[(255,192),(0,192)],fill=(181,99,41));d.line(pts,fill=(245,207,78),width=5)
    elif mode=='RADAR':
        for r in (28,54,80): d.ellipse((128-r,96-r,128+r,96+r),outline=(43,119,80),width=2)
        d.line((128,96,189,45),fill=(123,221,140),width=3)
        for x,y in [(168,66),(91,123),(146,134)]:d.rectangle((x,y,x+4,y+4),fill=(181,240,164))
    else:
        d.rectangle((0,25,256,105),fill=(101,138,152));d.rectangle((0,106,256,192),fill=(35,73,90))
        d.polygon([(0,140),(58,101),(81,130),(160,98),(256,143)],fill=(62,75,72))
    d.rectangle((0,0,256,22),fill=(7,18,24));d.text((8,4),mode+' / DEMO',font=font,fill=(233,230,195))
    im.save(OUT/(mode.lower()+'.png'))
im=Image.new('RGB',(192,80),(65,99,48));d=ImageDraw.Draw(im)
d.text((10,8),'VHF / DEMO',font=font,fill=(202,224,163))
d.text((10,34),'CH 16   STBY',font=font,fill=(202,224,163))
for j in range(7):d.rectangle((115+j*9,65-j*4,120+j*9,70),fill=(151,183,106))
im.save(OUT/'vhf.png')
print('24 gauge atlases, four illustrative screen modes and VHF face generated')
