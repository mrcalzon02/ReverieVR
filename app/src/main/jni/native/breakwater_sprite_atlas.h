#ifndef REVERIE_BREAKWATER_SPRITE_ATLAS_H
#define REVERIE_BREAKWATER_SPRITE_ATLAS_H

#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace reverie { namespace breakwater {
// Game-private original RGBA sprites. One tiny atlas, no imported/licensed art,
// runtime image decoder, network lookup or per-eye sprite animation state.
enum class SpriteId : uint8_t {
    InfantryStrideA, InfantryStrideB, Rowboat, LandingCraft,
    Gunboat, Strafer, Bomber, Count
};
constexpr int kSpriteTile = 32;
constexpr int kSpriteAtlasWidth = 128;
constexpr int kSpriteAtlasHeight = 64;
using SpritePixels = std::array<uint8_t,
    kSpriteAtlasWidth * kSpriteAtlasHeight * 4>;

struct SpriteColor { uint8_t r,g,b; };
namespace sprite_detail {
inline void Rect(SpritePixels &pixels, SpriteId id, int x0, int y0,
                 int x1, int y1, SpriteColor c) {
    const int col = static_cast<int>(id) % 4;
    const int row = static_cast<int>(id) / 4;
    for (int y = (y0<0?0:y0); y < y1 && y < kSpriteTile; ++y)
        for (int x = (x0<0?0:x0); x < x1 && x < kSpriteTile; ++x) {
            const size_t offset = static_cast<size_t>(
                ((row*kSpriteTile+y)*kSpriteAtlasWidth + col*kSpriteTile+x)*4);
            pixels[offset]=c.r; pixels[offset+1]=c.g;
            pixels[offset+2]=c.b; pixels[offset+3]=255;
        }
}
inline void Triangle(SpritePixels &pixels, SpriteId id,
                     int ax,int ay,int bx,int by,int cx,int cy,
                     SpriteColor color) {
    const int minX = (ax<bx?(ax<cx?ax:cx):(bx<cx?bx:cx));
    const int maxX = (ax>bx?(ax>cx?ax:cx):(bx>cx?bx:cx));
    const int minY = (ay<by?(ay<cy?ay:cy):(by<cy?by:cy));
    const int maxY = (ay>by?(ay>cy?ay:cy):(by>cy?by:cy));
    const auto edge=[](int x0,int y0,int x1,int y1,int x,int y) {
        return (x-x0)*(y1-y0) - (y-y0)*(x1-x0);
    };
    const int orientation=edge(ax,ay,bx,by,cx,cy);
    if (orientation==0) return;
    for (int y=(minY<0?0:minY); y<=maxY && y<32; ++y)
        for (int x=(minX<0?0:minX); x<=maxX && x<32; ++x) {
            const int a=edge(ax,ay,bx,by,x,y);
            const int b=edge(bx,by,cx,cy,x,y);
            const int c=edge(cx,cy,ax,ay,x,y);
            if ((orientation>0 && a>=0 && b>=0 && c>=0) ||
                (orientation<0 && a<=0 && b<=0 && c<=0))
                Rect(pixels,id,x,y,x+1,y+1,color);
        }
}
} // namespace sprite_detail

