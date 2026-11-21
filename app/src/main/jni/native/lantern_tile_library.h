#ifndef REVERIE_LANTERN_TILE_LIBRARY_H
#define REVERIE_LANTERN_TILE_LIBRARY_H
// Immutable, one-time baked 1,000-place world atlas. Source packs are
// packaged APK assets, never generated on an observation or war tick.
// Per-location Warfront assignments and timestamped unit orders are a
// separate state; the atlas holds no faction or report correctness.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>
namespace reverie { namespace lantern { namespace tiles {
constexpr int kLibrarySize=1000,kPackCount=10,kPerPack=100;
constexpr int kGrid=33,kNav=16,kBuildingCount=6,kFoliageCount=48;
constexpr int kAnchorCount=12,kRoadPoints=9,kRecordBytes=2880;
constexpr int kHeaderBytes=32,kPackBytes=kHeaderBytes+kRecordBytes*kPerPack;
constexpr uint32_t kBakingSeed=0x5ac1953bu;
constexpr uint16_t kUnassigned=0xffffu;
enum class TerrainFamily:uint8_t {
    RiverValley,Farmland,RollingHills,ForestedUplands,Marsh,
    Village,IndustrialRail,Mountains,Coastal,Orchard
};
enum class AnchorType:uint8_t {
    RoadEntry=1,RoadExit,RoadCenter,Village,Supply,HighGround,LowGround,
    TreeCover,Ambush,Camp,Reinforce,Observation
};
struct Building {
    uint8_t x=0,y=0,width=0,depth=0,height=0,kind=0,orientation=0,exists=0;
};
struct Foliage {
    uint8_t x=0,y=0,height=0,radius=0,kind=0,age=0;
};
struct Anchor {
    uint8_t x=0,y=0,kind=0,radius=0,orientation=0,flags=0;
};
struct Tile {
    uint16_t id=0;
    uint32_t seed=0;
    uint8_t theater=0,biome=0,elevation_class=0,flags=0;
    uint16_t base_altitude_dm=0;
    uint8_t ground_material=0,road_material=0;
    std::array<uint16_t,kGrid*kGrid> height_dm{};
    std::array<uint8_t,kNav*kNav> nav_cost{};
    std::array<uint8_t,kRoadPoints*2> road{};
    std::array<Building,kBuildingCount> buildings{};
    std::array<Foliage,kFoliageCount> foliage{};
    std::array<Anchor,kAnchorCount> anchors{};
    float RoadY(float x)const{
        const float t=std::max(0.f,std::min(7.999f,x/255.f*8.f));
        const int k=static_cast<int>(t),a=road[2*k+1],b=road[2*k+3];
        return a+(b-a)*(t-k);
    }
    float GroundHeight(float x,float y)const{
        const float px=std::max(0.f,std::min(32.f,x/255.f*32.f));
        const float py=std::max(0.f,std::min(32.f,y/255.f*32.f));
        const int x0=static_cast<int>(px),y0=static_cast<int>(py);
        const int x1=std::min(32,x0+1),y1=std::min(32,y0+1);
        const float fx=px-x0,fy=py-y0;
        const float a=height_dm[y0*33+x0],b=height_dm[y0*33+x1];
        const float c=height_dm[y1*33+x0],d=height_dm[y1*33+x1];
        return ((a*(1.f-fx)+b*fx)*(1.f-fy)+
                (c*(1.f-fx)+d*fx)*fy)*.1f;
    }
    bool Walkable(int x,int y)const{
        return x>=0&&x<kNav&&y>=0&&y<kNav&&nav_cost[y*kNav+x]>0;
    }
};
inline uint16_t Read16(const uint8_t* p){
    return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);
}
inline uint32_t Read32(const uint8_t* p){
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|
        (uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
inline uint32_t Fnv(const uint8_t* data,size_t n){
    uint32_t h=2166136261u;
    for(size_t i=0;i<n;++i)h=(h^data[i])*16777619u;
    return h;
}
inline uint32_t Hash(uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;
    return x^(x>>16);
}
inline bool ReadTile(const uint8_t* bytes,size_t length,uint16_t id,
                     Tile* output){
    if(!bytes||!output||id>=kLibrarySize||length!=kPackBytes||
       std::memcmp(bytes,"LTIL",4)!=0||Read16(bytes+4)!=1||
       Read16(bytes+6)!=kPerPack||Read16(bytes+8)!=kRecordBytes||
       bytes[10]!=kGrid||bytes[11]!=kNav||
       bytes[12]!=kBuildingCount||bytes[13]!=kFoliageCount||
       bytes[14]!=kAnchorCount||bytes[15]!=kRoadPoints||
       Read32(bytes+16)!=kBakingSeed||
       Read32(bytes+24)!=uint32_t((id/kPerPack)*kPerPack)||
       Read32(bytes+28)!=0u||Fnv(bytes+32,length-32)!=Read32(bytes+20))
        return false;
    const size_t start=kHeaderBytes+size_t(id%kPerPack)*kRecordBytes;
    const uint8_t* p=bytes+start;
    if(Read16(p)!=id||Read32(p+2)!=
       Hash(kBakingSeed^((id+1u)*0x9e3779b9u))||
       p[6]>=10||p[7]>=10||p[8]>=6||p[9]>7u||
       Read16(p+10)>65000u||p[14]||p[15]||
       Fnv(p,kRecordBytes-4)!=Read32(p+kRecordBytes-4))
        return false;
    Tile out{};
    out.id=id;out.seed=Read32(p+2);out.theater=p[6];out.biome=p[7];
    out.elevation_class=p[8];out.flags=p[9];
    out.base_altitude_dm=Read16(p+10);
    out.ground_material=p[12];out.road_material=p[13];
    for(size_t i=0;i<out.height_dm.size();++i){
        out.height_dm[i]=Read16(p+16+i*2);
        if(out.height_dm[i]>65000u)return false;
    }
    for(size_t i=0;i<out.nav_cost.size();++i)
        out.nav_cost[i]=p[2194+i];
    for(size_t i=0;i<out.road.size();++i)out.road[i]=p[2450+i];
    for(size_t i=0;i<out.buildings.size();++i){
        const uint8_t* b=p+2468+i*8;
        out.buildings[i]={b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7]};
        if(b[7]>1u||b[2]>32||b[3]>32||b[4]>32)return false;
    }
    for(size_t i=0;i<out.foliage.size();++i){
        const uint8_t* b=p+2516+i*6;
        out.foliage[i]={b[0],b[1],b[2],b[3],b[4],b[5]};
        if(b[4]>6u||b[3]>20u)return false;
    }
    for(size_t i=0;i<out.anchors.size();++i){
        const uint8_t* b=p+2804+i*6;
        out.anchors[i]={b[0],b[1],b[2],b[3],b[4],b[5]};
        if(b[2]<1||b[2]>kAnchorCount)return false;
    }
    *output=std::move(out);
    return true;
}
inline bool FileNameFor(uint16_t id,char* dst,size_t capacity){
    if(!dst||capacity<39u||id>=kLibrarySize)return false;
    constexpr char prefix[]="native/lantern/terrain/tiles-";
    constexpr char suffix[]=".ltpack";
    constexpr size_t start=sizeof(prefix)-1u;
    std::memcpy(dst,prefix,start);
    dst[start]=char('0'+id/100/10);
    dst[start+1]=char('0'+(id/100)%10);
    std::memcpy(dst+start+2,suffix,sizeof(suffix));
    return true;
}
inline std::array<uint16_t,kLibrarySize> AssignWarfront(uint32_t war_seed){
    // One allocation-free permutation when the WAR starts; no new terrain.
    // A visited front location keeps its tile for the entire campaign.
    std::array<uint16_t,kLibrarySize> assigned{};
    for(uint16_t i=0;i<kLibrarySize;++i)assigned[i]=i;
    for(int i=kLibrarySize-1;i>0;--i){
        const uint32_t h=Hash(war_seed^uint32_t(i)*0x9e3779b9u);
        std::swap(assigned[i],assigned[h%uint32_t(i+1)]);
    }
    return assigned;
}
struct Waypoint {uint8_t x=0,y=0;};
inline bool NearestWalkable(const Tile& tile,uint8_t x,uint8_t y,
                           int* cell_x,int* cell_y){
    int best=std::numeric_limits<int>::max(),bx=-1,by=-1;
    for(int yy=0;yy<16;++yy)for(int xx=0;xx<16;++xx){
        if(!tile.Walkable(xx,yy))continue;
        const int dx=int(xx*16+8)-x,dy=int(yy*16+8)-y;
        const int d=dx*dx+dy*dy;
        if(d<best){best=d;bx=xx;by=yy;}
    }
    if(bx<0)return false;
    *cell_x=bx;*cell_y=by;return true;
}
inline bool NavPath(const Tile& tile,uint8_t from,uint8_t to,
                    std::vector<Waypoint>* output){
    if(!output||from>=kAnchorCount||to>=kAnchorCount)return false;
    const Anchor& a=tile.anchors[from],&b=tile.anchors[to];
    int ax=0,ay=0,bx=0,by=0;
    if(!NearestWalkable(tile,a.x,a.y,&ax,&ay)||
       !NearestWalkable(tile,b.x,b.y,&bx,&by))return false;
    const int origin=ay*16+ax,target=by*16+bx;
    std::array<int32_t,256> dist{},parent{};
    std::array<uint8_t,256> used{};
    dist.fill(std::numeric_limits<int32_t>::max());parent.fill(-1);
    dist[origin]=0;
    for(int iter=0;iter<256;++iter){
        int current=-1;
        for(int i=0;i<256;++i)if(!used[i]&&
            (current<0||dist[i]<dist[current]))current=i;
        if(current<0||dist[current]==std::numeric_limits<int32_t>::max())break;
        if(current==target)break;
        used[current]=1u;
        const int x=current%16,y=current/16;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){
            if(!dx&&!dy)continue;
            const int nx=x+dx,ny=y+dy;
            if(!tile.Walkable(nx,ny))continue;
            // No diagonal corner-cutting through impassable terrain.
            if(dx&&dy&&(!tile.Walkable(nx,y)||!tile.Walkable(x,ny)))continue;
            const int n=ny*16+nx;
            const int step=(dx&&dy?14:10)+tile.nav_cost[n];
            if(dist[current]+step<dist[n]){
                dist[n]=dist[current]+step;parent[n]=current;
            }
        }
    }
    if(dist[target]==std::numeric_limits<int32_t>::max())return false;
    std::vector<Waypoint> reversed;
    for(int node=target;node>=0;node=parent[node]){
        reversed.push_back({static_cast<uint8_t>(node%16*16+8),
                            static_cast<uint8_t>(node/16*16+8)});
        if(node==origin)break;
        if(reversed.size()>256u)return false;
    }
    if(reversed.empty())return false;
    std::reverse(reversed.begin(),reversed.end());
    *output=std::move(reversed);
    return true;
}
enum class UnitType:uint8_t {
    Infantry,Truck,Tank,Artillery,CampTent,Supply,FieldGun
};
enum class Activity:uint8_t {Transit,Hold,Camp,Replenish,Conceal};
enum class Doctrine:uint8_t {
    Exposed,UnderFoliage,BehindBuildings,ReverseSlope,Dispersed
};
struct WarOrder {
    uint32_t unit_id=0;
    uint16_t front_location=0;
    UnitType type=UnitType::Infantry;
    Activity activity=Activity::Hold;
    Doctrine doctrine=Doctrine::Exposed;
    int64_t enter_at=0,depart_at=0;
    uint8_t from_anchor=0,to_anchor=1;
    uint8_t strength=1;
    int8_t observer_from_west=1;
};
struct PlacedUnit {
    uint32_t unit_id=0;
    UnitType type=UnitType::Infantry;
    Activity activity=Activity::Hold;
    float x=0.f,y=0.f,ground_z_m=0.f;
    uint8_t strength=1;
};
inline bool ResolveAt(const Tile& tile,uint16_t front_location,
                      int64_t captured_at,const std::vector<WarOrder>& orders,
                      std::vector<PlacedUnit>* out){
    if(!out)return false;
    std::vector<PlacedUnit> result;
    for(const auto& unit:orders){
        if(unit.front_location!=front_location||captured_at<unit.enter_at||
           captured_at>unit.depart_at)continue;
        if(unit.unit_id==0||unit.enter_at>unit.depart_at||
           unit.from_anchor>=kAnchorCount||unit.to_anchor>=kAnchorCount||
           static_cast<unsigned>(unit.type)>static_cast<unsigned>(UnitType::FieldGun)||
           static_cast<unsigned>(unit.activity)>static_cast<unsigned>(Activity::Conceal)||
           static_cast<unsigned>(unit.doctrine)>static_cast<unsigned>(Doctrine::Dispersed))
           return false;
        const auto& start=tile.anchors[unit.from_anchor];
        const auto& end=tile.anchors[unit.to_anchor];
        float x=start.x,y=start.y;
        if(unit.activity==Activity::Transit){
            std::vector<Waypoint> path;
            if(!NavPath(tile,unit.from_anchor,unit.to_anchor,&path))return false;
            const double duration=double(unit.depart_at-unit.enter_at);
            const double fraction=duration<=0?1.:
                std::max(0.,std::min(1.,double(captured_at-unit.enter_at)/duration));
            const double position=(path.size()-1u)*fraction;
            const size_t i=std::min(path.size()-1u,static_cast<size_t>(position));
            const size_t j=std::min(path.size()-1u,i+1);
            const float f=static_cast<float>(position-i);
            x=path[i].x+(path[j].x-path[i].x)*f;
            y=path[i].y+(path[j].y-path[i].y)*f;
        }else if(unit.doctrine==Doctrine::UnderFoliage){
            int best=0x7fffffff;
            for(const auto& plant:tile.foliage){
                if(!plant.kind)continue;
                const int dx=int(plant.x)-start.x,dy=int(plant.y)-start.y;
                const int distance=dx*dx+dy*dy;
                if(distance<best){best=distance;x=plant.x;y=plant.y;}
            }
        }else if(unit.doctrine==Doctrine::BehindBuildings){
            for(const auto& house:tile.buildings)if(house.exists){
                x=std::min(253.f,std::max(2.f,
                    house.x+(unit.observer_from_west>0?1.f:-1.f)*
                    (house.width*.5f+2.f)));
                y=house.y;break;
            }
        }else if(unit.doctrine==Doctrine::ReverseSlope){
            x=tile.anchors[5].x;y=tile.anchors[5].y;
        }
        result.push_back({unit.unit_id,unit.type,unit.activity,
                          x,y,tile.GroundHeight(x,y),unit.strength});
    }
    *out=std::move(result);
    return true;
}
}}} // reverie::lantern::tiles
#endif
