#ifndef REVERIE_BREAKWATER_MATERIALS_H
#define REVERIE_BREAKWATER_MATERIALS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>

namespace reverie { namespace breakwater { namespace materials {

// Original low-resolution coastal materials, appended below the existing
// 32x32 combat sprites in their single GLES2 RGBA atlas. The fourth tile is
// reserved for battered battery steel; no external art/license dependency.
enum class Surface : uint8_t { Shingle, Quay, Basalt, Steel, Sandbag, OlivePaint };
constexpr int kTile=64;
constexpr int kAtlasWidth=128;
constexpr int kBaseY=256;
constexpr int kAtlasHeight=kBaseY+3*kTile;
constexpr int kSectionsX=24,kSectionsZ=20;

struct Rgb { uint8_t r,g,b; };

inline uint32_t Hash(int x,int y,uint32_t salt) {
    uint32_t h=salt ^ (static_cast<uint32_t>(x)*0x9e3779b9u)
                    ^ (static_cast<uint32_t>(y)*0x85ebca6bu);
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;
    return h^(h>>16);
}
inline uint8_t Channel(int v) {
    return static_cast<uint8_t>(std::clamp(v,0,255));
}
inline Rgb Sample(Surface surface,int x,int y) {
    // The tile generator never needs random state or a frame timer.
    const uint32_t seed=0xb4ea9b21u+static_cast<uint32_t>(surface)*0x7198d3a5u;
    const uint32_t h=Hash(x,y,seed);
    const uint32_t region=Hash(x/7,y/6,seed+13u);
    const int grit=static_cast<int>(h%23u)-11;
    const int shade=static_cast<int>(region%27u)-13;
    switch(surface) {
        case Surface::Shingle: {
            const int cellX=x/5,cellY=y/4;
            const uint32_t pebble=Hash(cellX,cellY,seed+91u);
            const bool stone=pebble%5u<2u && (x%5>0) && (y%4>0);
            const int c=shade/2+grit+(stone?-33:0);
            return {Channel(185+c),Channel(166+c),Channel(127+c)};
        }
        case Surface::Quay: {
            const bool seam=(y%16==0) || ((x+(y/16%2)*9)%31==0);
            const bool chip=(region%11u==0u) && (h%3u==0u);
            const int c=shade+grit/2-(seam?39:0)-(chip?31:0);
            return {Channel(139+c),Channel(145+c),Channel(143+c)};
        }
        case Surface::Basalt: {
            const bool fissure=(x+(y/9)*5)%23==0 || y%21==0;
            const bool mineral=h%17u==0u;
            const int c=shade+grit/2-(fissure?23:0)+(mineral?19:0);
            return {Channel(76+c),Channel(77+c),Channel(87+c)};
        }
        case Surface::Sandbag: {
            // Woven jute is broadly tan with dark crossing fibers and seam
            // wear: not just recolored concrete or a stone masonry pattern.
            const bool warp=(x%7==0),weft=(y%5==0);
            const int c=grit/2+shade/2-(warp?22:0)-(weft?15:0);
            return {Channel(156+c),Channel(138+c),Channel(101+c)};
        }
        case Surface::OlivePaint: {
            // Faded green military enamel with scratching and oxide flecks.
            const bool scratch=((x+y/5)%29==0),chip=h%29u==0u;
            const int c=shade/2+grit/3-(scratch?23:0);
            return {Channel((chip?127:76)+c),Channel((chip?97:92)+c),
                    Channel((chip?63:70)+c)};
        }
        case Surface::Steel: {
            const bool seam=y%22==0;
            const bool rivet=x%19<=2 && y%22<=2;
            const bool rust=(region%9u<3u) && (h%7u<4u);
            const int c=shade/2+grit/3-(seam?25:0)-(rivet?52:0);
            return {Channel((rust?145:92)+c),Channel((rust?81:107)+c),
                    Channel((rust?53:111)+c)};
        }
    }
    return {255,0,255};
}

template<class WritePixel>
inline void Rasterize(WritePixel write) {
    for(int t=0;t<6;++t)for(int y=0;y<kTile;++y)
        for(int x=0;x<kTile;++x) {
            const Rgb c=Sample(static_cast<Surface>(t),x,y);
            write((t%2)*kTile+x,kBaseY+(t/2)*kTile+y,c);
        }
}

// Use the existing five-float textured sprite vertex format and existing
// shader/bind/draw. UVs inset half a texel avoid atlas neighbor bleeding.
inline void Vertex(std::vector<float>&mesh,float x,float y,float z,
                   float u,float v) {
    mesh.push_back(x);mesh.push_back(y);mesh.push_back(z);
    mesh.push_back(u);mesh.push_back(v);
}
// Opaque cladding applied to the actual, preexisting bunker boxes. The
// original XYZRGB box remains underneath as a fallback, while outward
// surfaces sit 3 mm beyond it to avoid Z-fighting on GLES2. This creates
// no new collider, target or pass: AppendBoxCladding goes into the existing
// sprite VBO. No texture state or shader is changed by these functions.
struct Point { float x,y,z; };
inline void Face(std::vector<float>&mesh,Surface surface,
                 Point a,Point b,Point c,Point d) {
    const int tile=static_cast<int>(surface);
    const float u0=((tile%2)*kTile+.5f)/kAtlasWidth;
    const float u1=((tile%2+1)*kTile-.5f)/kAtlasWidth;
    const float v0=(kBaseY+(tile/2)*kTile+.5f)/kAtlasHeight;
    const float v1=(kBaseY+(tile/2+1)*kTile-.5f)/kAtlasHeight;
    Vertex(mesh,a.x,a.y,a.z,u0,v0);
    Vertex(mesh,b.x,b.y,b.z,u1,v0);
    Vertex(mesh,c.x,c.y,c.z,u1,v1);
    Vertex(mesh,a.x,a.y,a.z,u0,v0);
    Vertex(mesh,c.x,c.y,c.z,u1,v1);
    Vertex(mesh,d.x,d.y,d.z,u0,v1);
}
inline bool AppendBoxCladding(std::vector<float>&mesh,Surface material,
                              float x,float y,float z,float w,float h,float d) {
    if(material>Surface::OlivePaint || !std::isfinite(x) || !std::isfinite(y) ||
       !std::isfinite(z) || !std::isfinite(w) || !std::isfinite(h) ||
       !std::isfinite(d) || w<=0.f || h<=0.f || d<=0.f)return false;
    constexpr float pad=.003f;
    const float xl=x-w*.5f-pad,xr=x+w*.5f+pad;
    const float yl=y-h*.5f-pad,yr=y+h*.5f+pad;
    const float zn=z-d*.5f-pad,zf=z+d*.5f+pad;
    Face(mesh,material,{xl,yl,zn},{xr,yl,zn},{xr,yr,zn},{xl,yr,zn});
    Face(mesh,material,{xr,yl,zf},{xl,yl,zf},{xl,yr,zf},{xr,yr,zf});
    Face(mesh,material,{xl,yr,zn},{xr,yr,zn},{xr,yr,zf},{xl,yr,zf});
    Face(mesh,material,{xl,yl,zf},{xr,yl,zf},{xr,yl,zn},{xl,yl,zn});
    Face(mesh,material,{xl,yl,zf},{xl,yl,zn},{xl,yr,zn},{xl,yr,zf});
    Face(mesh,material,{xr,yl,zn},{xr,yl,zf},{xr,yr,zf},{xr,yr,zn});
    return true;
}
inline Surface CoastSurface(uint8_t location) {
    return location==1?Surface::Quay:
           location==2?Surface::Basalt:Surface::Shingle;
}

template<class HeightAt>
inline void AppendCoast(std::vector<float>&mesh,uint8_t location,HeightAt height) {
    if(location>=3)return;
    const int tile=static_cast<int>(CoastSurface(location));
    const float u0=((tile%2)*kTile+.5f)/kAtlasWidth;
    const float u1=((tile%2+1)*kTile-.5f)/kAtlasWidth;
    const float v0=(kBaseY+(tile/2)*kTile+.5f)/kAtlasHeight;
    const float v1=(kBaseY+(tile/2+1)*kTile-.5f)/kAtlasHeight;
    constexpr float minX=-3.86f,maxX=3.86f;
    constexpr float surfZ=-2.20f,rearZ=-.26f;
    for(int iz=0;iz<kSectionsZ;++iz) {
        const float z0=surfZ+(rearZ-surfZ)*iz/kSectionsZ;
        const float z1=surfZ+(rearZ-surfZ)*(iz+1)/kSectionsZ;
        for(int ix=0;ix<kSectionsX;++ix) {
            const float x0=minX+(maxX-minX)*ix/kSectionsX;
            const float x1=minX+(maxX-minX)*(ix+1)/kSectionsX;
            const float y00=height(location,x0,z0);
            const float y10=height(location,x1,z0);
            const float y01=height(location,x0,z1);
            const float y11=height(location,x1,z1);
            Vertex(mesh,x0,y00,z0,u0,v0);
            Vertex(mesh,x1,y10,z0,u1,v0);
            Vertex(mesh,x1,y11,z1,u1,v1);
            Vertex(mesh,x0,y00,z0,u0,v0);
            Vertex(mesh,x1,y11,z1,u1,v1);
            Vertex(mesh,x0,y01,z1,u0,v1);
        }
    }
}

}}} // namespace reverie::breakwater::materials
#endif
