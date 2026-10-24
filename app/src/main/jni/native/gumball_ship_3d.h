#ifndef REVERIE_GUMBALL_SHIP_3D_H
#define REVERIE_GUMBALL_SHIP_3D_H
#include "gumball_shipyard.h"
#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
namespace reverie { namespace gumball {
constexpr int kShipDepth=5;
constexpr int kShipVoxels=kShipSize*kShipSize*kShipDepth;
struct Ship3DSave {
    uint32_t magic=0x33424853u,version=2u; // SHB3; independent of V1 72-byte save
    uint8_t cells[kShipVoxels]{};
    uint8_t stock[kPartKinds]{};
    uint32_t checksum=0;
};
static_assert(sizeof(Ship3DSave)==268,"3D ship save layout");
inline uint32_t Ship3DChecksum(const Ship3DSave& s){
    uint32_t h=2166136261u;
    auto mix=[&h](uint8_t v){h=(h^v)*16777619u;};
    for(unsigned i=0;i<4;++i)mix(static_cast<uint8_t>(s.magic>>(i*8)));
    for(unsigned i=0;i<4;++i)mix(static_cast<uint8_t>(s.version>>(i*8)));
    for(uint8_t v:s.cells)mix(v);
    for(uint8_t v:s.stock)mix(v);
    return h;
}
class ShipAssembly {
public:
    ShipAssembly(){import_legacy(Shipyard{});}
    static bool inside(int x,int y,int z){
        return Shipyard::inside(x,y)&&z>=0&&z<kShipDepth;
    }
    Part at(int x,int y,int z)const{
        return inside(x,y,z)?static_cast<Part>(cells_[index(x,y,z)]):Part::Empty;
    }
    unsigned stock(Part p)const{
        return static_cast<unsigned>(p)<kPartKinds?stock_[static_cast<unsigned>(p)]:0u;
    }
    int count(Part p)const{
        int result=0;
        for(uint8_t v:cells_)if(v==static_cast<uint8_t>(p))++result;
        return result;
    }
    // Authorized conversion of a checked 2D v1 lattice to the middle Z slice.
    void import_legacy(const Shipyard& legacy){
        cells_.fill(0);
        for(int y=0;y<kShipSize;++y)for(int x=0;x<kShipSize;++x)
            cells_[index(x,y,2)]=static_cast<uint8_t>(legacy.at(x,y));
        for(unsigned i=0;i<kPartKinds;++i)
            stock_[i]=static_cast<uint8_t>(legacy.stock(static_cast<Part>(i)));
    }
    ShipEdit edit(int x,int y,int z,Part selected){
        if(!inside(x,y,z)||static_cast<unsigned>(selected)<=1u||
           static_cast<unsigned>(selected)>=kPartKinds)return ShipEdit::Invalid;
        const Part old=at(x,y,z);
        if(old==Part::Core)return ShipEdit::CoreLocked;
        const int target=index(x,y,z);
        if(old==selected){
            cells_[target]=0;
            if(!connected()){
                cells_[target]=static_cast<uint8_t>(old);
                return ShipEdit::NotConnected;
            }
            ++stock_[static_cast<unsigned>(old)];
            return ShipEdit::Removed;
        }
        if(stock(selected)==0u)return ShipEdit::NoStock;
        cells_[target]=static_cast<uint8_t>(selected);
        if(!connected()){
            cells_[target]=static_cast<uint8_t>(old);
            return ShipEdit::NotConnected;
        }
        --stock_[static_cast<unsigned>(selected)];
        if(old!=Part::Empty)++stock_[static_cast<unsigned>(old)];
        return old==Part::Empty?ShipEdit::Placed:ShipEdit::Replaced;
    }
    bool connected()const{
        if(at(3,3,2)!=Part::Core||count(Part::Core)!=1)return false;
        std::array<uint8_t,kShipVoxels> seen{};
        std::array<int,kShipVoxels> queue{};
        int next=0,front=0;
        const int root=index(3,3,2);
        queue[next++]=root;seen[root]=1;
        while(front<next){
            const int current=queue[front++];
            const int z=current/49,y=(current%49)/7,x=current%7;
            constexpr int dx[]={1,-1,0,0,0,0};
            constexpr int dy[]={0,0,1,-1,0,0};
            constexpr int dz[]={0,0,0,0,1,-1};
            for(int d=0;d<6;++d){
                const int nx=x+dx[d],ny=y+dy[d],nz=z+dz[d];
                if(!inside(nx,ny,nz))continue;
                const int i=index(nx,ny,nz);
                if(cells_[i]!=0&&!seen[i]){seen[i]=1;queue[next++]=i;}
            }
        }
        for(int i=0;i<kShipVoxels;++i)
            if(cells_[i]&&!seen[i])return false;
        return true;
    }
    ShipProfile profile()const{
        ShipProfile p{};p.mounts=0;
        for(int z=0;z<kShipDepth;++z)
        for(int y=0;y<kShipSize;++y)for(int x=0;x<kShipSize;++x){
            switch(at(x,y,z)){
            case Part::Core:p.mass+=3;break;
            case Part::Frame:p.mass+=1;p.hull+=3;break;
            case Part::Armor:p.mass+=3;p.hull+=14;break;
            case Part::Wing:p.mass+=1;p.speed+=.025f;break;
            case Part::Thruster:p.mass+=2;p.speed+=.16f;p.demand+=1;break;
            case Part::Reactor:p.mass+=3;p.power+=8;break;
            case Part::Battery:p.mass+=2;p.power+=3;break;
            case Part::Shield:p.mass+=2;p.shield+=12;p.demand+=2;break;
            case Part::Mount:
                p.mass+=2;p.demand+=2;
                if(p.mounts<2){
                    p.muzzle_x[p.mounts]=(x-3)*.062f;
                    p.muzzle_y[p.mounts]=(3-y)*.061f;
                    ++p.mounts;
                }break;
            case Part::Cargo:p.mass+=2;p.pickup_radius+=.045f;p.demand+=1;break;
            default:break;
            }
        }
        p.hull=std::max(0,p.hull-3);
        p.shield=std::max(0,p.shield-12);
        p.speed=std::max(-.20f,p.speed-.16f-(p.mass-13)*.012f);
        return p;
    }
    bool ready()const{
        if(!connected()||count(Part::Thruster)<1||
           count(Part::Reactor)<1||count(Part::Mount)<1)return false;
        const ShipProfile p=profile();
        return p.power>=p.demand&&p.mass<=18*count(Part::Thruster);
    }
    Ship3DSave snapshot()const{
        Ship3DSave data;
        std::memcpy(data.cells,cells_.data(),kShipVoxels);
        std::memcpy(data.stock,stock_.data(),kPartKinds);
        data.checksum=Ship3DChecksum(data);
        return data;
    }
    bool restore(const Ship3DSave& data){
        if(data.magic!=0x33424853u||data.version!=2u||
           data.checksum!=Ship3DChecksum(data))return false;
        for(uint8_t v:data.cells)if(v>=kPartKinds)return false;
        ShipAssembly candidate=*this;
        std::memcpy(candidate.cells_.data(),data.cells,kShipVoxels);
        std::memcpy(candidate.stock_.data(),data.stock,kPartKinds);
        if(!candidate.connected()||candidate.stock_[0]!=0||candidate.stock_[1]!=0)return false;
        const Shipyard original;
        for(unsigned i=2;i<kPartKinds;++i){
            const Part part=static_cast<Part>(i);
            if(candidate.count(part)+static_cast<int>(candidate.stock_[i])!=
               original.count(part)+static_cast<int>(original.stock(part)))return false;
        }
        *this=candidate;return true;
    }
    // Enemies and the commander share the SAME voxel assembly vocabulary.
    // Deterministic factory patterns are legal connected volume hulls, not
    // different procedural sprites pretending to be constructed ships.
    static ShipAssembly enemy(unsigned kind,uint32_t salt){
        ShipAssembly ship;
        ship.cells_.fill(0);ship.stock_.fill(0);
        const auto add=[&ship](int x,int y,int z,Part p){
            if(inside(x,y,z))ship.cells_[index(x,y,z)]=static_cast<uint8_t>(p);
        };
        add(3,3,2,Part::Core);
        add(3,2,2,Part::Frame);
        add(3,1,2,Part::Mount);
        add(3,4,2,Part::Thruster);
        add(4,3,2,Part::Reactor);
        add(2,3,2,Part::Frame);
        add(1,3,2,(kind%3u)==0u?Part::Armor:Part::Wing);
        add(5,3,2,(kind%3u)==0u?Part::Armor:Part::Wing);
        if(kind%2u){add(3,3,3,Part::Shield);add(3,3,1,Part::Battery);}
        if((kind+salt)%3u==0u){add(3,2,3,Part::Armor);add(4,2,2,Part::Cargo);}
        if(kind>=4u){add(2,2,2,Part::Mount);add(3,4,3,Part::Armor);}
        // Enemy factories obey the exact same launch gates as the player.
        // Reinforce heavier variants with a connected second thruster and
        // optional reactor supply rather than granting free physics.
        if(!ship.ready())add(3,5,2,Part::Thruster);
        if(!ship.ready())add(4,3,3,Part::Battery);
        return ship;
    }
private:
    static int index(int x,int y,int z){return z*49+y*7+x;}
    std::array<uint8_t,kShipVoxels> cells_{};
    std::array<uint8_t,kPartKinds> stock_{};
};
inline void AppendAssemblyMesh(std::vector<float>& mesh,const ShipAssembly& ship,
                               float ox,float oy,float oz,float scale,float yaw,
                               float tint=1.f){
    const float ca=std::cos(yaw),sa=std::sin(yaw);
    constexpr float palette[kPartKinds][3]={
        {0,0,0},{.98f,.72f,.90f},{.56f,.29f,.32f},{.96f,.72f,.32f},
        {.54f,.92f,.43f},{.24f,.92f,.88f},{1.f,.61f,.22f},
        {.99f,.88f,.31f},{.29f,.67f,1.f},{1.f,.30f,.45f},{.63f,.80f,.98f}};
    constexpr int dx[]={0,0,1,-1,0,0};
    constexpr int dy[]={0,0,0,0,1,-1};
    constexpr int dz[]={1,-1,0,0,0,0};
    // Cube face corners, arranged counter-clockwise from the outside.
    constexpr int face[6][4][3]={
        {{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}},
        {{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}},
        {{-1,-1,-1},{-1,-1,1},{-1,1,1},{-1,1,-1}},
        {{-1,1,1},{1,1,1},{1,1,-1},{-1,1,-1}},
        {{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}}
    };
    constexpr float light[]={1.f,.52f,.69f,.83f,.91f,.57f};
    // Painter ordering follows yaw without clearing the shell's depth.
    // This is an interim GLES2 draw order, not a fully depth-tested pass.
    const int faceOrder[]={ca>=0.f?1:0,
                           sa>=0.f?2:3,4,5,
                           sa>=0.f?3:2,ca>=0.f?0:1};
    for(int zi=0;zi<kShipDepth;++zi)
    for(int y=0;y<kShipSize;++y)
    for(int xi=0;xi<kShipSize;++xi){
        const int z=ca>=0.f?zi:kShipDepth-1-zi;
        const int x=sa>=0.f?kShipSize-1-xi:xi;
        const Part p=ship.at(x,y,z);
        if(p==Part::Empty)continue;
        const unsigned type=static_cast<unsigned>(p);
        const float half=(p==Part::Wing?.38f:p==Part::Mount?.35f:.46f);
        for(int faceIndex:faceOrder){
            const int f=faceIndex;
            if(ship.at(x+dx[f],y+dy[f],z+dz[f])!=Part::Empty)continue;
            const float shade=light[f]*tint;
            const int triangles[]={0,1,2,0,2,3};
            for(int t:triangles){
                const float localX=(x-3)+half*face[f][t][0];
                const float localY=(3-y)+half*face[f][t][1];
                const float localZ=(z-2)+half*face[f][t][2];
                mesh.push_back(ox+scale*(localX*ca+localZ*sa));
                mesh.push_back(oy+scale*localY);
                mesh.push_back(oz+scale*(localZ*ca-localX*sa));
                for(float component:palette[type])
                    mesh.push_back(std::min(1.f,component*shade));
            }
        }
    }
}
}}
#endif
