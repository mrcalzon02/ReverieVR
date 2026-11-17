#ifndef REVERIE_SITKA_CRESCENT_HARBOR_H
#define REVERIE_SITKA_CRESCENT_HARBOR_H
#include "sitka_boat_mesh.h"
#include <algorithm>

namespace reverie { namespace sitka {
// In metres from (480000 E, 6322900 N), EPSG:32608. The centerline passes
// through the city gangway (31) and connected dock polygon (38). Every segment
// is within the measured horizontal footprint. Heights below are game art.
struct DeckPoint {float east,north;};
inline constexpr std::array<DeckPoint,6> kCrescentWalk={{
    {38.70f,216.f},{36.33f,193.65f},{36.23f,191.50f},{44.8f,190.85f},
    {42.75f,143.54f},{40.85f,99.66f}
}};
inline const std::array<float,kCrescentWalk.size()>& CrescentDistances(){
    static const auto stops=[](){
        std::array<float,kCrescentWalk.size()> out{};
        for(size_t i=1;i<out.size();++i){
            auto a=kCrescentWalk[i-1],b=kCrescentWalk[i];
            out[i]=out[i-1]+std::hypot(b.east-a.east,b.north-a.north);
        }
        return out;
    }();
    return stops;
}
inline DeckPoint CrescentPosition(float metres){
    const auto& stops=CrescentDistances();
    for(size_t i=1;i<stops.size();++i){
        if(metres<=stops[i]){
            const float t=std::clamp((metres-stops[i-1])/(stops[i]-stops[i-1]),0.f,1.f);
            auto a=kCrescentWalk[i-1],b=kCrescentWalk[i];
            return {a.east+(b.east-a.east)*t,a.north+(b.north-a.north)*t};
        }
    }
    return kCrescentWalk.back();
}
// NOAA US5AK2NE.000, SOUNDG LNAM 0226AA147BE7DC34: 3.9 m below MLLW.
// Converted from WGS84 to the same EPSG:32608 origin as the city dock.
inline constexpr DeckPoint kCrescentSounding={63.58416f,117.25269f};
inline constexpr float kCrescentSoundingDepth=3.9f;
inline float CrescentCastMark(){
    const auto& stops=CrescentDistances();
    return stops[4]+(stops[5]-stops[4])*(143.54f-124.f)/(143.54f-99.66f);
}
inline bool CrescentCanCast(float walked){return std::abs(walked-CrescentCastMark())<=1.8f;}
inline TerrainMesh CrescentFishingLine(){
    TerrainMesh mesh;
    mesh.vertices={{0,0,-.045f,242,230,167},{0,0,.045f,242,230,167},
                   {1,0,.045f,236,236,209},{1,0,-.045f,236,236,209}};
    mesh.indices={0,1,2,0,2,3};return mesh;
}
struct DockStop {size_t node; const char* name; const char* action;};
inline constexpr std::array<DockStop,4> kCrescentStops={{
    {0,"GANGWAY / PICKUP","SELECT: TAKE NET BUNDLE"},
    {4,"FLOAT 2 / BERTH 2-4","SELECT: HAND OVER NET / TAKE TAGS"},
    {5,"FLOAT 2 / BERTH 2-10","SELECT: DELIVER TAGS / TAKE RECEIPT"},
    {0,"GANGWAY / RETURN","SELECT: FILE RECEIPT  +60 CR"}
}};
inline void DressCrescentDeck(TerrainMesh& mesh){
    using Color=std::array<uint8_t,3>;
    auto quad=[&](DeckPoint a,DeckPoint b,DeckPoint c,DeckPoint d,float y,Color color){
        const auto idx=uint32_t(mesh.vertices.size());
        for(auto p:{a,b,c,d})mesh.vertices.push_back({p.east,y,p.north,color[0],color[1],color[2]});
        for(auto i:{0u,1u,2u,0u,2u,3u})mesh.indices.push_back(idx+i);
    };
    // Shaded harbor surface for the walking view only. It supplies no depth,
    // cast target or navigation data.
    quad({-180,-65},{345,-65},{345,252},{-180,252},0,{39,91,109});
    for(int row=0;row<29;++row)for(int col=0;col<25;++col){
        const float x=-158.f+19.5f*col+float((row*17+col*7)%13);
        const float z=-55.f+10.5f*row+float((col*13+row*3)%5);
        quad({x,z},{x+3.0f,z+.12f},{x+4.8f,z+.37f},{x+1.5f,z+.28f},.015f,{58,109,124});
    }
    // The two game workboats are moored at the city's slip label points.
    // Their hulls and equipment are from the same model used at the helm.
    const std::array<DeckPoint,2> berths={{{33.687f,143.545f},{31.762f,99.656f}}};
    for(size_t i=0;i<berths.size();++i){
        HelmProgress fittings;
        fittings.hull=i==0?2u:1u;
        fittings.owned=HelmMask(HelmItem::Roof)|HelmMask(HelmItem::RodRacks)|
            HelmMask(HelmItem::Battery)|HelmMask(HelmItem::Outboard);
        const auto boat=BuildBoatMesh(fittings);
        const auto start=uint32_t(mesh.vertices.size());
        for(auto v:boat.vertices){
            const float east=berths[i].east+v.z;
            v.z=berths[i].north-v.x;v.x=east;v.y+=.12f-kBoatDeckY;
            mesh.vertices.push_back(v);
        }
        for(auto index:boat.indices)mesh.indices.push_back(start+index);
    }
    const auto& distance=CrescentDistances();
    // Paint marks and cross-plank seams follow the traversable centerline.
    for(float m=.6f;m<distance.back()-.4f;m+=.8f){
        auto a=CrescentPosition(m),b=CrescentPosition(m+.06f);
        const float l=std::hypot(b.east-a.east,b.north-a.north);
        if(l<.001f)continue;
        const float e=(b.north-a.north)/l,n=-(b.east-a.east)/l;
        auto edge=[&](DeckPoint p,float w){return DeckPoint{p.east+e*w,p.north+n*w};};
        quad(edge(a,-.8f),edge(a,.8f),edge(b,.8f),edge(b,-.8f),.87f,{134,112,85});
        if(int(m*10)%32<8){
            auto end=CrescentPosition(m+.3f);
            quad(edge(a,-.12f),edge(a,.12f),edge(end,.12f),edge(end,-.12f),.89f,{247,204,81});
        }
    }
    const auto mark=CrescentPosition(CrescentCastMark());
    quad({mark.east-.9f,mark.north-.35f},{mark.east+.9f,mark.north-.35f},
         {mark.east+.9f,mark.north+.35f},{mark.east-.9f,mark.north+.35f},.90f,{126,185,248});
    for(const size_t node:{0u,4u,5u}){
        auto p=kCrescentWalk[node];
        quad({p.east-.7f,p.north-.7f},{p.east+.7f,p.north-.7f},
             {p.east+.7f,p.north+.7f},{p.east-.7f,p.north+.7f},.90f,{46,142,133});
    }
}
}} // namespace reverie::sitka
#endif
