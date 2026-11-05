#ifndef REVERIE_SITKA_CITY_DISCOVERIES_H
#define REVERIE_SITKA_CITY_DISCOVERIES_H
// Reviewed public Sitka exploration points only. No authored file means
// no city reward. Visit nearby sourced landmarks on foot for training.
#include "sitka_beach_discoveries.h"
#include <cstdint>
#include <cmath>
#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace reverie { namespace sitka {
struct CitySite{
    uint32_t id=0u;
    CityFind kind=CityFind::Harbor;
    float x=0.f,z=0.f;
};
struct CityVisit{
    bool valid=false;
    uint32_t site=0u;
    CityFind kind=CityFind::Harbor;
};
inline std::vector<CitySite> ParseCitySites(
    const std::string& data,double origin_east,double origin_north){
    if(data.size()>8192u)
        throw std::invalid_argument("Sitka city exploration file oversized");
    std::istringstream input(data);
    std::string row;
    if(!std::getline(input,row)||row!="SITKA-CITY-1")
        throw std::invalid_argument("Sitka city exploration file version");
    bool provenance=false,origin=false;
    std::set<uint32_t> used;
    std::vector<CitySite> sites;
    while(std::getline(input,row)){
        if(!row.empty()&&row.back()=='\r')row.pop_back();
        if(row.empty()||row[0]=='#')continue;
        const auto f=BeachFields(row);
        if(f[0]=="P"){
            if(provenance||f.size()!=5u||!BeachId(f[1])||
               f[2].size()!=64u||f[3]!="EPSG32608"||f[4]!="reviewed")
                throw std::invalid_argument("Sitka city provenance");
            for(char c:f[2])
                if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))
                    throw std::invalid_argument("Sitka city SHA format");
            provenance=true;
        }else if(f[0]=="O"){
            if(origin||f.size()!=3u||
               std::abs(BeachNumber(f[1],-1.e8,1.e8)-origin_east)>.001||
               std::abs(BeachNumber(f[2],-1.e8,1.e8)-origin_north)>.001)
                throw std::invalid_argument("Sitka city origin mismatch");
            origin=true;
        }else if(f[0]=="D"){
            if(f.size()!=5u||sites.size()>=64u)
                throw std::invalid_argument("Sitka city site fields");
            const double raw=BeachNumber(f[1],0.,63.);
            if(raw!=std::floor(raw))
                throw std::invalid_argument("Sitka city immutable ID");
            const uint32_t id=static_cast<uint32_t>(raw);
            if(!used.insert(id).second)
                throw std::invalid_argument("Sitka city duplicate ID");
            CityFind kind;
            if(f[2]=="harbor")kind=CityFind::Harbor;
            else if(f[2]=="heritage")kind=CityFind::Heritage;
            else if(f[2]=="public_art")kind=CityFind::PublicArt;
            else if(f[2]=="trailhead")kind=CityFind::Trailhead;
            else if(f[2]=="civic")kind=CityFind::Civic;
            else throw std::invalid_argument("Sitka city unknown category");
            sites.push_back({id,kind,
                static_cast<float>(BeachNumber(f[3],-100000.,100000.)),
                static_cast<float>(BeachNumber(f[4],-100000.,100000.))});
        }else throw std::invalid_argument("Sitka city unsupported record");
    }
    if(!provenance||!origin||sites.empty())
        throw std::invalid_argument("Missing reviewed Sitka city landmarks");
    return sites;
}
inline CityVisit FindCityVisit(const FreeRoamSession& roam,
    const std::vector<CitySite>& sites,uint64_t visited,
    uint32_t observation_level){
    CityVisit found;
    const RoamPose pose=roam.pose();
    const double reach=2.5+.10*std::min(8u,observation_level);
    double best=reach;
    for(const auto& site:sites){
        if(site.id>=64u||(visited&(uint64_t(1)<<site.id)))continue;
        const double d=std::hypot(double(site.x)-pose.x,
                                  double(site.z)-pose.z);
        if(d>=best)continue;
        const auto hit=roam.GroundAt(site.x,site.z);
        if(!hit.valid||std::abs(hit.y-pose.ground_y)>1.)continue;
        best=d;found={true,site.id,site.kind};
    }
    return found;
}
}} // namespace reverie::sitka
#endif
