#ifndef REVERIE_VR_PROJECT_SOUND_H
#define REVERIE_VR_PROJECT_SOUND_H
// Bounded object-anchored sound descriptors and stereo distance calculation.
// Only existing shell preloaded cues 1..21; never parses executable or WAV.
#include "reverie_scene_document.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
namespace reverie { namespace authoring {
struct VrSound {
    bool enabled=false;
    std::string anchor_id;
    unsigned cue=11u;
    double interval=3.,radius=6.;
};
inline bool ValidVrSound(const Scene& scene,const VrSound& sound){
    if(!sound.enabled)return sound.anchor_id.empty();
    bool found=false;
    for(const auto& p:scene.pieces)if(p.id==sound.anchor_id)found=true;
    return found&&ValidId(sound.anchor_id)&&sound.cue>=1&&sound.cue<=21&&
        std::isfinite(sound.interval)&&sound.interval>=.5&&sound.interval<=10.&&
        std::isfinite(sound.radius)&&sound.radius>=1.&&sound.radius<=12.;
}
inline std::string EncodeVrSound(const Scene& scene,const VrSound& s){
    if(!ValidVrSound(scene,s))throw std::invalid_argument("invalid sound binding");
    std::ostringstream o;o.imbue(std::locale::classic());
    o<<std::setprecision(17)<<"RVSOUND\t1\n";
    if(s.enabled)o<<"sound\t"<<s.anchor_id<<"\t"<<s.cue<<"\t"
                   <<s.interval<<"\t"<<s.radius<<"\n";
    return o.str();
}
inline VrSound ParseVrSound(const Scene& scene,const std::string& data){
    if(data.size()<10||data.size()>512||
       data.compare(0,10,"RVSOUND\t1\n")!=0)
        throw std::invalid_argument("invalid sound document");
    VrSound s;
    if(data.size()==10)return s;
    if(data.back()!='\n')throw std::invalid_argument("truncated sound");
    const auto f=Fields(data.substr(10,data.size()-11));
    if(f.size()!=5||f[0]!="sound"||f[2].empty()||f[2].size()>2)
        throw std::invalid_argument("invalid sound fields");
    unsigned cue=0;
    for(unsigned char c:f[2]){
        if(c<'0'||c>'9')throw std::invalid_argument("nondecimal cue");
        cue=cue*10u+static_cast<unsigned>(c-'0');
    }
    s.enabled=true;s.anchor_id=f[1];s.cue=cue;
    s.interval=Number(f[3]);s.radius=Number(f[4]);
    if(!ValidVrSound(scene,s))
        throw std::invalid_argument("dangling or unsafe sound binding");
    return s;
}
struct VrStereoCue {bool audible=false;float left=0.f,right=0.f;};
inline VrStereoCue SpatializeVrSound(double sx,double sy,double sz,
            double lx,double ly,double lz,double rx,double rz,double radius){
    const double dx=sx-lx,dy=sy-ly,dz=sz-lz;
    const double d2=dx*dx+dy*dy+dz*dz,unit=rx*rx+rz*rz;
    if(!std::isfinite(d2)||!std::isfinite(unit)||!std::isfinite(radius)||
       radius<1.||radius>12.||unit<.95||unit>1.05||d2>radius*radius)return {};
    const double planar=std::hypot(dx,dz);
    const double pan=planar>.0001?
        std::max(-1.,std::min(1.,(dx*rx+dz*rz)/planar)):0.;
    const double volume=.25+.75/(1.+4.*d2/(radius*radius));
    return {true,static_cast<float>(volume*(pan>0.?1.-.8*pan:1.)),
        static_cast<float>(volume*(pan<0.?1.+.8*pan:1.))};
}
}}
#endif
