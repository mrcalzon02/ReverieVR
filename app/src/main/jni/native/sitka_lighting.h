#ifndef REVERIE_SITKA_LIGHTING_H
#define REVERIE_SITKA_LIGHTING_H

// Strict bridge for the actual Java 17 LightingAuthoringPanel schema=1 file.
// Only authored AMBIENT intensity is mapped to the source-only GLES2 terrain.
// Source beam, reach, diffusion, flicker and cache settings are validated but
// are NOT represented as dynamic lights, occlusion or shadows.
#include "sitka_environment.h"
#include <map>
#include <stdexcept>
#include <string>

namespace reverie { namespace sitka {
struct Lighting {
    float ambient_scale = 1.0f;
};
inline const std::string& LightingValue(
    const std::map<std::string,std::string>& p,const char* key) {
    const auto it=p.find(key);
    if(it==p.end())throw std::invalid_argument(std::string("missing Sitka lighting ")+key);
    return it->second;
}
inline int LightingInteger(const std::map<std::string,std::string>& p,
                           const char* key,int low,int high) {
    const auto& raw=LightingValue(p,key);
    if(raw.empty())throw std::invalid_argument(std::string("empty Sitka lighting ")+key);
    int value=0;
    for(const unsigned char digit:raw) {
        if(digit<'0'||digit>'9'||value>(high-(digit-'0'))/10)
            throw std::invalid_argument(std::string("invalid Sitka lighting ")+key);
        value=value*10+(digit-'0');
    }
    if(value<low||value>high)
        throw std::invalid_argument(std::string("out of range Sitka lighting ")+key);
    return value;
}
inline bool LightingBoolean(const std::map<std::string,std::string>& p,const char* key) {
    const auto& raw=LightingValue(p,key);
    if(raw=="true")return true;
    if(raw=="false")return false;
    throw std::invalid_argument(std::string("invalid Sitka lighting boolean ")+key);
}
inline Lighting ParseLighting(const std::string& text) {
    const auto p=Properties(text);
    // Java Properties.store adds comment lines; Properties() ignores those.
    // The v1 authoring dock writes exactly these 13 properties.
    if(p.size()!=13u||LightingValue(p,"schema")!="1")
        throw std::invalid_argument("invalid Sitka lighting schema");
    const auto& preset=LightingValue(p,"preset");
    if(preset!="CUSTOM"&&preset!="BALANCED"&&
       preset!="PERFORMANCE"&&preset!="QUALITY")
        throw std::invalid_argument("invalid Sitka lighting preset");
    const auto& beam=LightingValue(p,"beam");
    if(beam!="FLUORESCENT"&&beam!="VAN_DIFFUSER"&&
       beam!="TELEVISION"&&beam!="TABLE_LAMP"&&
       beam!="FROSTED_DOME"&&beam!="OMNIDIRECTIONAL")
        throw std::invalid_argument("invalid Sitka lighting beam");
    const int ambient=LightingInteger(p,"ambient",50,150);
    const int source=LightingInteger(p,"source",50,150);
    const int reach=LightingInteger(p,"reach",65,140);
    const int diffusion=LightingInteger(p,"diffusion",5,100);
    const int flicker=LightingInteger(p,"flicker",0,200);
    const int cache=LightingInteger(p,"cache",50,150);
    const int cell=LightingInteger(p,"cell",2,8);
    if(cell!=2&&cell!=4&&cell!=8)
        throw std::invalid_argument("invalid Sitka lighting cell size");
    const bool outdoor=LightingBoolean(p,"outdoor");
    const bool changed=LightingBoolean(p,"changed");
    const bool reuse=LightingBoolean(p,"reuse");
    // Match Beckenburg LightingModule.preset(): named presets are canonical.
    if(preset!="CUSTOM") {
        const int expected_cache=preset=="PERFORMANCE"?70:
                                 preset=="QUALITY"?140:100;
        const int expected_cell=preset=="QUALITY"?2:4;
        const bool expected_outdoor=preset!="QUALITY";
        if(ambient!=100||source!=100||reach!=100||diffusion!=100||
           flicker!=100||cache!=expected_cache||cell!=expected_cell||
           outdoor!=expected_outdoor||!changed||!reuse)
            throw std::invalid_argument("noncanonical Sitka lighting preset");
    }
    return Lighting{ambient/100.0f};
}
}} // namespace reverie::sitka
#endif
