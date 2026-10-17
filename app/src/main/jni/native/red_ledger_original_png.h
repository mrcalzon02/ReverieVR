#ifndef REVERIE_RED_LEDGER_PNG_H
#define REVERIE_RED_LEDGER_PNG_H

// Bounded RGBA decoder for owner's true UE Editor-exported 300px icons and
// 1024px source product textures. Unsupported PNG variants fail closed. No thumbnail fallback.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <zlib.h>

namespace reverie { namespace redledger { namespace original_png {

inline uint32_t Read32(const uint8_t *p) {
    return (static_cast<uint32_t>(p[0]) << 24)
        | (static_cast<uint32_t>(p[1]) << 16)
        | (static_cast<uint32_t>(p[2]) << 8)
        | static_cast<uint32_t>(p[3]);
}
inline int Paeth(int a,int b,int c) {
    const int p=a+b-c;
    const int da=p>a?p-a:a-p;
    const int db=p>b?p-b:b-p;
    const int dc=p>c?p-c:c-p;
    return da<=db && da<=dc?a:(db<=dc?b:c);
}
inline bool DecodeRgba(const uint8_t *data,size_t length,
                       size_t side,std::vector<uint8_t> *out) {
    const uint8_t signature[8]={137,80,78,71,13,10,26,10};
    if(!data || !out || length<33 ||
       (side!=300u && side!=1024u) ||
       !std::equal(signature,signature+8,data)) return false;
    const size_t stride=side*4u, decoded_bytes=side*(stride+1u);
    std::vector<uint8_t> idat;
    bool saw_ihdr=false,saw_iend=false;
    size_t p=8;
    while(p+12<=length) {
        const uint32_t size=Read32(data+p);
        if(size>8u*1024u*1024u || size>length-p-12u) return false;
        const uint8_t *tag=data+p+4;
        const uint8_t *bytes=data+p+8;
        const uLong crc=crc32(crc32(0L,tag,4),bytes,size);
        if(static_cast<uint32_t>(crc)!=Read32(bytes+size)) return false;
        const bool ihdr=tag[0]=='I'&&tag[1]=='H'&&tag[2]=='D'&&tag[3]=='R';
        const bool chunk_idat=tag[0]=='I'&&tag[1]=='D'&&tag[2]=='A'&&tag[3]=='T';
        const bool iend=tag[0]=='I'&&tag[1]=='E'&&tag[2]=='N'&&tag[3]=='D';
        if(ihdr) {
            if(saw_ihdr || p!=8 || size!=13 || Read32(bytes)!=side ||
               Read32(bytes+4)!=side || bytes[8]!=8 || bytes[9]!=6 ||
               bytes[10]!=0 || bytes[11]!=0 || bytes[12]!=0) return false;
            saw_ihdr=true;
        } else if(chunk_idat) {
            if(!saw_ihdr || saw_iend || idat.size()+size>4u*1024u*1024u)
                return false;
            idat.insert(idat.end(),bytes,bytes+size);
        } else if(iend) {
            if(!saw_ihdr || idat.empty() || size!=0) return false;
            saw_iend=true;
        } else if((tag[0]&0x20u)==0u) {
            // Unknown critical chunks would change the pixel semantics.
            return false;
        }
        p+=static_cast<size_t>(size)+12u;
        if(saw_iend)break;
    }
    if(!saw_iend || p!=length) return false;
    std::vector<uint8_t> filtered(decoded_bytes);
    uLongf target=static_cast<uLongf>(filtered.size());
    if(uncompress(filtered.data(),&target,idat.data(),
                  static_cast<uLong>(idat.size()))!=Z_OK ||
       target!=decoded_bytes) return false;
    out->assign(side*stride,0u);
    for(size_t y=0;y<side;++y) {
        const uint8_t *src=filtered.data()+y*(stride+1u);
        const uint8_t filter=src[0];
        if(filter>4u)return false;
        uint8_t *dst=out->data()+y*stride;
        const uint8_t *prev=y?dst-stride:nullptr;
        for(size_t x=0;x<stride;++x) {
            const int left=x>=4u?dst[x-4u]:0;
            const int up=prev?prev[x]:0;
            const int ul=prev&&x>=4u?prev[x-4u]:0;
            int predictor=0;
            if(filter==1u)predictor=left;
            if(filter==2u)predictor=up;
            if(filter==3u)predictor=(left+up)/2;
            if(filter==4u)predictor=Paeth(left,up,ul);
            dst[x]=static_cast<uint8_t>(src[1u+x]+predictor);
        }
    }
    return true;
}
// Low-memory GPU upload: retain the intact 1024px master in APK storage,
// but use a deterministic 2x2 box filter for the Galaxy S9's GLES2 VR
// texture. The original decoded pixels are unmodified.
inline bool Reduce1024RgbaTo512(const std::vector<uint8_t> &source,
                                std::vector<uint8_t> *reduced) {
    constexpr size_t side=1024u,half=512u;
    if(!reduced || source.size()!=side*side*4u)return false;
    std::vector<uint8_t> result(half*half*4u);
    for(size_t y=0;y<half;++y)for(size_t x=0;x<half;++x) {
        const size_t input=(2u*y*side+2u*x)*4u;
        const size_t output=(y*half+x)*4u;
        for(size_t c=0;c<4u;++c) {
            const unsigned int sum=unsigned(source[input+c])+
                unsigned(source[input+4u+c])+
                unsigned(source[input+side*4u+c])+
                unsigned(source[input+side*4u+4u+c]);
            result[output+c]=static_cast<uint8_t>((sum+2u)/4u);
        }
    }
    reduced->swap(result);
    return true;
}
inline bool Decode300Rgba(const uint8_t *data,size_t length,
                          std::vector<uint8_t> *out) {
    return DecodeRgba(data,length,300u,out);
}
}}}
#endif
