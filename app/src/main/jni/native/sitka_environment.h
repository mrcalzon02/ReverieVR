#ifndef REVERIE_SITKA_ENVIRONMENT_H
#define REVERIE_SITKA_ENVIRONMENT_H

// Adapted *parameters*, not Java renderer code. Consumes Beckenburg v1 .water
// and Sitka desktop weather profile. The GLES2 module must later upload a
// bounded mesh and use these deterministic values once/update, twice/render.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace reverie::sitka {
inline constexpr float kPi=3.14159265358979323846f;

struct Water {
    std::string id;
    std::string shape;
    float x=0,y=0,z=0,width=8,depth=8,yaw=0;
    float height=.16f,speed=.8f,grid=.75f,shoreHeight=.06f;
    float scrollA=.05f,scrollB=.12f;
    // The source .water settings include alpha, Fresnel, refraction and
    // bathymetry. Unsupported optical settings are NOT represented as rendered.
};
struct Weather {
    std::string kind="CLEAR";
    float hour=13,wind=1,heading=0;
    float rain=0,snow=0,fog=0,visibility=1;
};
inline float Number(const std::map<std::string,std::string>& p,const std::string& key,
                    float low,float high,float fallback,bool required=false) {
    const auto it=p.find(key);
    if(it==p.end()) {
        if(required)throw std::invalid_argument("missing "+key);
        return fallback;
    }
    float n=0;
    try {size_t end=0;n=std::stof(it->second,&end);
         if(end!=it->second.size())throw std::invalid_argument("trailing characters");}
    catch(const std::exception&){throw std::invalid_argument("invalid "+key);}
    if(!std::isfinite(n)||n<low||n>high)throw std::invalid_argument("out of range "+key);
    return n;
}
inline std::map<std::string,std::string> Properties(const std::string& text) {
    if(text.size()>2'000'000)throw std::invalid_argument("oversized environment document");
    std::map<std::string,std::string> m;
    size_t pos=0;
    while(pos<text.size()){
        size_t end=text.find('\n',pos);if(end==std::string::npos)end=text.size();
        std::string line=text.substr(pos,end-pos);pos=end+1;
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#')continue;
        const auto split=line.find('=');
        if(split==std::string::npos||split==0||split==line.size()-1)
            throw std::invalid_argument("invalid property line");
        if(!m.emplace(line.substr(0,split),line.substr(split+1)).second)
            throw std::invalid_argument("duplicate property");
    }
    return m;
}
inline std::vector<Water> ParseWater(const std::string& text){
    const auto p=Properties(text);
    if(p.count("version")==0||p.at("version")!="1"||
       p.count("level")==0||p.at("level")!="sitka")
        throw std::invalid_argument("not Sitka Beckenburg water v1");
    const int count=static_cast<int>(Number(p,"count",0,64,0,true));
    std::vector<Water> water;water.reserve(count);
    for(int i=0;i<count;++i){
        const auto prefix="water."+std::to_string(i)+".";
        Water w;
        if(p.count(prefix+"id")==0||p.count(prefix+"shape")==0)
            throw std::invalid_argument("missing water id/shape");
        w.id=p.at(prefix+"id");w.shape=p.at(prefix+"shape");
        if(w.id.empty()||w.id.size()>64||
           (w.shape!="SQUARE"&&w.shape!="CIRCLE"&&w.shape!="OBLONG"&&
            w.shape!="OCTAGON"&&w.shape!="PILL"))
            throw std::invalid_argument("invalid water id/shape");
        w.x=Number(p,prefix+"x",-10000,10000,0,true);
        w.y=Number(p,prefix+"y",-10000,10000,0,true);
        w.z=Number(p,prefix+"z",-10000,10000,0,true);
        w.width=Number(p,prefix+"width",.1f,2000,8,true);
        w.depth=Number(p,prefix+"depth",.1f,2000,8,true);
        w.yaw=Number(p,prefix+"yaw",-kPi,kPi,0,true);
        const auto s=prefix+"setting.";
        w.height=Number(p,s+"wave.height",0,.5f,.16f);
        w.grid=Number(p,s+"wave.grid",.25f,8,.75f);
        w.speed=Number(p,s+"wave.speed",0,5,.8f);
        w.shoreHeight=Number(p,s+"shore.height",0,.08f,.06f);
        w.scrollA=Number(p,s+"layer1.speed",0,4,.05f);
        w.scrollB=Number(p,s+"layer2.speed",0,4,.12f);
        for(const auto& existing:water)if(existing.id==w.id)
            throw std::invalid_argument("duplicate water id");
        water.push_back(w);
    }
    return water;
}
inline Weather ParseWeather(const std::string& text){
    const auto p=Properties(text);
    if(p.count("version")==0||p.at("version")!="1"||!p.count("condition"))
        throw std::invalid_argument("invalid Sitka weather version");
    Weather w;w.kind=p.at("condition");
    w.hour=Number(p,"hour",0,23.9f,13,true);
    w.wind=Number(p,"wind_mps",0,60,1,true);
    w.heading=Number(p,"wind_heading_deg",0,360,0,true)*kPi/180.f;
    if(w.kind=="CLEAR"){}
    else if(w.kind=="OVERCAST"){w.fog=.12f;w.visibility=.86f;}
    else if(w.kind=="DRIZZLE"){w.rain=.17f;w.fog=.20f;w.visibility=.83f;}
    else if(w.kind=="RAIN"){w.rain=.52f;w.fog=.22f;w.visibility=.71f;}
    else if(w.kind=="HEAVY_RAIN"){w.rain=1;w.fog=.44f;w.visibility=.49f;}
    else if(w.kind=="THUNDERSTORM"){w.rain=.94f;w.fog=.53f;w.visibility=.44f;}
    else if(w.kind=="FOG"){w.fog=.59f;w.visibility=.44f;}
    else if(w.kind=="DENSE_FOG"){w.fog=.89f;w.visibility=.19f;}
    else if(w.kind=="SNOW"){w.snow=.50f;w.fog=.30f;w.visibility=.70f;}
    else throw std::invalid_argument("unsupported Sitka weather preset");
    return w;
}
// Coordinate contract: positive X=east, positive Z=north, positive Y=up.
// A dry point must not be counted as ocean just because an enclosing water
// footprint exists: later shoreline/bathymetry masks must reject dry land.
inline bool FootprintContains(const Water& w,float x,float z){
    const float dx=x-w.x,dz=z-w.z,c=std::cos(w.yaw),s=std::sin(w.yaw);
    const float a=dx*c+dz*s,b=-dx*s+dz*c;
    if(w.shape=="CIRCLE")return a*a+b*b<=w.width*w.width*.25f;
    if(w.shape=="OBLONG")return (a*a/(w.width*w.width)+b*b/(w.depth*w.depth))<=.25f;
    if(w.shape=="PILL"){
        const float r=std::min(w.width,w.depth)*.5f;
        if(w.width>=w.depth){const float edge=(w.width-w.depth)*.5f,offset=a-std::clamp(a,-edge,edge);return offset*offset+b*b<=r*r;}
        const float edge=(w.depth-w.width)*.5f,offset=b-std::clamp(b,-edge,edge);return offset*offset+a*a<=r*r;
    }
    // OCTAGON first-stage preview uses conservative rectangle bounds. Native
    // polygon tessellation/true edge clipping remains pending.
    return std::abs(a)<=w.width*.5f&&std::abs(b)<=w.depth*.5f;
}
inline float WaveHeight(const Water& w,float east,float north,float seconds){
    // Two independently advancing layers; simulation supplies one timestamp to
    // both eye renders. No GL or clock mutation occurs during sample/render.
    const float u=(east-w.x)/w.grid,v=(north-w.z)/w.grid;
    const float slow=std::sin(u*.39f+v*.52f+seconds*w.speed*.74f);
    const float chop=std::sin(u*1.14f-v*.95f-seconds*(w.speed+w.scrollB)*1.81f);
    return w.y+w.height*(.68f*slow+.32f*chop);
}
} // namespace reverie::sitka
#endif
