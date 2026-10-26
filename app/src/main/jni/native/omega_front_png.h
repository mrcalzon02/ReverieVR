#ifndef REVERIE_OMEGA_FRONT_PNG_H
#define REVERIE_OMEGA_FRONT_PNG_H

// Small bounded PNG decoder for non-interlaced tactical sprites. Packed grayscale/indexed 1/2/4-bit and full 8-bit PNG samples are supported.
// zlib is already available to the native Android NDK; no Java bitmap round-trip.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <zlib.h>

namespace reverie { namespace omega_art {
struct Image {
    unsigned width=0, height=0;
    std::vector<uint8_t> rgba;
};

inline uint32_t Big32(const uint8_t *p) {
    return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|
           (uint32_t(p[2])<<8)|uint32_t(p[3]);
}
inline uint8_t Paeth(uint8_t a,uint8_t b,uint8_t c) {
    const int p=int(a)+int(b)-int(c);
    const int pa=p>int(a)?p-int(a):int(a)-p;
    const int pb=p>int(b)?p-int(b):int(b)-p;
    const int pc=p>int(c)?p-int(c):int(c)-p;
    return pa<=pb && pa<=pc?a:pb<=pc?b:c;
}
inline bool DecodePng(const uint8_t *data,size_t length,Image &out) {
    out=Image{};
    static const uint8_t signature[8]={137,80,78,71,13,10,26,10};
    if(!data || length<45 || std::memcmp(data,signature,8)!=0)return false;
    size_t at=8;
    unsigned w=0,h=0;
    uint8_t channels=0,kind=255,depth=0;
    bool header=false,ended=false;
    std::vector<uint8_t> compressed,palette,transparent;
    while(at+12<=length) {
        const uint32_t count=Big32(data+at);
        if(count>length-at-12)return false;
        const uint8_t *tag=data+at+4,*chunk=tag+4;
        const uLong expected_crc=Big32(chunk+count);
        uLong actual_crc=crc32(0L,Z_NULL,0);
        actual_crc=crc32(actual_crc,tag,4);
        actual_crc=crc32(actual_crc,chunk,count);
        if(actual_crc!=expected_crc)return false;
        if(std::memcmp(tag,"IHDR",4)==0) {
            if(header || count!=13 || at!=8)return false;
            w=Big32(chunk);h=Big32(chunk+4);
            if(w==0 || h==0 || w>512 || h>512 ||
               chunk[10]!=0 || chunk[11]!=0 || chunk[12]!=0)return false;
            depth=chunk[8];kind=chunk[9];
            channels=kind==0?1:kind==2?3:kind==3?1:kind==4?2:kind==6?4:0;
            if(!channels)return false;
            if(!(depth==8 || ((kind==0 || kind==3) &&
                   (depth==1 || depth==2 || depth==4))))return false;
            header=true;
        } else if(std::memcmp(tag,"PLTE",4)==0) {
            if(!header || count>768 || count%3!=0)return false;
            palette.assign(chunk,chunk+count);
        } else if(std::memcmp(tag,"tRNS",4)==0) {
            if(!header || count>256)return false;
            transparent.assign(chunk,chunk+count);
        } else if(std::memcmp(tag,"IDAT",4)==0) {
            if(!header || compressed.size()+count>3u*1024u*1024u)return false;
            compressed.insert(compressed.end(),chunk,chunk+count);
        } else if(std::memcmp(tag,"IEND",4)==0) {
            if(count!=0)return false;
            ended=true;break;
        } else if((tag[0]&32u)==0u) {
            // Unknown critical chunk: decoding its pixels would be unsound.
            return false;
        }
        at+=size_t(count)+12u;
    }
    if(!header || !ended || compressed.empty() ||
       (kind==3 && (palette.empty() || palette.size()%3!=0)))return false;
    // PNG filters work on bytes, including packed sub-byte samples.
    const size_t stride=(size_t(w)*channels*depth+7u)/8u;
    const size_t filter_bpp=(size_t(channels)*depth+7u)/8u;
    const size_t scan_size=(stride+1u)*h;
    if(scan_size>2u*1024u*1024u)return false;
    std::vector<uint8_t> encoded(scan_size),raw(stride*h);
    uLongf inflated=static_cast<uLongf>(encoded.size());
    if(uncompress(encoded.data(),&inflated,compressed.data(),
                  static_cast<uLong>(compressed.size()))!=Z_OK ||
       inflated!=encoded.size())return false;
    for(unsigned y=0;y<h;++y) {
        const size_t src=size_t(y)*(stride+1u),dst=size_t(y)*stride;
        const uint8_t filter=encoded[src];
        if(filter>4)return false;
        for(size_t x=0;x<stride;++x) {
            const uint8_t a=x>=filter_bpp?raw[dst+x-filter_bpp]:0u;
            const uint8_t b=y?raw[dst-stride+x]:0u;
            const uint8_t c=y && x>=filter_bpp?
                raw[dst-stride+x-filter_bpp]:0u;
            const uint8_t pred=filter==0?0:filter==1?a:filter==2?b:
                              filter==3?uint8_t((unsigned(a)+b)/2u):Paeth(a,b,c);
            raw[dst+x]=uint8_t(unsigned(encoded[src+1u+x])+pred);
        }
    }
    out.width=w;out.height=h;out.rgba.resize(size_t(w)*h*4u);
    for(size_t i=0;i<size_t(w)*h;++i) {
        const unsigned x=static_cast<unsigned>(i%w);
        const unsigned y=static_cast<unsigned>(i/w);
        uint8_t sample=0;
        const uint8_t *p=nullptr;
        if(depth<8){
            // PNG packs samples high-bit first within each scanline.
            const size_t bit=size_t(x)*depth;
            const unsigned shift=8u-depth-static_cast<unsigned>(bit%8u);
            sample=uint8_t((raw[size_t(y)*stride+bit/8u]>>shift)&
                           ((1u<<depth)-1u));
        }else {
            p=&raw[size_t(y)*stride+size_t(x)*channels];
            sample=p[0];
        }
        uint8_t r=0,g=0,b=0,a=255;
        if(kind==6){r=p[0];g=p[1];b=p[2];a=p[3];}
        else if(kind==2){r=p[0];g=p[1];b=p[2];
            if(transparent.size()>=6 && r==transparent[1] &&
               g==transparent[3] && b==transparent[5])a=0;
        } else if(kind==4){r=g=b=p[0];a=p[1];}
        else if(kind==0){
            const unsigned max_value=(1u<<depth)-1u;
            r=g=b=uint8_t(unsigned(sample)*255u/max_value);
            if(transparent.size()==2u && transparent[0]==0u &&
               transparent[1]==sample)a=0;
        } else {
            const size_t pal=size_t(sample)*3u;
            if(pal+2u>=palette.size()){out=Image{};return false;}
            r=palette[pal];g=palette[pal+1];b=palette[pal+2];
            if(sample<transparent.size())a=transparent[sample];
        }
        out.rgba[i*4]=r;out.rgba[i*4+1]=g;
        out.rgba[i*4+2]=b;out.rgba[i*4+3]=a;
    }
    return true;
}
}} // namespace reverie::omega_art
#endif
