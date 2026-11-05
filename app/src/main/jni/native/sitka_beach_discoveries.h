#ifndef REVERIE_SITKA_BEACH_DISCOVERIES_H
#define REVERIE_SITKA_BEACH_DISCOVERIES_H
// Immutable, reviewed *geographic anchors*. Item kind, appearance and
// unlocks are game fiction, not a claim about real specimens at those sites.
// A source file is optional; zero file means zero fabricated items.
#include "sitka_free_roam.h"
#include "sitka_fishing.h"
#include "sitka_intertidal_models.h"
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
    ShoreSpecimen specimen=ShoreSpecimen::EmptyClamShell;
    unsigned zone=unsigned(ShoreZone::Gravel);
};
struct BeachTarget {
    bool valid=false;
    uint32_t site=0u;
    BeachFind kind=BeachFind::Shell;
    ShoreSpecimen specimen=ShoreSpecimen::EmptyClamShell;
    ShoreAction action=ShoreAction::Collect;
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
            if((f.size()!=5u&&f.size()!=6u)||result.size()>=64u)
                throw std::invalid_argument("Sitka beach site fields");
            const double raw=BeachNumber(f[1],0.,63.);
            if(raw!=std::floor(raw))
                throw std::invalid_argument("Sitka beach immutable ID");
            const uint32_t id=static_cast<uint32_t>(raw);
            if(!used.insert(id).second)
                throw std::invalid_argument("Sitka beach duplicate ID");
            ShoreSpecimen type=ShoreSpecimen::EmptyClamShell;
            bool old_tag=true;
            if(f[2]=="shell")type=ShoreSpecimen::EmptyClamShell;
            else if(f[2]=="crab")type=ShoreSpecimen::PurpleShoreCrab;
            else if(f[2]=="rock")type=ShoreSpecimen::BeachPebble;
            else if(f[2]=="sea_glass")type=ShoreSpecimen::SeaGlass;
            else if(f[2]=="driftwood")type=ShoreSpecimen::DriftwoodChip;
            else{
                old_tag=false;
                if(!ShoreSpecimenByToken(f[2],type))
                    throw std::invalid_argument("Sitka unknown shore species");
            }
            const auto& def=Specimen(type);
            unsigned zone=0u;
            if(f.size()==6u){
                if(!ShoreZoneByToken(f[5],zone)||!(def.zones&zone))
                    throw std::invalid_argument("Sitka shore habitat mismatch");
            }else if(!old_tag)
                throw std::invalid_argument("Specific species need reviewed habitat");
            else zone=def.zones&(~def.zones+1u); // legacy record without substrate
            result.push_back({id,def.category,
                static_cast<float>(BeachNumber(f[3],-100000.,100000.)),
                static_cast<float>(BeachNumber(f[4],-100000.,100000.)),
                type,zone});
        }else throw std::invalid_argument("Sitka beach unsupported record");
    }
    if(!provenance||!origin||result.empty())
        throw std::invalid_argument("Missing reviewed Sitka beach sites");
    return result;
}
// Only models within exactly five feet of the grounded player are sent to
// GLES2. A creature or protected archaeological trace remains in the world
// after observation; only inert collectables are removed.
inline uint64_t BeachVisibleMask(const std::vector<BeachSite>& sites,
    const Journal& journal,const FreeRoamSession& roam){
    uint64_t mask=0u;
    const auto player=roam.pose();
    for(const auto& s:sites){
        if(s.id>=64u)continue;
        const bool found=(journal.beach_sites&(uint64_t(1)<<s.id))!=0u;
        if(found&&Specimen(s.specimen).action==ShoreAction::Collect)continue;
        if(std::hypot(double(s.x)-player.x,double(s.z)-player.z)>
           kShoreRevealRadiusM)continue;
        const auto ground=roam.GroundAt(s.x,s.z);
        if(!ground.valid||std::abs(ground.y-player.ground_y)>1.25f)continue;
        mask|=uint64_t(1)<<s.id;
    }
    return mask;
}
inline TerrainMesh BeachMarkerMesh(const std::vector<BeachSite>& sites,
    const Journal& journal,const FreeRoamSession& roam,uint64_t visible){
    TerrainMesh mesh;
    const auto player=roam.pose();
    for(const auto& s:sites){
        if(s.id>=64u||(visible&(uint64_t(1)<<s.id))==0u)continue;
        const bool found=(journal.beach_sites&(uint64_t(1)<<s.id))!=0u;
        if(found&&Specimen(s.specimen).action==ShoreAction::Collect)continue;
        // Do not trust a stale visibility mask supplied by the caller.
        if(std::hypot(double(s.x)-player.x,double(s.z)-player.z)>
           kShoreRevealRadiusM)continue;
        const auto ground=roam.GroundAt(s.x,s.z);
        if(!ground.valid||std::abs(ground.y-player.ground_y)>1.25f)continue;
        AppendShoreSpecimen(mesh,s.specimen,s.x,ground.y,s.z,!found);
    }
    return mesh;
}
inline BeachTarget FindBeachTarget(const float origin[3],
    const float ray[3],double player_x,double player_z,
    const FreeRoamSession& roam,const std::vector<BeachSite>& sites,
    uint64_t collected,uint32_t kit_level,
    uint32_t observation_level=0u){
    BeachTarget best;
    if(!origin||!ray||!std::isfinite(player_x)||!std::isfinite(player_z))
        return best;
    for(unsigned i=0;i<3u;++i)
        if(!std::isfinite(origin[i])||!std::isfinite(ray[i]))return best;
    const double dirlen=std::hypot(std::hypot(ray[0],ray[1]),ray[2]);
    if(dirlen<.8||dirlen>1.2)return best;
    const double max_reach=2.75+.08*std::min(8u,kit_level)+
                           .08*std::min(8u,observation_level);
    double nearest=max_reach+1.;
    for(const auto& site:sites){
        if((collected&(uint64_t(1)<<site.id))!=0u)continue;
        const GroundHit ground=roam.GroundAt(site.x,site.z);
        if(!ground.valid)continue;
        const double dx=double(site.x)-origin[0];
        const double dy=double(ground.y)+.04-origin[1];
        const double dz=double(site.z)-origin[2];
        const double near_player=std::hypot(double(site.x)-player_x,
                                            double(site.z)-player_z);
        if(near_player>kShoreRevealRadiusM)continue;
        const double along=dx*ray[0]+dy*ray[1]+dz*ray[2];
        if(along<.20||along>max_reach||along>=nearest)continue;
        const double off2=dx*dx+dy*dy+dz*dz-along*along;
        if(off2>.038)continue; // deliberate, fine VR pointer precision
        nearest=along;
        const auto& spec=Specimen(site.specimen);
        best={true,site.id,site.kind,site.specimen,spec.action};
    }
    return best;
}
}} // namespace reverie::sitka
#endif
