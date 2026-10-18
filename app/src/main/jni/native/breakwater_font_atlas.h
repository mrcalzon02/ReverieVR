#ifndef REVERIE_BREAKWATER_FONT_ATLAS_H
#define REVERIE_BREAKWATER_FONT_ATLAS_H
// Genuine proportional Lato Semibold outlines baked to an 8-level alpha atlas,
// not the former 5x7 native stroke font. Offline-only; one 512x64 GL_ALPHA
// texture shared by both eyes, 0 texture uploads during stereo draws.
// Copyright (c) 2010-2011 Lukasz Dziedzic; Lato licensed under SIL OFL 1.1.
// The font is embedded as derived alpha pixels, not a runtime font dependency.
// Licensing: LICENSES/OFL-1.1.txt
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <zlib.h>
namespace reverie { namespace breakwater { namespace font {
constexpr int kWidth=512, kHeight=64;
struct Glyph {int ch,x,y,w,h,bx,by,advance;};
inline const Glyph* Lookup(unsigned char c) {
    if(c>='a' && c<='z')c=static_cast<unsigned char>(c-'a'+'A');
    static const Glyph kGlyphs[] = {
        {32,2,2,6,1,0,0,6},
        {65,11,2,19,19,-1,-19,18},
        {66,33,2,17,19,0,-19,17},
        {67,53,2,17,19,0,-19,17},
        {68,73,2,20,19,0,-19,20},
        {69,96,2,15,19,0,-19,15},
        {70,114,2,15,19,0,-19,15},
        {71,132,2,19,19,0,-19,19},
        {72,154,2,20,19,0,-19,20},
        {73,177,2,8,19,0,-19,8},
        {74,188,2,11,19,0,-19,11},
        {75,202,2,18,19,0,-19,18},
        {76,223,2,14,19,0,-19,13},
        {77,240,2,24,19,0,-19,24},
        {78,267,2,20,19,0,-19,20},
        {79,290,2,21,19,0,-19,21},
        {80,314,2,16,19,0,-19,16},
        {81,333,2,21,23,0,-19,21},
        {82,357,2,17,19,0,-19,17},
        {83,377,2,14,19,0,-19,14},
        {84,394,2,16,19,0,-19,15},
        {85,413,2,19,19,0,-19,19},
        {86,435,2,19,19,-1,-19,18},
        {87,457,2,27,19,0,-19,27},
        {88,487,2,17,19,0,-19,17},
        {89,2,28,18,19,-1,-19,17},
        {90,23,28,16,19,0,-19,16},
        {48,42,28,15,19,0,-19,15},
        {49,60,28,15,19,0,-19,15},
        {50,78,28,15,19,0,-19,15},
        {51,96,28,15,19,0,-19,15},
        {52,114,28,15,19,0,-19,15},
        {53,132,28,15,19,0,-19,15},
        {54,150,28,15,19,0,-19,15},
        {55,168,28,15,19,0,-19,15},
        {56,186,28,15,19,0,-19,15},
        {57,204,28,15,19,0,-19,15},
        {47,222,28,12,21,0,-20,12},
        {45,237,28,10,9,0,-9,10},
        {43,250,28,15,16,0,-16,15},
        {58,268,28,7,13,0,-13,7},
        {46,278,28,6,4,0,-4,6},
        {44,287,28,6,8,0,-4,6},
        {33,296,28,7,19,0,-19,7},
        {63,306,28,12,19,0,-19,12},
        {37,321,28,21,19,0,-19,21},
        {40,345,28,7,25,0,-21,7},
        {41,355,28,7,25,0,-21,7},
        {39,365,28,5,19,0,-19,5},
        {38,373,28,19,19,0,-19,19},
    };
    for(const Glyph& g:kGlyphs)if(g.ch==c)return &g;
    for(const Glyph& g:kGlyphs)if(g.ch=='?')return &g;
    return nullptr;
}
inline bool DecodeAtlas(std::vector<unsigned char>& pixels) {
    static const char encoded[]=
        "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"
        ;
    std::vector<unsigned char> compressed;
    compressed.reserve(sizeof(encoded)*3/4);
    uint32_t buffer=0;int bits=0;
    for(const unsigned char *p=reinterpret_cast<const unsigned char*>(encoded);*p;++p){
        const unsigned char c=*p;
        if(c=='=')break;
        const int digit=c>='A' && c<='Z'?c-'A':
            c>='a' && c<='z'?c-'a'+26:
            c>='0' && c<='9'?c-'0'+52:
            c=='+'?62:c=='/'?63:-1;
        if(digit<0)return false;
        buffer=(buffer<<6)|static_cast<uint32_t>(digit);
        bits+=6;
        if(bits>=8) {
            bits-=8;
            compressed.push_back(static_cast<unsigned char>((buffer>>bits)&255u));
            buffer &= (1u<<bits)-1u;
        }
    }
    pixels.assign(static_cast<size_t>(kWidth*kHeight),0);
    uLongf result=static_cast<uLongf>(pixels.size());
    return uncompress(pixels.data(),&result,compressed.data(),
                      static_cast<uLong>(compressed.size()))==Z_OK &&
           result==static_cast<uLongf>(pixels.size());
}
// The old geometry font advanced by 4*scale per glyph. A proportional
// 26px font needs actual glyph metrics to fit cards without clipping.
inline float Advance(const char* text,float scale) {
    if(!text||scale<=0.f)return 0.f;
    float units=0.f;
    for(const unsigned char *p=reinterpret_cast<const unsigned char*>(text);*p;++p) {
        const Glyph* g=Lookup(*p); if(g)units+=static_cast<float>(g->advance);
    }
    return units*scale*.22f;
}
inline float FittedScale(const char* text,float width,float requested) {
    if(width<=.12f||requested<=0.f)return 0.f;
    const float glyphWidth=Advance(text,1.f);
    return glyphWidth>0.f?std::min(requested,(width-.12f)/glyphWidth):0.f;
}
inline void AppendText(std::vector<float>& vertices,const char* text,
                       float x,float y,float scale,float r,float g,float b,float z) {
    if(!text||scale<=0.f)return;
    const float size=.22f*scale;
    float pen=x-Advance(text,scale)*.5f;
    const float baseline=y-2.2f*scale;
    auto add=[&vertices,r,g,b,z](float px,float py,float u,float v) {
        vertices.insert(vertices.end(),{px,py,z,u,v,r,g,b});
    };
    for(const unsigned char *p=reinterpret_cast<const unsigned char*>(text);*p;++p) {
        const Glyph* a=Lookup(*p); if(!a)continue;
        if(*p!=' ') {
            const float left=pen+a->bx*size;
            const float right=left+a->w*size;
            const float top=baseline-a->by*size;
            const float bottom=top-a->h*size;
            const float u0=float(a->x)/float(kWidth),u1=float(a->x+a->w)/float(kWidth);
            const float v0=float(a->y)/float(kHeight),v1=float(a->y+a->h)/float(kHeight);
            // GL uploads the top-down atlas row zero as texture row zero.
            add(left,bottom,u0,v1);add(right,bottom,u1,v1);add(right,top,u1,v0);
            add(left,bottom,u0,v1);add(right,top,u1,v0);add(left,top,u0,v0);
        }
        pen+=a->advance*size;
    }
}
}}}
#endif
