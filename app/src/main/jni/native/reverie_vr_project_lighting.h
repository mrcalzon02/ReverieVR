#ifndef REVERIE_VR_PROJECT_LIGHTING_H
#define REVERIE_VR_PROJECT_LIGHTING_H
// One bounded, authorable point light per VR project. GPU adapter in Test
// Chamber; Java 17 Beckenburg LightModel math mirrored without Swing.
#include "reverie_scene_document.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace reverie { namespace authoring {
struct VrLight {
    bool enabled=false;
    std::string anchor_id;
    double power=1.,radius=5.,edge=.65,ambient=.35;
};
inline bool ValidLight(const Scene& scene,const VrLight& light) {
    if(!light.enabled)return light.anchor_id.empty();
    bool found=false;
    for(const auto& p:scene.pieces)if(p.id==light.anchor_id)found=true;
    return found&&ValidId(light.anchor_id) &&
        std::isfinite(light.power)&&light.power>=.10&&light.power<=3. &&
        std::isfinite(light.radius)&&light.radius>=1.&&light.radius<=8. &&
        std::isfinite(light.edge)&&light.edge>=.10&&light.edge<=1. &&
        std::isfinite(light.ambient)&&light.ambient>=0.&&light.ambient<=1.05;
}
// Exact Beckenburg LightModel.diffusionFade and attenuation operations;
// shader uses the same equations, with occlusion currently unavailable.
inline double VrLightFade(double distance,double radius,double edge) {
    if(!std::isfinite(distance)||!std::isfinite(radius)||
       !std::isfinite(edge)||radius<=0||edge<=0||edge>1)
        throw std::invalid_argument("invalid diffusion");
    if(distance>=radius)return 0.;
    const double start=radius*(1.-edge);
    if(distance<=start)return 1.;
    return (radius-distance)/(radius*edge);
}
inline double VrLightContribution(double power,double fade,double distanceSquared) {
    if(!std::isfinite(power)||!std::isfinite(fade)||
       !std::isfinite(distanceSquared)||power<0||fade<0||fade>1||
       distanceSquared<0)
        throw std::invalid_argument("invalid light contribution");
    return power*fade*(.45+.55*fade)/(1.+.13*distanceSquared);
}
inline std::string EncodeVrLight(const Scene& scene,const VrLight& light){
    if(!ValidLight(scene,light))throw std::invalid_argument("invalid light anchor");
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out<<std::setprecision(17)<<"RVLIGHT\t1\n";
    if(light.enabled)out<<"light\t"<<light.anchor_id<<"\t"<<light.power<<"\t"
        <<light.radius<<"\t"<<light.edge<<"\t"<<light.ambient<<"\n";
    return out.str();
}
inline VrLight ParseVrLight(const Scene& scene,const std::string& source){
    if(source.empty()||source.size()>512||source.compare(0,10,"RVLIGHT\t1\n")!=0)
        throw std::invalid_argument("invalid light format");
    VrLight light;
    if(source.size()==10)return light;
    if(source.back()!='\n')throw std::invalid_argument("truncated light");
    const auto line=source.substr(10,source.size()-11);
    const auto fields=Fields(line);
    if(fields.size()!=6||fields[0]!="light")
        throw std::invalid_argument("invalid light fields");
    light.enabled=true;light.anchor_id=fields[1];
    light.power=Number(fields[2]);light.radius=Number(fields[3]);
    light.edge=Number(fields[4]);light.ambient=Number(fields[5]);
    if(!ValidLight(scene,light))throw std::invalid_argument("invalid light profile");
    return light;
}
}} // namespace
#endif
