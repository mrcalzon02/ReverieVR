#ifndef REVERIE_LANTERN_TILE_CAPTURE_H
#define REVERIE_LANTERN_TILE_CAPTURE_H
// Real terrain heightfields and physical object placement are sampled
// into one near-nadir airborne view. Terrain comes only from the baked
// LTIL catalog; units come from a separate timestamped war snapshot.
// Render is a capture-time step, never a per-stereo-eye computation.
#include "lantern_tile_library.h"
#include "lantern_photo_capture_quality.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace reverie { namespace lantern { namespace tiles {
constexpr int kPhotoDimension=1024;
constexpr float kPhotoWorldSpan=256.f;
constexpr float kPixelsPerWorldUnit=kPhotoDimension/kPhotoWorldSpan;
constexpr float kWorldUnitsPerPixel=kPhotoWorldSpan/kPhotoDimension;
constexpr float kPi=3.14159265358979323846f;
struct Celestial {
    int day_of_year=180;            // 1..366, from war calendar
    float latitude_degrees=51.f;
    float local_hour=12.f;          // [0,24), capture local solar time
    float moon_phase=0.5f;         // 0=new, .5=full
    float moon_hour_angle=0.f;     // radians from local meridian
    float cloud_attenuation=0.f;   // opacity of physical weather at sun
    float star_irradiance=.012f;    // low minimum light, not daylight
    bool buildings_lit=false;
};
struct Illumination {
    float sun_altitude=0.f,sun_dx=0.f,sun_dy=0.f;
    float moon_altitude=0.f;
    float direct_solar=0.f,moonlight=0.f,ambient=.01f;
    float sun_x=1.f,sun_y=0.f;
};
inline bool Valid(const Celestial& c){
    return c.day_of_year>=1&&c.day_of_year<=366&&
           std::isfinite(c.latitude_degrees)&&std::abs(c.latitude_degrees)<=85.f&&
           std::isfinite(c.local_hour)&&c.local_hour>=0.f&&c.local_hour<24.f&&
           std::isfinite(c.moon_phase)&&c.moon_phase>=0.f&&c.moon_phase<=1.f&&
           std::isfinite(c.moon_hour_angle)&&std::abs(c.moon_hour_angle)<=kPi&&
           std::isfinite(c.cloud_attenuation)&&c.cloud_attenuation>=0.f&&
           c.cloud_attenuation<=1.f&&std::isfinite(c.star_irradiance)&&
           c.star_irradiance>=0.f&&c.star_irradiance<=.1f;
}
inline Illumination Illuminate(const Celestial& c){
    Illumination v{};
    if(!Valid(c))return v;
    const float latitude=c.latitude_degrees*kPi/180.f;
    const float dec=23.44f*kPi/180.f*
        std::sin(2.f*kPi*(c.day_of_year-81)/365.25f);
    const float hour=(c.local_hour-12.f)*kPi/12.f;
    const float sine=std::sin(latitude)*std::sin(dec)+
                     std::cos(latitude)*std::cos(dec)*std::cos(hour);
    v.sun_altitude=std::asin(std::max(-1.f,std::min(1.f,sine)));
    const float dx=-std::cos(dec)*std::sin(hour);
    const float dy=std::sin(dec)*std::cos(latitude)-
                   std::cos(dec)*std::sin(latitude)*std::cos(hour);
    const float length=std::max(.0001f,std::hypot(dx,dy));
    v.sun_x=dx/length;v.sun_y=dy/length;
    const float sun=std::max(0.f,sine);
    v.direct_solar=sun*(1.f-.82f*c.cloud_attenuation);
    // Lunar altitude and phase are independent of civil day/night; the
    // caller supplies orbital phase/meridian from persistent war time.
    const float lunarDec=5.14f*kPi/180.f*
        std::sin(c.moon_phase*2.f*kPi);
    const float lunarSine=std::sin(latitude)*std::sin(lunarDec)+
         std::cos(latitude)*std::cos(lunarDec)*
         std::cos(c.moon_hour_angle);
    v.moon_altitude=std::asin(std::max(-1.f,std::min(1.f,lunarSine)));
    const float illuminated=(1.f-std::cos(c.moon_phase*2.f*kPi))*.5f;
    v.moonlight=(1.f-.80f*c.cloud_attenuation)*
        illuminated*std::max(0.f,lunarSine)*.18f;
    v.ambient=c.star_irradiance+
        .12f*v.direct_solar+.26f*v.moonlight;
    return v;
}
struct CaptureLayers {
    capture::GrayPlate photo;
    std::vector<uint8_t> building_mask,foliage_mask,unit_mask,shadow_mask;
    bool Valid()const{
        const size_t n=kPhotoDimension*kPhotoDimension;
        return photo.Valid()&&photo.width==kPhotoDimension&&
               photo.height==kPhotoDimension&&
               building_mask.size()==n&&foliage_mask.size()==n&&
               unit_mask.size()==n&&shadow_mask.size()==n;
    }
};
inline uint8_t Byte(float value){
    return static_cast<uint8_t>(std::lround(
        std::max(0.f,std::min(255.f,value))));
}
inline void ShadePixel(CaptureLayers& image,std::vector<float>& zbuf,
                       int x,int y,float height,float grayscale,
                       uint8_t building,uint8_t canopy,uint8_t unit){
    if(x<0||x>=kPhotoDimension||y<0||y>=kPhotoDimension)return;
    const size_t k=static_cast<size_t>(y)*kPhotoDimension+x;
    if(height<zbuf[k])return;
    zbuf[k]=height;image.photo.pixels[k]=Byte(grayscale);
    if(building)image.building_mask[k]=building;
    if(canopy)image.foliage_mask[k]=canopy;
    if(unit)image.unit_mask[k]=unit;
}
inline uint32_t NoiseSeed(uint32_t seed,uint32_t x,uint32_t y){
    return Hash(seed^uint32_t(x*0x27d4eb2du)^uint32_t(y*0x9e3779b9u));
}
inline bool MakePhoto(const Tile& tile,const std::vector<PlacedUnit>& units,
                      const Celestial& time,CaptureLayers* out,
                      bool bridge_broken=false){
    if(!out||!Valid(time)||tile.id>=kLibrarySize)return false;
    CaptureLayers image{};
    image.photo.width=image.photo.height=kPhotoDimension;
    const size_t pixelCount=kPhotoDimension*kPhotoDimension;
    image.photo.pixels.resize(pixelCount);
    image.building_mask.resize(pixelCount,0);
    image.foliage_mask.resize(pixelCount,0);
    image.unit_mask.resize(pixelCount,0);
    image.shadow_mask.resize(pixelCount,0);
    std::vector<float> zbuf(pixelCount,0.f);
    const auto sun=Illuminate(time);
    const float sx=sun.sun_x,sy=sun.sun_y;
    const float sunTan=std::tan(std::max(.07f,sun.sun_altitude));
    const float sky=std::min(1.5f,
        sun.ambient+sun.direct_solar+sun.moonlight);
    for(int py=0;py<kPhotoDimension;++py)for(int px=0;px<kPhotoDimension;++px){
        const size_t i=static_cast<size_t>(py)*kPhotoDimension+px;
        const float x=(px+.5f)*kWorldUnitsPerPixel;
        const float y=(py+.5f)*kWorldUnitsPerPixel;
        const float h=tile.GroundHeight(x,y);
        zbuf[i]=h;
        const float hx=tile.GroundHeight(std::min(x+2.f,255.f),y)-
                       tile.GroundHeight(std::max(0.f,x-2.f),y);
        const float hy=tile.GroundHeight(x,std::min(y+2.f,255.f))-
                       tile.GroundHeight(x,std::max(0.f,y-2.f));
        const float nz=1.f/std::sqrt(1.f+hx*hx*.05f+hy*hy*.05f);
        float directional=std::max(0.f,nz*(.9f-
            (hx*sx+hy*sy)*.25f));
        // Short deterministic terrain-horizon test: hilltops occlude
        // sunlight from the real angle, not prepainted screen shadows.
        bool terrainOccluded=false;
        if(sun.direct_solar>.015f)
            for(int step=1;step<=9;++step){
                const float d=step*3.5f;
                const float xx=x+sx*d,yy=y+sy*d;
                if(xx<0.f||xx>255.f||yy<0.f||yy>255.f)break;
                if(tile.GroundHeight(xx,yy)>h+d*sunTan+.15f){
                    terrainOccluded=true;break;
                }
            }
        if(terrainOccluded){
            directional*=.18f;
            image.shadow_mask[i]=1u;
        }
        const uint32_t sample=NoiseSeed(tile.seed,px,py);
        const float grain=(sample&255u)/255.f-.5f;
        float albedo=89.f+(tile.biome%4)*7.f+
             grain*35.f+6.f*std::sin(x*.36f+y*.12f);
        if(tile.flags&1u){
            const float riverX=128.f+27.f*std::sin(y/52.f+
                float(Hash(tile.seed^0x80851u)%3000u)/450.f*.4f);
            if(std::abs(x-riverX)<17.f)
                albedo=54.f+24.f*grain;
        }
        const float roadDistance=std::abs(y-tile.RoadY(x));
        if(roadDistance<5.5f)
            albedo=156.f+18.f*grain; // same physical road at every visit
        if(bridge_broken&&(tile.flags&1u)&&roadDistance<6.f){
            const float riverX=128.f+27.f*std::sin(y/52.f+
                float(Hash(tile.seed^0x80851u)%3000u)/450.f*.4f);
            const float gap=std::abs(x-riverX);
            if(gap<13.f){
                // A changed bridge deck is a WORLD EVENT. Terrain and the
                // road approach are immutable; only center-span debris,
                // missing decking and dark exposed river are different.
                albedo=gap<8.f?(48.f+18.f*grain):(84.f+77.f*grain);
            }
        }
        if((tile.flags&4u)&&std::abs(y-(89.f+6.f*std::sin(x*.07f)))<2.f)
            albedo=65.f+12.f*grain; // rail geometry is immutable
        float lamps=0.f;
        if(time.buildings_lit&&sun.direct_solar<.07f){
            for(const auto& b:tile.buildings){
                if(!b.exists)continue;
                const float d=std::hypot(x-b.x,y-b.y);
                lamps+=.29f*std::max(0.f,1.f-d/24.f);
            }
        }
        // Hemispherical fill plus terrain bounce, directional sun/moon,
        // and physically placed point-light pools. Not expensive full GI.
        const float light=sky*.18f+sun.direct_solar*directional+
            sun.moonlight*.6f+lamps;
        image.photo.pixels[i]=Byte(albedo*(.28f+light*1.4f));
    }
    const auto shadow=[&](float x,float y,float radiusX,float radiusY,
                          float objectHeight){
        if(sun.direct_solar<.025f)return;
        const float length=std::min(55.f,objectHeight/
            std::max(.12f,std::tan(sun.sun_altitude)));
        const float ox=-sx*length,oy=-sy*length;
        const int minX=std::max(0,int((x+ox-radiusX)*kPixelsPerWorldUnit));
        const int maxX=std::min(kPhotoDimension-1,int((x+ox+radiusX)*kPixelsPerWorldUnit));
        const int minY=std::max(0,int((y+oy-radiusY)*kPixelsPerWorldUnit));
        const int maxY=std::min(kPhotoDimension-1,int((y+oy+radiusY)*kPixelsPerWorldUnit));
        for(int py=minY;py<=maxY;++py)for(int px=minX;px<=maxX;++px){
            const float wx=(px+.5f)*kWorldUnitsPerPixel,wy=(py+.5f)*kWorldUnitsPerPixel;
            const float dx=(wx-x-ox)/std::max(.5f,radiusX);
            const float dy=(wy-y-oy)/std::max(.5f,radiusY);
            if(dx*dx+dy*dy>=1.f)continue;
            const size_t i=static_cast<size_t>(py)*kPhotoDimension+px;
            image.shadow_mask[i]=2u;
            image.photo.pixels[i]=Byte(image.photo.pixels[i]*.64f);
        }
    };
    // Static rooftops are real elevated planes with shade, silhouettes and
    // directional ground shadows, never painted into a temporary war layer.
    for(const auto& house:tile.buildings){
        if(!house.exists)continue;
        shadow(house.x,house.y,house.width,house.depth,house.height);
        for(int y=std::max(0,int((house.y-house.depth*.5f)*kPixelsPerWorldUnit));y<=
                std::min(kPhotoDimension-1,int((house.y+house.depth*.5f)*kPixelsPerWorldUnit));++y)
          for(int x=std::max(0,int((house.x-house.width*.5f)*kPixelsPerWorldUnit));x<=
                  std::min(kPhotoDimension-1,int((house.x+house.width*.5f)*kPixelsPerWorldUnit));++x){
            const float wx=(x+.5f)*kWorldUnitsPerPixel,wy=(y+.5f)*kWorldUnitsPerPixel;
            const float slope=(wx-house.x)/std::max(2.f,static_cast<float>(house.width));
            const float height=tile.GroundHeight(wx,wy)+house.height-
                               1.2f*std::abs(slope);
            const float roofLight=sun.direct_solar*
                     (slope*sx>.0f?.90f:.47f)+sun.ambient+sun.moonlight;
            const float paint=(house.kind==4?73.f:126.f)+
                 100.f*roofLight+(house.kind==2?20.f:0.f);
            ShadePixel(image,zbuf,x,y,height,paint,1,0,0);
        }
    }
    // Dynamic equipment is below elevated canopy and most building roofs.
    // Tents and latrines are derived from actual unit strength/activity,
    // and disappear when the war orders move to another tile.
    for(const auto& unit:units){
        const int instances=(unit.type==UnitType::CampTent?
            std::min(12u,1u+(unit.strength+1u)/2u):1u);
        for(int k=0;k<instances;++k){
            const float ox=(k%4)*7.f-9.f,oy=(k/4)*8.f-6.f;
            const float x=unit.x+(unit.type==UnitType::CampTent?ox:0.f);
            const float y=unit.y+(unit.type==UnitType::CampTent?oy:0.f);
            const float rx=(unit.type==UnitType::Tank?5.f:
                unit.type==UnitType::Truck?5.5f:unit.type==UnitType::CampTent?3.5f:2.f);
            const float ry=(unit.type==UnitType::Tank?2.9f:
                unit.type==UnitType::Truck?2.5f:unit.type==UnitType::CampTent?2.5f:1.5f);
            shadow(x,y,rx,ry,unit.type==UnitType::CampTent?2.6f:3.5f);
            for(int py=std::max(0,int((y-ry)*kPixelsPerWorldUnit));py<=std::min(kPhotoDimension-1,int((y+ry)*kPixelsPerWorldUnit));++py)
            for(int px=std::max(0,int((x-rx)*kPixelsPerWorldUnit));px<=std::min(kPhotoDimension-1,int((x+rx)*kPixelsPerWorldUnit));++px){
                const float wx=(px+.5f)*kWorldUnitsPerPixel,wy=(py+.5f)*kWorldUnitsPerPixel;
                const float dx=(wx-x)/rx,dy=(wy-y)/ry;
                const float ax=std::abs(dx),ay=std::abs(dy);
                float material=93.f,heightAboveGround=2.f;
                // Readable near-nadir forms from real world objects, not
                // colored truth symbols. Every feature shares world scale.
                if(unit.type==UnitType::Tank){
                    const bool tracks=ax<.88f&&ay>.55f&&ay<.95f;
                    const bool hull=ax<.72f&&ay<.62f;
                    const bool turret=(dx+.12f)*(dx+.12f)*2.8f+dy*dy*4.f<1.f;
                    const bool barrel=dx>.15f&&dx<.98f&&ay<.12f;
                    if(!(tracks||hull||turret||barrel))continue;
                    material=tracks?47.f:hull?82.f:65.f;
                    if(tracks&&std::fmod((dx+1.f)*13.f,1.f)<.22f)material+=18.f;
                    if(turret){material=117.f;heightAboveGround=2.8f;}
                    if(barrel){material=76.f;heightAboveGround=2.6f;}
                }else if(unit.type==UnitType::Truck){
                    const bool cab=dx>.36f&&dx<.92f&&ay<.71f;
                    const bool bed=dx<.30f&&dx>-.90f&&ay<.80f;
                    const bool wheels=ay>.66f&&ay<.96f&&
                        (std::abs(dx+.58f)<.21f||std::abs(dx-.61f)<.18f);
                    if(!(cab||bed||wheels))continue;
                    material=wheels?42.f:bed?103.f:143.f;
                    if(bed&&ay>.66f)material=76.f;
                    if(cab&&dx>.70f)material=83.f;
                }else if(unit.type==UnitType::CampTent){
                    if(ax>.95f||ay>.90f||ax+ay>1.71f)continue;
                    heightAboveGround=2.4f*(1.f-.62f*ay);
                    material=dy*sx+dy*sy>0.f?154.f:116.f;
                    if(ay<.085f)material=185.f;
                }else if(unit.type==UnitType::Supply){
                    if(ax>.87f||ay>.86f)continue;
                    material=(ax>.68f||ay>.67f)?72.f:132.f;
                }else{
                    if(dx*dx+dy*dy>1.f)continue;
                    material=93.f;
                }
                const float height=tile.GroundHeight(wx,wy)+heightAboveGround;
                ShadePixel(image,zbuf,px,py,height,
                           material*(.42f+sky*.75f),0,0,1);
            }
        }
    }
    // Raised canopy impostors live in the terrain's X/Y/Z coordinate
    // system. Their cutout alpha and z-buffer *actually conceal* vehicles,
    // tents and soldiers below, including partial recognisable edges.
    for(const auto& tree:tile.foliage){
        if(!tree.kind)continue;
        shadow(tree.x,tree.y,tree.radius*1.1f,tree.radius*.8f,
               float(tree.height));
        for(int py=std::max(0,int((tree.y-tree.radius)*kPixelsPerWorldUnit));py<=
                std::min(kPhotoDimension-1,int((tree.y+tree.radius)*kPixelsPerWorldUnit));++py)
        for(int px=std::max(0,int((tree.x-tree.radius)*kPixelsPerWorldUnit));px<=
                std::min(kPhotoDimension-1,int((tree.x+tree.radius)*kPixelsPerWorldUnit));++px){
            const float wx=(px+.5f)*kWorldUnitsPerPixel,wy=(py+.5f)*kWorldUnitsPerPixel;
            const float nx=(wx-tree.x)/tree.radius,
                        ny=(wy-tree.y)/tree.radius;
            const float d=nx*nx+ny*ny;
            if(d>=1.f)continue;
            const uint32_t grain=NoiseSeed(tile.seed+tree.kind,px,py);
            const float holes=(grain&1023u)/1023.f;
            // No paper-flat solid circles: use sparse cutouts and ragged
            // branching with a lower alpha density at the canopy fringe.
            if(holes>(d>.65f?.55f:.86f))continue;
            const float h=tile.GroundHeight(wx,wy)+tree.height*
                        (.87f+.13f*(1.f-d));
            const float albedo=56.f+(tree.kind%3)*13.f+
                35.f*((grain>>12u)&255u)/255.f;
            ShadePixel(image,zbuf,px,py,h,albedo*(.4f+sky),0,1,0);
        }
    }
    *out=std::move(image);
    return true;
}
}}} // reverie::lantern::tiles
#endif
