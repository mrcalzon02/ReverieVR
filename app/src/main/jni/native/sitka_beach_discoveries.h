#ifndef REVERIE_SITKA_BEACH_DISCOVERIES_H
#define REVERIE_SITKA_BEACH_DISCOVERIES_H
// Immutable, reviewed *geographic anchors*. Item kind, appearance and
// unlocks are game fiction, not a claim about real specimens at those sites.
// A source file is optional; zero file means zero fabricated items.
#include "sitka_free_roam.h"
#include "sitka_fishing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace reverie { namespace sitka {
struct BeachSite {
    uint32_t id=0u;
    BeachFind kind=BeachFind::Shell;
    float x=0.f,z=0.f;
};
struct BeachTarget {
    bool valid=false;
    uint32_t site=0u;
    BeachFind kind=BeachFind::Shell;
};
inline constexpr size_t kMaxBeachAssetBytes=8192u;
inline bool BeachId(const std::string& id){
    if(id.empty()||id.size()>64u)return false;
    for(char c:id)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return false;
    return true;
}
inline std::vector<std::string> BeachFields(const std::string& row){
    std::vector<std::string> fields;
    size_t pos=0;
    while(true){
        const size_t end=row.find('|',pos);
        fields.push_back(row.substr(pos,end==std::string::npos?
                                             end:end-pos));
        if(end==std::string::npos)break;
        pos=end+1u;
    }
    return fields;
}
inline double BeachNumber(const std::string& s,double min,double max){
    size_t end=0u;double n=0.;
    try{n=std::stod(s,&end);}
    catch(const std::exception&){throw std::invalid_argument("Sitka beach number");}
    if(end!=s.size()||!std::isfinite(n)||n<min||n>max)
        throw std::invalid_argument("Sitka beach number range");
    return n;
}
inline std::vector<BeachSite> ParseBeachSites(
    const std::string& data,double origin_east,double origin_north){
    if(data.size()>kMaxBeachAssetBytes)
        throw std::invalid_argument("Oversized Sitka beach discovery file");
    std::istringstream text(data);std::string row;
    if(!std::getline(text,row)||row!="SITKA-BEACH-1")
        throw std::invalid_argument("Sitka beach file version");
    bool provenance=false,origin=false;
    std::vector<BeachSite> result;
    std::set<uint32_t> used;
    while(std::getline(text,row)){
        if(!row.empty()&&row.back()=='\r')row.pop_back();
        if(row.empty()||row[0]=='#')continue;
        const auto f=BeachFields(row);
        if(f[0]=="P"){
            if(provenance||f.size()!=5u||!BeachId(f[1])||
               f[2].size()!=64u||f[3]!="EPSG32608"||f[4]!="reviewed")
                throw std::invalid_argument("Sitka beach provenance");
            for(char c:f[2])if(!((c>='a'&&c<='f')||(c>='0'&&c<='9')))
                throw std::invalid_argument("Sitka beach provenance SHA");
            provenance=true;
        }else if(f[0]=="O"){
            if(origin||f.size()!=3u||
               std::abs(BeachNumber(f[1],-1.e8,1.e8)-origin_east)>.001||
               std::abs(BeachNumber(f[2],-1.e8,1.e8)-origin_north)>.001)
                throw std::invalid_argument("Sitka beach origin mismatch");
            origin=true;
        }else if(f[0]=="D"){
            if(f.size()!=5u||result.size()>=64u)
                throw std::invalid_argument("Sitka beach site fields");
            const double raw=BeachNumber(f[1],0.,63.);
            if(raw!=std::floor(raw))
                throw std::invalid_argument("Sitka beach immutable ID");
            const uint32_t id=static_cast<uint32_t>(raw);
            if(!used.insert(id).second)
                throw std::invalid_argument("Sitka beach duplicate ID");
            BeachFind kind;
            if(f[2]=="shell")kind=BeachFind::Shell;
            else if(f[2]=="crab")kind=BeachFind::Crab;
            else if(f[2]=="rock")kind=BeachFind::Rock;
            else if(f[2]=="sea_glass")kind=BeachFind::SeaGlass;
            else if(f[2]=="driftwood")kind=BeachFind::Driftwood;
            else throw std::invalid_argument("Sitka unknown beach find");
            result.push_back({id,kind,
                static_cast<float>(BeachNumber(f[3],-100000.,100000.)),
                static_cast<float>(BeachNumber(f[4],-100000.,100000.))});
        }else throw std::invalid_argument("Sitka beach unsupported record");
    }
    if(!provenance||!origin||result.empty())
        throw std::invalid_argument("Missing reviewed Sitka beach sites");
    return result;
}
// Colored geometry is an explicitly temporary low-poly gameplay marker,
// NOT a geologic specimen model or a fabricated city/shore structure.
inline TerrainMesh BeachMarkerMesh(const std::vector<BeachSite>& sites,
    const Journal& j,const FreeRoamSession& roam){
    TerrainMesh mesh;
    constexpr std::array<std::array<uint8_t,3>,5> colors={{
        {{219,207,176}},{{196,73,53}},{{80,104,113}},
        {{58,194,174}},{{153,107,70}}
    }};
    for(const auto& site:sites){
        if((j.beach_sites&(uint64_t(1)<<site.id))!=0u)continue;
        const GroundHit ground=roam.GroundAt(site.x,site.z);
        if(!ground.valid)continue;
        const auto color=colors[unsigned(site.kind)];
        const uint32_t base=static_cast<uint32_t>(mesh.vertices.size());
        const float x=site.x,z=site.z,y=ground.y+.08f;
        const float width=site.kind==BeachFind::Crab?.20f:.14f;
        const float height=site.kind==BeachFind::Rock?.28f:.22f;
        mesh.vertices.insert(mesh.vertices.end(),{
            {x-width,y,z,color[0],color[1],color[2]},
            {x+width,y,z,color[0],color[1],color[2]},
            {x,y,z-width,color[0],color[1],color[2]},
            {x,y,z+width,color[0],color[1],color[2]},
            {x,y+height,z,color[0],color[1],color[2]},
            {x,y-.035f,z,color[0],color[1],color[2]}
        });
        for(unsigned t=0;t<4u;++t){
            constexpr uint32_t perimeter[]={0u,2u,1u,3u,0u};
            mesh.indices.insert(mesh.indices.end(),{
                base+perimeter[t],base+perimeter[t+1u],base+4u,
                base+perimeter[t+1u],base+perimeter[t],base+5u
            });
        }
    }
    return mesh;
}
inline BeachTarget FindBeachTarget(const float origin[3],
    const float ray[3],double player_x,double player_z,
    const FreeRoamSession& roam,const std::vector<BeachSite>& sites,
    uint64_t collected,uint32_t kit_level){
    BeachTarget best;
    if(!origin||!ray||!std::isfinite(player_x)||!std::isfinite(player_z))
        return best;
    for(unsigned i=0;i<3u;++i)
        if(!std::isfinite(origin[i])||!std::isfinite(ray[i]))return best;
    const double dirlen=std::hypot(std::hypot(ray[0],ray[1]),ray[2]);
    if(dirlen<.8||dirlen>1.2)return best;
    const double max_reach=2.75+.08*std::min(8u,kit_level);
    double nearest=max_reach+1.;
    for(const auto& site:sites){
        if((collected&(uint64_t(1)<<site.id))!=0u)continue;
        const GroundHit ground=roam.GroundAt(site.x,site.z);
        if(!ground.valid)continue;
        const double dx=double(site.x)-origin[0];
        const double dy=double(ground.y)+.23-origin[1];
        const double dz=double(site.z)-origin[2];
        const double near_player=std::hypot(double(site.x)-player_x,
                                            double(site.z)-player_z);
        if(near_player>max_reach)continue;
        const double along=dx*ray[0]+dy*ray[1]+dz*ray[2];
        if(along<.20||along>max_reach||along>=nearest)continue;
        const double off2=dx*dx+dy*dy+dz*dz-along*along;
        if(off2>.14)continue; // narrow pointer-select, not proximity auto-loot
        nearest=along;best={true,site.id,site.kind};
    }
    return best;
}
}} // namespace reverie::sitka
#endif