inline SpritePixels BuildSpriteAtlas() {
    SpritePixels pixels{};
    using sprite_detail::Rect;
    using sprite_detail::Triangle;
    const SpriteColor olive{84,95,72}, dark{43,52,49};
    const SpriteColor steel{87,99,106}, hull{63,78,83};
    const SpriteColor sand{148,139,105}, metal{173,171,150};
    for (SpriteId id : {SpriteId::InfantryStrideA,SpriteId::InfantryStrideB}) {
        const bool a=id==SpriteId::InfantryStrideA;
        // Generic early-2000s Western infantry: helmet, vest, rifle and boots.
        Rect(pixels,id,12,23,21,27,olive);
        Rect(pixels,id,13,27,20,29,dark);
        Rect(pixels,id,11,14,22,23,olive);
        Rect(pixels,id,13,15,20,21,dark);
        Rect(pixels,id,11,17,14,22,sand);
        Rect(pixels,id,8,15,12,20,olive);
        Rect(pixels,id,21,15,25,20,olive);
        Rect(pixels,id,19,18,30,20,dark);
        Rect(pixels,id,24,17,27,18,metal);
        Rect(pixels,id,a?11:12,6,a?15:16,15,olive);
        Rect(pixels,id,a?18:17,a?7:6,a?22:21,15,olive);
        Rect(pixels,id,a?9:11,5,a?15:17,7,dark);
        Rect(pixels,id,a?18:16,5,a?24:22,7,dark);
    }
    Triangle(pixels,SpriteId::Rowboat,2,10,30,10,25,5,hull);
    Triangle(pixels,SpriteId::Rowboat,2,10,25,5,7,5,hull);
    Rect(pixels,SpriteId::Rowboat,6,10,26,13,steel);
    Rect(pixels,SpriteId::Rowboat,13,13,20,16,dark);
    Rect(pixels,SpriteId::Rowboat,14,16,19,21,olive);
    Rect(pixels,SpriteId::Rowboat,15,21,19,24,sand);
    Rect(pixels,SpriteId::LandingCraft,3,8,29,17,hull);
    Rect(pixels,SpriteId::LandingCraft,4,15,28,18,steel);
    Rect(pixels,SpriteId::LandingCraft,8,17,24,22,dark);
    for (int x : {10,17,23}) {
        Rect(pixels,SpriteId::LandingCraft,x,20,x+3,24,olive);
        Rect(pixels,SpriteId::LandingCraft,x,24,x+3,26,sand);
    }
    Rect(pixels,SpriteId::LandingCraft,11,5,22,15,metal);
    Rect(pixels,SpriteId::LandingCraft,12,6,21,10,steel);
    Triangle(pixels,SpriteId::Gunboat,1,8,31,8,25,4,hull);
    Triangle(pixels,SpriteId::Gunboat,1,8,25,4,8,4,hull);
    Rect(pixels,SpriteId::Gunboat,4,8,29,13,steel);
    Rect(pixels,SpriteId::Gunboat,10,13,22,22,hull);
    Rect(pixels,SpriteId::Gunboat,12,17,20,19,metal);
    Rect(pixels,SpriteId::Gunboat,23,14,26,16,dark);
    Rect(pixels,SpriteId::Gunboat,25,15,31,16,metal);
    const SpriteId strafer=SpriteId::Strafer;
    Triangle(pixels,strafer,15,11,17,11,17,30,steel);
    Triangle(pixels,strafer,15,11,17,30,14,26,steel);
    Triangle(pixels,strafer,15,21,3,16,15,17,olive);
    Triangle(pixels,strafer,17,21,29,16,17,17,olive);
    Triangle(pixels,strafer,15,12,8,8,15,9,steel);
    Triangle(pixels,strafer,17,12,24,8,17,9,steel);
    Rect(pixels,strafer,15,15,18,24,dark);
    const SpriteId bomber=SpriteId::Bomber;
    Rect(pixels,bomber,14,8,19,29,steel);
    Triangle(pixels,bomber,14,23,1,14,14,16,olive);
    Triangle(pixels,bomber,19,23,31,14,19,16,olive);
    Rect(pixels,bomber,5,16,9,22,hull);
    Rect(pixels,bomber,24,16,28,22,hull);
    Triangle(pixels,bomber,14,12,7,7,14,9,steel);
    Triangle(pixels,bomber,19,12,26,7,19,9,steel);
    Rect(pixels,bomber,15,20,18,26,dark);
    return pixels;
}

inline bool AppendSprite(std::vector<float> &mesh, SpriteId id,
                         float x,float y,float z,float width,float height,
                         float bank=0.0f) {
    if (id>=SpriteId::Count || !std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(z) || !std::isfinite(width) || !std::isfinite(height) ||
        !std::isfinite(bank) || width<=0 || height<=0) return false;
    const int index=static_cast<int>(id);
    const int col=index%4,row=index/4;
    const float u0=(col*32.0f+.5f)/kSpriteAtlasWidth;
    const float u1=((col+1)*32.0f-.5f)/kSpriteAtlasWidth;
    const float v0=(row*32.0f+.5f)/kSpriteAtlasHeight;
    const float v1=((row+1)*32.0f-.5f)/kSpriteAtlasHeight;
    const float cosine=std::cos(bank),sine=std::sin(bank);
    const auto vertex=[&](float localX,float localY,float u,float v) {
        mesh.push_back(x+cosine*localX-sine*localY);
        mesh.push_back(y+sine*localX+cosine*localY);
        mesh.push_back(z);
        mesh.push_back(u); mesh.push_back(v);
    };
    const float left=-width*.5f,right=width*.5f;
    const float bottom=-height*.5f,top=height*.5f;
    vertex(left,bottom,u0,v0); vertex(right,bottom,u1,v0);
    vertex(right,top,u1,v1); vertex(left,bottom,u0,v0);
    vertex(right,top,u1,v1); vertex(left,top,u0,v1);
    return true;
}
}} // namespace reverie::breakwater
#endif
