#ifndef REVERIE_SITKA_BRIDGE_LAYOUT_H
#define REVERIE_SITKA_BRIDGE_LAYOUT_H

// Strict source-only bridge placement: no guessed Sitka axis or elevation.
// Author must supply actual projected origin, source checksum, approved site,
// and 2..65 checked centreline/grade points. Parsing never reads geography.
#include "sitka_bridge_planes.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace reverie { namespace sitka {
constexpr size_t kMaxBridgeLayoutBytes=8192u;

inline std::string BridgeTrim(const std::string& s){
    const size_t a=s.find_first_not_of(" \t\r");
    if(a==std::string::npos)return {};
    const size_t b=s.find_last_not_of(" \t\r");
    return s.substr(a,b-a+1u);
}
inline double BridgeNumber(const std::string& s){
    size_t end=0u;
    const double d=std::stod(s,&end);
    if(end!=s.size()||!std::isfinite(d))
        throw std::invalid_argument("Invalid authored bridge number");
    return d;
}
inline bool BridgeSourceSha(const std::string& s){
    if(s.size()!=64u)return false;
    for(char c:s)
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
inline bool BridgeSourceId(const std::string& s){
    if(s.size()<5u||s.size()>128u)return false;
    for(char c:s)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'||c==':'))return false;
    return true;
}
inline BridgePlaneMesh ParseBridgeLayout(
    const std::string& text,double terrain_east,double terrain_north){
    if(text.size()>kMaxBridgeLayoutBytes ||
       !std::isfinite(terrain_east)||!std::isfinite(terrain_north))
        throw std::invalid_argument("Sitka bridge layout byte/origin budget");
    std::istringstream input(text);
    std::string line;
    if(!std::getline(input,line)||BridgeTrim(line)!="SITKA-BRIDGE-1")
        throw std::invalid_argument("Sitka bridge layout version mismatch");
    std::map<std::string,std::string> values;
    while(std::getline(input,line)){
        line=BridgeTrim(line);
        if(line.empty())continue;
        const size_t sep=line.find('=');
        if(sep==std::string::npos)
            throw std::invalid_argument("Invalid Sitka bridge layout entry");
        const std::string key=BridgeTrim(line.substr(0u,sep));
        const std::string value=BridgeTrim(line.substr(sep+1u));
        if(key.empty()||value.empty()||!values.emplace(key,value).second)
            throw std::invalid_argument("Duplicate or blank bridge property");
    }
    const auto value=[&](const std::string& k)->const std::string&{
        const auto it=values.find(k);
        if(it==values.end())throw std::invalid_argument(
            "Missing authored bridge property: "+k);
        return it->second;
    };
    if(value("site_id")!="oconnell_bridge" ||
       value("geometry_reviewed")!="true" ||
       value("coordinate_system")!="EPSG:32608" ||
       !BridgeSourceId(value("source_feature_id")) ||
       !BridgeSourceSha(value("source_sha256")))
        throw std::invalid_argument("Bridge source identity/review gate failed");
    const double origin_e=BridgeNumber(value("origin_east"));
    const double origin_n=BridgeNumber(value("origin_north"));
    if(std::abs(origin_e-terrain_east)>.001 ||
       std::abs(origin_n-terrain_north)>.001)
        throw std::invalid_argument("Bridge and terrain CRS origins disagree");
    const double count_number=BridgeNumber(value("axis_count"));
    if(count_number<2.||count_number>65.||std::floor(count_number)!=count_number)
        throw std::invalid_argument("Invalid bridge axis point count");
    const size_t count=static_cast<size_t>(count_number);
    if(values.size()!=11u+count)
        throw std::invalid_argument("Unexpected bridge layout property");
    std::vector<BridgeAxisPoint> axis;
    axis.reserve(count);
    for(size_t i=0u;i<count;++i){
        const auto& triplet=value("axis."+std::to_string(i));
        std::istringstream tuple(triplet);
        std::string part;
        std::array<float,3> coords{};
        for(size_t k=0u;k<3u;++k){
            if(!std::getline(tuple,part,','))
                throw std::invalid_argument("Incomplete bridge axis point");
            const double n=BridgeNumber(BridgeTrim(part));
            if(std::abs(n)>100000.)
                throw std::invalid_argument("Bridge point magnitude excessive");
            coords[k]=static_cast<float>(n);
        }
        if(std::getline(tuple,part,','))
            throw std::invalid_argument("Extra bridge axis coordinate");
        axis.push_back({coords[0],coords[1],coords[2]});
    }
    const float width=static_cast<float>(BridgeNumber(value("width_m")));
    const float thickness=static_cast<float>(BridgeNumber(value("deck_thickness_m")));
    const float silhouette=static_cast<float>(BridgeNumber(value("side_height_m")));
    return BuildBridgePlanes(axis,width,thickness,silhouette);
}
}} // namespace reverie::sitka
#endif
