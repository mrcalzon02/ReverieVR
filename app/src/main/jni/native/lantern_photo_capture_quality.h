#ifndef REVERIE_LANTERN_PHOTO_CAPTURE_QUALITY_H
#define REVERIE_LANTERN_PHOTO_CAPTURE_QUALITY_H
// Game-private reference capture pipeline. A stable registered photographic
// plate is an INPUT, not painted by this algorithm. No World Truth, target
// identity, assessment or report result is exposed to its output.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace reverie { namespace lantern { namespace capture {

enum class Cloud : uint8_t {
    Clear, ScatteredCumulus, BrokenCumulus, Cirrus, Stratus, Overcast, Squall
};
enum class Precipitation : uint8_t {
    None, Rain, Storm, HeavyStorm, Snow, Hail
};
struct Conditions {
    Cloud cloud=Cloud::Clear;
    Precipitation precipitation=Precipitation::None;
    uint32_t weather_seed=0x5a13e91bu;
    float wind_east_mps=0.f,wind_north_mps=0.f;
    float exposure_seconds=0.f;
    // World registration of the AUTHORED, fixed near-nadir source plate.
    // Adjacent frames use the same origin/scale; wind is world-relative.
    float west_m=0.f,north_m=0.f,meters_per_pixel=1.f;
    float ambient_haze=0.f; // measured/seeded mission conditions, [0,1]
};
struct Camera {
    uint32_t grime_seed=0xbaca5217u; // aircraft/lens identity, NOT weather seed
    float grime=0.f;                 // retained until cleaning, [0,1]
    float sensor_noise=0.f;          // capture-time loss, [0,1]
};
struct GrayPlate {
    int width=0,height=0;
    std::vector<uint8_t> pixels;
    bool Valid()const{
        return width>0&&height>0&&width<=4096&&height<=4096&&
            pixels.size()==static_cast<size_t>(width)*static_cast<size_t>(height);
    }
};
// Masks are separate, small alpha channels; not copies of scenery.
// Every 4096x4096 photograph gets at most five 256x256 mask channels.
struct MaskLayers {
    int width=0,height=0;
    std::vector<uint8_t> cloud,cloud_shadow,precipitation,haze,lens_grime;
};
struct CapturedMaster {
    GrayPlate recorded;             // physically obscured master (once)
    MaskLayers masks;               // capture provenance, no military labels
    Conditions conditions{};
    Camera camera{};
};
inline float Bound(float v){return std::max(0.f,std::min(1.f,v));}
inline uint8_t Quantize(float v){
    return static_cast<uint8_t>(std::lround(Bound(v)*255.f));
}
inline uint32_t Mix(uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
    return x;
}
inline float Random(int x,int y,uint32_t seed){
    const uint32_t h=Mix(static_cast<uint32_t>(x)*0x9e3779b9u ^
                         static_cast<uint32_t>(y)*0x85ebca6bu ^seed);
    return static_cast<float>(h&0x00ffffffu)/16777215.f;
}
inline float Fade(float x){return x*x*(3.f-2.f*x);}
inline float Noise(float x,float y,uint32_t seed){
    const int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));
    const float fx=Fade(x-std::floor(x)),fy=Fade(y-std::floor(y));
    const float a=Random(ix,iy,seed),b=Random(ix+1,iy,seed);
    const float c=Random(ix,iy+1,seed),d=Random(ix+1,iy+1,seed);
    return (a+(b-a)*fx)*(1.f-fy)+(c+(d-c)*fx)*fy;
}
inline float Fractal(float x,float y,uint32_t seed){
    return .57f*Noise(x,y,seed)+.29f*Noise(x*2.03f,y*2.03f,seed+17u)+
           .14f*Noise(x*4.11f,y*4.11f,seed+37u);
}
inline bool Valid(const Conditions& w,const Camera& c){
    return static_cast<unsigned>(w.cloud)<=static_cast<unsigned>(Cloud::Squall)&&
           static_cast<unsigned>(w.precipitation)<=static_cast<unsigned>(Precipitation::Hail)&&
           std::isfinite(w.wind_east_mps)&&std::isfinite(w.wind_north_mps)&&
           std::abs(w.wind_east_mps)<=100.f&&std::abs(w.wind_north_mps)<=100.f&&
           std::isfinite(w.exposure_seconds)&&w.exposure_seconds>=0.f&&
           w.exposure_seconds<=31536000.f&&std::isfinite(w.west_m)&&
           std::isfinite(w.north_m)&&std::abs(w.west_m)<=10000000.f&&
           std::abs(w.north_m)<=10000000.f&&
           std::isfinite(w.meters_per_pixel)&&w.meters_per_pixel>.01f&&
           w.meters_per_pixel<=1000.f&&std::isfinite(w.ambient_haze)&&
           w.ambient_haze>=0.f&&w.ambient_haze<=1.f&&
           std::isfinite(c.grime)&&c.grime>=0.f&&c.grime<=1.f&&
           std::isfinite(c.sensor_noise)&&c.sensor_noise>=0.f&&c.sensor_noise<=1.f;
}
inline float CloudOpacity(Cloud kind,float x,float y,float across,
                          uint32_t seed){
    if(kind==Cloud::Clear)return 0.f;
    const float broad=Fractal(x*.63f,y*.63f,seed);
    const float detail=Fractal(x*1.67f,y*1.67f,seed+71u);
    switch(kind){
    case Cloud::ScatteredCumulus: return Bound((broad*.72f+detail*.28f-.58f)*3.5f);
    case Cloud::BrokenCumulus: return Bound((broad*.72f+detail*.28f-.43f)*3.1f);
    case Cloud::Cirrus: return Bound((.30f+.25f*std::sin(across*11.f+detail*3.f)-.26f)*1.35f);
    case Cloud::Stratus: return Bound(.29f+.46f*broad+.12f*std::sin(across*2.1f));
    case Cloud::Overcast: return Bound(.59f+.32f*broad);
    case Cloud::Squall: return Bound(.24f+.45f*broad+
                                      .30f*std::sin(across*6.4f+detail*1.5f));
    case Cloud::Clear: break;
    }
    return 0.f;
}
inline float PrecipStrength(Precipitation kind){
    switch(kind){
    case Precipitation::None:return 0.f;
    case Precipitation::Rain:return .30f;
    case Precipitation::Storm:return .56f;
    case Precipitation::HeavyStorm:return .85f;
    case Precipitation::Snow:return .49f;
    case Precipitation::Hail:return .68f;
    }
    return 0.f;
}
inline bool BuildMasks(int image_width,int image_height,
                       const Conditions& weather,const Camera& camera,
                       MaskLayers* out){
    if(!out||image_width<=0||image_height<=0||image_width>4096||
       image_height>4096||!Valid(weather,camera))return false;
    MaskLayers next;
    next.width=std::min(image_width,256);
    next.height=std::min(image_height,256);
    const size_t count=static_cast<size_t>(next.width)*next.height;
    next.cloud.resize(count);next.cloud_shadow.resize(count);
    next.precipitation.resize(count);next.haze.resize(count);
    next.lens_grime.resize(count);
    const float length=std::hypot(weather.wind_east_mps,weather.wind_north_mps);
    const float dirx=length>.001f?weather.wind_east_mps/length:1.f;
    const float diry=length>.001f?weather.wind_north_mps/length:0.f;
    const float intensity=PrecipStrength(weather.precipitation);
    for(int y=0;y<next.height;++y)for(int x=0;x<next.width;++x){
        const size_t idx=static_cast<size_t>(y)*next.width+x;
        const float u=(x+.5f)/next.width,v=(y+.5f)/next.height;
        // Seeded atmosphere is sampled in physical world coordinates.
        // The same storm drifts by actual wind; camera dirt does not.
        const float wx=weather.west_m+u*image_width*weather.meters_per_pixel-
                       weather.wind_east_mps*weather.exposure_seconds;
        const float wy=weather.north_m+v*image_height*weather.meters_per_pixel-
                       weather.wind_north_mps*weather.exposure_seconds;
        const float along=(wx*dirx+wy*diry)/360.f;
        const float across=(-wx*diry+wy*dirx)/360.f;
        const float cloud=CloudOpacity(weather.cloud,wx/360.f,wy/360.f,across,
                                       weather.weather_seed);
        const float stripe=Noise(along*9.f+across*3.f,across*63.f,
                                 weather.weather_seed+519u);
        const float flakes=Noise(wx/7.f,wy/7.f,weather.weather_seed+911u);
        const bool cold=weather.precipitation==Precipitation::Snow||
                        weather.precipitation==Precipitation::Hail;
        const float visible=cold?Bound((flakes-.60f)*3.6f):
                                 Bound((stripe-.55f)*3.5f);
        const float precip=intensity*(.20f+.80f*visible);
        const float weather_haze=Bound(weather.ambient_haze+
            intensity*(weather.precipitation==Precipitation::HeavyStorm?.38f:.15f));
        const float hazeVariation=.78f+.22f*Fractal(wx/580.f,wy/580.f,
                                                     weather.weather_seed+241u);
        // Persistent camera contamination is keyed ONLY to lens and image
        // coordinates. It cannot change because cloud/wind/weather changed.
        const float dust=Fractal(u*13.f,v*13.f,camera.grime_seed);
        const float streak=Noise(u*3.f+v*.35f,v*24.f,camera.grime_seed+131u);
        const float grime= camera.grime *
            Bound(.12f+.75f*Bound((dust-.40f)*2.4f)+
                            .50f*Bound((streak-.61f)*3.f));
        next.cloud[idx]=Quantize(cloud);
        // Sunward parallax: the ground shadow is offset in physical
        // world coordinates from the visible cloud at flight altitude.
        // Unlike merely multiplying opacity, this can cover terrain
        // beyond the bright cloud silhouette while retaining the same
        // weather seed and wind trajectory.
        const float shadowX=wx+115.f,shadowY=wy-70.f;
        const float shadowAcross=(-shadowX*diry+shadowY*dirx)/360.f;
        const float shadowOpacity=CloudOpacity(weather.cloud,
            shadowX/360.f,shadowY/360.f,shadowAcross,
            weather.weather_seed);
        next.cloud_shadow[idx]=Quantize(shadowOpacity*.32f);
        next.precipitation[idx]=Quantize(precip);
        next.haze[idx]=Quantize(weather_haze*hazeVariation);
        next.lens_grime[idx]=Quantize(grime);
    }
    *out=std::move(next);
    return true;
}
inline float Sample(const std::vector<uint8_t>& channel,const MaskLayers& mask,
                    int x,int y,int image_width,int image_height){
    const float px=(x+.5f)*mask.width/image_width-.5f;
    const float py=(y+.5f)*mask.height/image_height-.5f;
    const int x0=std::max(0,std::min(mask.width-1,static_cast<int>(std::floor(px))));
    const int y0=std::max(0,std::min(mask.height-1,static_cast<int>(std::floor(py))));
    const int x1=std::min(mask.width-1,x0+1),y1=std::min(mask.height-1,y0+1);
    const float fx=Bound(px-x0),fy=Bound(py-y0);
    const auto at=[&](int xx,int yy){
        return channel[static_cast<size_t>(yy)*mask.width+xx]/255.f;
    };
    return ((at(x0,y0)*(1.f-fx)+at(x1,y0)*fx)*(1.f-fy)+
            (at(x0,y1)*(1.f-fx)+at(x1,y1)*fx)*fy);
}
inline bool Record(const GrayPlate& registered_source,
                   const Conditions& weather,const Camera& camera,
                   CapturedMaster* out){
    if(!out||!registered_source.Valid()||!Valid(weather,camera))return false;
    CapturedMaster next;
    if(!BuildMasks(registered_source.width,registered_source.height,
                   weather,camera,&next.masks))return false;
    next.recorded.width=registered_source.width;
    next.recorded.height=registered_source.height;
    next.recorded.pixels.resize(registered_source.pixels.size());
    next.conditions=weather;next.camera=camera;
    const auto& mask=next.masks;
    const bool bright=weather.precipitation==Precipitation::Snow||
                      weather.precipitation==Precipitation::Hail;
    for(int y=0;y<registered_source.height;++y)
        for(int x=0;x<registered_source.width;++x){
            const size_t i=static_cast<size_t>(y)*registered_source.width+x;
            float light=registered_source.pixels[i]/255.f;
            light*=1.f-.50f*Sample(mask.cloud_shadow,mask,x,y,
                                   registered_source.width,registered_source.height);
            const float haze=Sample(mask.haze,mask,x,y,
                                    registered_source.width,registered_source.height);
            light+=haze*(.64f-light);
            const float cloud=Sample(mask.cloud,mask,x,y,
                                     registered_source.width,registered_source.height);
            light+=cloud*(.82f-light);
            const float precip=Sample(mask.precipitation,mask,x,y,
                                      registered_source.width,registered_source.height);
            light+=precip*((bright?.93f:.45f)-light);
            const float grime=Sample(mask.lens_grime,mask,x,y,
                                     registered_source.width,registered_source.height);
            light*=1.f-.79f*grime;
            if(camera.sensor_noise>0.f){
                const float noise=Random(x,y,weather.weather_seed^0x81829ab1u)-.5f;
                light+=noise*camera.sensor_noise*.18f;
            }
            next.recorded.pixels[i]=Quantize(light);
        }
    *out=std::move(next);
    return true;
}
// Development/postprocess tiers of the same retained captured master:
// no recapture, weather removal, grime cleaning or invented image detail.
// Actual purchase prices and tier assignments belong to future HAP gameplay.
inline bool Develop(const CapturedMaster& source,unsigned resolution_tier,
                    GrayPlate* output){
    if(!output||!source.recorded.Valid()||resolution_tier>4u)return false;
    const auto& plate=source.recorded;
    constexpr int kCeiling[]={128,256,512,1024,4096};
    const int limit=kCeiling[resolution_tier];
    if(plate.width<=limit&&plate.height<=limit){
        *output=plate;return true;
    }
    const float factor=static_cast<float>(limit)/
                       std::max(plate.width,plate.height);
    const int low_w=std::max(1,static_cast<int>(std::ceil(plate.width*factor)));
    const int low_h=std::max(1,static_cast<int>(std::ceil(plate.height*factor)));
    std::vector<float> reduced(static_cast<size_t>(low_w)*low_h);
    // First box-average independent source pixels to avoid aliasing;
    // then bilinear reconstruction gives camera blur, not blocky squares.
    for(int cy=0;cy<low_h;++cy)for(int cx=0;cx<low_w;++cx){
        const int y0=cy*plate.height/low_h;
        const int y1=std::max(y0+1,(cy+1)*plate.height/low_h);
        const int x0=cx*plate.width/low_w;
        const int x1=std::max(x0+1,(cx+1)*plate.width/low_w);
        unsigned sum=0;
        for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)
            sum+=plate.pixels[static_cast<size_t>(y)*plate.width+x];
        reduced[static_cast<size_t>(cy)*low_w+cx]=
            static_cast<float>(sum)/((y1-y0)*(x1-x0));
    }
    GrayPlate developed{plate.width,plate.height,{}};
    developed.pixels.resize(plate.pixels.size());
    for(int y=0;y<plate.height;++y)for(int x=0;x<plate.width;++x){
        const float sx=(x+.5f)*low_w/plate.width-.5f;
        const float sy=(y+.5f)*low_h/plate.height-.5f;
        const int x0=std::max(0,std::min(low_w-1,static_cast<int>(std::floor(sx))));
        const int y0=std::max(0,std::min(low_h-1,static_cast<int>(std::floor(sy))));
        const int x1=std::min(low_w-1,x0+1),y1=std::min(low_h-1,y0+1);
        const float fx=Bound(sx-x0),fy=Bound(sy-y0);
        const auto at=[&](int a,int b){return reduced[static_cast<size_t>(b)*low_w+a];};
        const float level=(at(x0,y0)*(1.f-fx)+at(x1,y0)*fx)*(1.f-fy)+
                          (at(x0,y1)*(1.f-fx)+at(x1,y1)*fx)*fy;
        developed.pixels[static_cast<size_t>(y)*plate.width+x]=
            Quantize(level/255.f);
    }
    *output=std::move(developed);
    return true;
}
}}} // reverie::lantern::capture
#endif
