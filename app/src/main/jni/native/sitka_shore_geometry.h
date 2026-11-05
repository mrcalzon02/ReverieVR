#ifndef REVERIE_SITKA_SHORE_GEOMETRY_H
#define REVERIE_SITKA_SHORE_GEOMETRY_H

// Sitka game-private, source-anchored shore massing. All X/Z values are metres
// relative to terrain.sitkam's double-precision projected origin. This visual
// P0 pass does not confer a surveyed height, navigable surface or public entry.
#include "sitka_terrain_mesh.h"
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
struct ShorePoint { float x=0,z=0; };
struct ShoreFeature {
    std::string id, material;
    bool polygon=false, ramp=false;
    float x=0,z=0,base=0,width=0,depth=0,height=0,yaw=0,rise=0;
    std::vector<ShorePoint> footprint;
};
constexpr size_t kMaxShoreTextBytes=65536u;
constexpr size_t kMaxShoreFeatures=256u;
constexpr size_t kMaxShoreFootprintPoints=32u;

inline std::vector<std::string> ShoreSplit(const std::string& s,char separator){
    std::vector<std::string> out;
    size_t start=0;
    do { const size_t end=s.find(separator,start);
         out.push_back(s.substr(start,end==std::string::npos?end:end-start));
         if(end==std::string::npos)break;
         start=end+1;
    } while(true);
    return out;
}
inline float ShoreFloat(const std::string& value,float minimum,float maximum){
    size_t used=0;
    float n;
    try { n=std::stof(value,&used); }
    catch(const std::exception&){throw std::invalid_argument("Sitka shore number");}
    if(used!=value.size() || !std::isfinite(n) || n<minimum || n>maximum)
        throw std::invalid_argument("Sitka shore number range");
    return n;
}
inline float ShoreCross(ShorePoint a,ShorePoint b,ShorePoint c){
    return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);
}
inline bool ShoreBetween(float a,float b,float v){
    return v>=std::min(a,b)-.00001f && v<=std::max(a,b)+.00001f;
}
inline bool ShoreOnSegment(ShorePoint a,ShorePoint b,ShorePoint p){
    return std::abs(ShoreCross(a,b,p))<.00001f &&
           ShoreBetween(a.x,b.x,p.x)&&ShoreBetween(a.z,b.z,p.z);
}
inline bool ShoreSegmentsIntersect(ShorePoint a,ShorePoint b,
                                   ShorePoint c,ShorePoint d){
    const float ac=ShoreCross(a,b,c),ad=ShoreCross(a,b,d);
    const float ca=ShoreCross(c,d,a),cb=ShoreCross(c,d,b);
    if((((ac>0)&&(ad<0))||((ac<0)&&(ad>0)))&&
       (((ca>0)&&(cb<0))||((ca<0)&&(cb>0))))return true;
    return ShoreOnSegment(a,b,c)||ShoreOnSegment(a,b,d)||
           ShoreOnSegment(c,d,a)||ShoreOnSegment(c,d,b);
}
inline float ShoreArea(const std::vector<ShorePoint>& poly){
    double twice=0;
    for(size_t i=0;i<poly.size();++i){
        const auto a=poly[i],b=poly[(i+1)%poly.size()];
        twice+=double(a.x)*b.z-double(b.x)*a.z;
    }
    return static_cast<float>(twice*.5);
}
inline void ShoreCheckPolygon(const std::vector<ShorePoint>& poly){
    if(poly.size()<3u || poly.size()>kMaxShoreFootprintPoints ||
       std::abs(ShoreArea(poly))<.02f)
        throw std::invalid_argument("Sitka shore footprint area/count");
    for(size_t i=0;i<poly.size();++i){
        const auto a=poly[i],b=poly[(i+1u)%poly.size()];
        if(std::hypot(a.x-b.x,a.z-b.z)<.05f)
            throw std::invalid_argument("Sitka shore zero footprint edge");
        for(size_t j=i+1;j<poly.size();++j){
            if(j==i+1 || (i==0&&j+1==poly.size()))continue;
            if(ShoreSegmentsIntersect(a,b,poly[j],poly[(j+1)%poly.size()]))
                throw std::invalid_argument("Sitka shore intersecting footprint");
        }
    }
}
inline std::vector<ShoreFeature> ParseShoreLayout(const std::string& text){
    if(text.size()>kMaxShoreTextBytes)
        throw std::invalid_argument("Sitka shore file too large");
    std::istringstream in(text);
    std::string line;
    if(!std::getline(in,line)||line!="SITKA-SHORE-1")
        throw std::invalid_argument("Sitka shore version");
    std::vector<ShoreFeature> result;
    std::set<std::string> ids;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#')continue;
        const auto fields=ShoreSplit(line,'|');
        ShoreFeature p;
        if(fields.size()<3u||fields[1].empty()||fields[1].size()>64u)
            throw std::invalid_argument("Sitka shore feature id");
        p.id=fields[1];p.material=fields[2];
        for(char c:p.id) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                             (c>='0'&&c<='9')||c=='_'||c=='-'))
            throw std::invalid_argument("Sitka shore feature id character");
        if(!ids.insert(p.id).second)
            throw std::invalid_argument("Sitka shore duplicate id");
        if(fields[0]=="P"){
            if(fields.size()!=6u || (p.material!="storefront"&&
               p.material!="cannery"&&p.material!="heritage"&&
               p.material!="warehouse"))
                throw std::invalid_argument("Sitka shore polygon material");
            p.polygon=true;
            p.base=ShoreFloat(fields[3],-2000.f,2000.f);
            p.height=ShoreFloat(fields[4],.25f,70.f);
            for(const auto& point:ShoreSplit(fields[5],';')){
                const auto pair=ShoreSplit(point,',');
                if(pair.size()!=2u)throw std::invalid_argument("Sitka shore point");
                p.footprint.push_back({ShoreFloat(pair[0],-100000,100000),
                                       ShoreFloat(pair[1],-100000,100000)});
            }
            ShoreCheckPolygon(p.footprint);
        }else if(fields[0]=="B" || fields[0]=="R"){
            p.ramp=fields[0]=="R";
            if(fields.size()!=(p.ramp?11u:10u))
                throw std::invalid_argument("Sitka shore block fields");
            if(p.ramp?(p.material!="bridge_ramp"):
                      (p.material!="rock"&&p.material!="forest"&&
                       p.material!="pier"&&p.material!="pontoon"&&
                       p.material!="pile"&&p.material!="harbor_shack"&&
                       p.material!="bridge_deck"))
                throw std::invalid_argument("Sitka shore block material");
            p.x=ShoreFloat(fields[3],-100000,100000);
            p.z=ShoreFloat(fields[4],-100000,100000);
            p.base=ShoreFloat(fields[5],-2000,2000);
            p.width=ShoreFloat(fields[6],.05f,800);
            p.depth=ShoreFloat(fields[7],.05f,800);
            p.height=ShoreFloat(fields[8],.05f,80);
            p.yaw=ShoreFloat(fields[9],-3.141593f,3.141593f);
            if(p.ramp)p.rise=ShoreFloat(fields[10],-50,50);
            if(p.ramp&&p.height+std::min(0.f,p.rise)<.05f)
                throw std::invalid_argument("Sitka shore inverted bridge ramp");
        }else throw std::invalid_argument("Sitka shore record kind");
        if(result.size()>=kMaxShoreFeatures)
            throw std::invalid_argument("Sitka shore feature budget");
        result.push_back(std::move(p));
    }
    return result;
}
inline std::array<uint8_t,3> ShoreColor(const std::string& m){
    if(m=="rock")return {43,47,49};
    if(m=="forest")return {35,63,41};
    if(m=="pier")return {113,94,72};
    if(m=="pontoon")return {141,146,137};
    if(m=="pile")return {53,71,62};
    if(m=="cannery")return {151,160,157};
    if(m=="storefront")return {115,94,75};
    if(m=="heritage")return {84,125,89};
    if(m=="warehouse")return {105,113,110};
    if(m=="harbor_shack")return {180,54,49};
    if(m=="bridge_deck"||m=="bridge_ramp")return {143,148,148};
    throw std::invalid_argument("Sitka shore unknown material");
}
inline uint32_t ShoreVertex(TerrainMesh& mesh,float x,float y,float z,
                            const std::array<uint8_t,3>& color){
    if(mesh.vertices.size()>=kMaxTerrainVertices)
        throw std::invalid_argument("Sitka shore total vertex budget");
    mesh.vertices.push_back({x,y,z,color[0],color[1],color[2]});
    return static_cast<uint32_t>(mesh.vertices.size()-1u);
}
inline void ShoreTriangle(TerrainMesh& mesh,uint32_t a,uint32_t b,uint32_t c){
    if(mesh.indices.size()/3u>=kMaxTerrainTriangles)
        throw std::invalid_argument("Sitka shore total triangle budget");
    mesh.indices.insert(mesh.indices.end(),{a,b,c});
}
inline void ShoreQuad(TerrainMesh& mesh,
                      std::array<std::array<float,3>,4> pts,
                      std::array<uint8_t,3> color){
    const uint32_t start=static_cast<uint32_t>(mesh.vertices.size());
    for(const auto& p:pts)ShoreVertex(mesh,p[0],p[1],p[2],color);
    ShoreTriangle(mesh,start,start+1,start+2);
    ShoreTriangle(mesh,start,start+2,start+3);
}
inline void ShoreBox(TerrainMesh& mesh,const ShoreFeature& f){
    // Rise is authored from the -Z end toward +Z; bridge approaches do not
    // become a falsely walkable road graph until nav links are authored.
    const float c=std::cos(f.yaw),s=std::sin(f.yaw);
    std::array<std::array<float,3>,8> pts{};
    for(unsigned i=0;i<8u;++i){
        const float lx=(i&1u)?f.width*.5f:-f.width*.5f;
        const float lz=(i&2u)?f.depth*.5f:-f.depth*.5f;
        const bool upper=(i&4u)!=0u;
        pts[i]={f.x+lx*c-lz*s,
                f.base+(upper?f.height+((i&2u)?f.rise:0.f):0.f),
                f.z+lx*s+lz*c};
    }
    const auto color=ShoreColor(f.material);
    const auto dark=std::array<uint8_t,3>{
        static_cast<uint8_t>(color[0]*.73f),
        static_cast<uint8_t>(color[1]*.73f),
        static_cast<uint8_t>(color[2]*.73f)};
    ShoreQuad(mesh,{pts[4],pts[5],pts[7],pts[6]},color); // deck/top
    ShoreQuad(mesh,{pts[0],pts[2],pts[3],pts[1]},dark);
    ShoreQuad(mesh,{pts[0],pts[1],pts[5],pts[4]},dark);
    ShoreQuad(mesh,{pts[2],pts[6],pts[7],pts[3]},dark);
    ShoreQuad(mesh,{pts[0],pts[4],pts[6],pts[2]},dark);
    ShoreQuad(mesh,{pts[1],pts[3],pts[7],pts[5]},dark);
}
inline bool ShoreInside(ShorePoint a,ShorePoint b,ShorePoint c,ShorePoint p,
                        float orient){
    return orient*ShoreCross(a,b,p)>=-1.e-5f &&
           orient*ShoreCross(b,c,p)>=-1.e-5f &&
           orient*ShoreCross(c,a,p)>=-1.e-5f;
}
inline void ShorePolygon(TerrainMesh& mesh,const ShoreFeature& f){
    const auto& p=f.footprint;
    const float orientation=ShoreArea(p)>0.f?1.f:-1.f;
    auto color=ShoreColor(f.material);
    auto dark=std::array<uint8_t,3>{
        static_cast<uint8_t>(color[0]*.8f),
        static_cast<uint8_t>(color[1]*.8f),
        static_cast<uint8_t>(color[2]*.8f)};
    for(size_t i=0;i<p.size();++i){
        const auto a=p[i],b=p[(i+1)%p.size()];
        ShoreQuad(mesh,{{{a.x,f.base,a.z},{b.x,f.base,b.z},
                         {b.x,f.base+f.height,b.z},{a.x,f.base+f.height,a.z}}},dark);
    }
    std::vector<size_t> remaining;
    for(size_t i=0;i<p.size();++i)remaining.push_back(i);
    while(remaining.size()>3u){
        bool clipped=false;
        for(size_t i=0;i<remaining.size();++i){
            const size_t ia=remaining[(i+remaining.size()-1)%remaining.size()];
            const size_t ib=remaining[i];
            const size_t ic=remaining[(i+1)%remaining.size()];
            if(orientation*ShoreCross(p[ia],p[ib],p[ic])<1.e-5f)continue;
            bool occupied=false;
            for(size_t k:remaining){
                if(k!=ia&&k!=ib&&k!=ic&&
                   ShoreInside(p[ia],p[ib],p[ic],p[k],orientation)){
                    occupied=true;break;
                }
            }
            if(occupied)continue;
            const uint32_t a=ShoreVertex(mesh,p[ia].x,f.base+f.height,p[ia].z,color);
            const uint32_t b=ShoreVertex(mesh,p[ib].x,f.base+f.height,p[ib].z,color);
            const uint32_t c=ShoreVertex(mesh,p[ic].x,f.base+f.height,p[ic].z,color);
            ShoreTriangle(mesh,a,b,c);
            remaining.erase(remaining.begin()+static_cast<std::ptrdiff_t>(i));
            clipped=true;break;
        }
        if(!clipped)throw std::invalid_argument("Sitka shore roof triangulation failed");
    }
    for(size_t k:remaining)ShoreVertex(mesh,p[k].x,f.base+f.height,p[k].z,color);
    const uint32_t end=static_cast<uint32_t>(mesh.vertices.size());
    ShoreTriangle(mesh,end-3u,end-2u,end-1u);
}
inline void AppendShoreGeometry(TerrainMesh& mesh,const std::vector<ShoreFeature>& features){
    // Transactional CPU merge: malformed profiles cannot partially change
    // the authoritative terrain mesh. GPU upload remains once/context.
    TerrainMesh candidate=mesh;
    for(const auto& feature:features){
        if(feature.polygon)ShorePolygon(candidate,feature);
        else ShoreBox(candidate,feature);
    }
    mesh=std::move(candidate);
}
}} // namespace reverie::sitka
#endif