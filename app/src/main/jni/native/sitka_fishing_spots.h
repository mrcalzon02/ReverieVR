#ifndef REVERIE_SITKA_FISHING_SPOTS_H
#define REVERIE_SITKA_FISHING_SPOTS_H

// Editor-authored *surveyed* fishing targets. No fake depth estimates from
// terrain, weather or 2D water footprints. An absent target file fails closed.
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace reverie { namespace sitka {
struct FishingSpot {
    std::string id, water_id;
    float east=0.f, north=0.f, radius=0.f, depth_metres=0.f;
};
inline bool FishingId(const std::string& s) {
    if(s.empty()||s.size()>48u)return false;
    for(char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
       (c>='0'&&c<='9')||c=='-'||c=='_'))return false;
    return true;
}
inline std::vector<FishingSpot> ParseFishingSpots(const std::string& text) {
    if(text.size()>8192u)throw std::invalid_argument("oversized Sitka fishing sites");
    std::istringstream input(text);
    std::string line;
    if(!std::getline(input,line)||line!="SITKA-FISH-SITES-1")
        throw std::invalid_argument("invalid Sitka fishing site version");
    std::vector<FishingSpot> sites;
    while(std::getline(input,line)){
        if(line.empty()||line.front()=='#')continue;
        if(sites.size()>=32u||line.size()>256u)
            throw std::invalid_argument("too many/oversized Sitka fishing sites");
        std::istringstream row(line);
        FishingSpot site;
        std::string trailing;
        if(!(row>>site.id>>site.water_id>>site.east>>site.north>>site.radius>>site.depth_metres)
           ||(row>>trailing)||!FishingId(site.id)||!FishingId(site.water_id)
           ||!std::isfinite(site.east)||!std::isfinite(site.north)
           ||!std::isfinite(site.radius)||!std::isfinite(site.depth_metres)
           ||std::abs(site.east)>10000.f||std::abs(site.north)>10000.f
           ||site.radius<.3f||site.radius>12.f
           ||site.depth_metres<.3f||site.depth_metres>200.f)
            throw std::invalid_argument("invalid Sitka fishing site record");
        for(const auto& previous:sites)if(previous.id==site.id)
            throw std::invalid_argument("duplicate Sitka fishing site id");
        sites.push_back(site);
    }
    if(sites.empty())throw std::invalid_argument("no authored Sitka fishing sites");
    return sites;
}
struct FishingRay {
    float ox=0.f,oy=0.f,oz=0.f,dx=0.f,dy=0.f,dz=0.f;
};
struct FishingCastTarget {
    bool valid=false;
    float depth_metres=0.f;
    float east=0.f,north=0.f;
};
// Uses actual authored water footprint and surveyed fishing site together.
// Does not claim global shoreline clipping; no synthetic data is shipped.
template<typename WaterContainer>
inline FishingCastTarget FindFishingTarget(const FishingRay& ray,
    const std::vector<FishingSpot>& spots,const WaterContainer& waters) {
    FishingCastTarget found;
    const float len2=ray.dx*ray.dx+ray.dy*ray.dy+ray.dz*ray.dz;
    if(!std::isfinite(ray.ox)||!std::isfinite(ray.oy)||!std::isfinite(ray.oz)||
       !std::isfinite(len2)||len2<.25f||len2>2.25f||
       !std::isfinite(ray.dx)||!std::isfinite(ray.dy)||!std::isfinite(ray.dz)||
       ray.dy>=-.01f)return found;
    float closest=35.f;
    for(const FishingSpot& site:spots){
        for(const auto& w:waters){
            if(w.id!=site.water_id)continue;
            const float t=(w.y-ray.oy)/ray.dy;
            if(!std::isfinite(t)||t<.2f||t>closest)continue;
            const float east=ray.ox+t*ray.dx,north=ray.oz+t*ray.dz;
            const float a=east-site.east,b=north-site.north;
            if(a*a+b*b>site.radius*site.radius||!FootprintContains(w,east,north))continue;
            found={true,site.depth_metres,east,north};
            closest=t;
        }
    }
    return found;
}
}} // namespace reverie::sitka
#endif
