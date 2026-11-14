#!/usr/bin/env python3
"""Convert an actual 512x256 native GLES2 RGB framebuffer to PNG, sans Pillow."""
from pathlib import Path
import argparse
import struct
import zlib

HEADER=b"P6\n512 256\n255\n"
WIDTH=512
HEIGHT=256
def chunk(tag,data):
    return (struct.pack(">I",len(data))+tag+data+
            struct.pack(">I",zlib.crc32(tag+data)&0xffffffff))
def convert(src,dst):
    raw=Path(src).read_bytes()
    expected=WIDTH*HEIGHT*3
    if len(raw)!=len(HEADER)+expected or not raw.startswith(HEADER):
        raise ValueError("not a complete original native GLES2 P6 framebuffer")
    pixels=raw[len(HEADER):]
    scan=b"".join(b"\x00"+pixels[i*WIDTH*3:(i+1)*WIDTH*3]
                  for i in range(HEIGHT))
    result=(b"\x89PNG\r\n\x1a\n"+
            chunk(b"IHDR",struct.pack(">IIBBBBB",WIDTH,HEIGHT,8,2,0,0,0))+
            chunk(b"IDAT",zlib.compress(scan,9))+
            chunk(b"IEND",b""))
    target=Path(dst)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(result)
    return len(result)
if __name__=="__main__":
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input",type=Path)
    ap.add_argument("output",type=Path)
    a=ap.parse_args()
    print("Rendered genuine GLES2 screenshot PNG bytes:",convert(a.input,a.output))
