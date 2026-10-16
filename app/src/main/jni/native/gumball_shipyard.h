#ifndef REVERIE_GUMBALL_SHIPYARD_H
#define REVERIE_GUMBALL_SHIPYARD_H
#include <array>
#include <cstdint>
#include <algorithm>
namespace reverie { namespace gumball {
enum class Part : uint8_t { Empty, Core, Frame, Armor, Wing, Thruster, Reactor, Battery, Shield, Mount, Cargo };
enum class ShipEdit : uint8_t {
    Placed, Replaced, Removed, CoreLocked, NoStock, NotConnected, Invalid
};
inline const char* ShipEditMessage(ShipEdit result){
    switch(result){
    case ShipEdit::Placed:return "PART PLACED";
    case ShipEdit::Replaced:return "PART REPLACED";
    case ShipEdit::Removed:return "PART REMOVED";
    case ShipEdit::CoreLocked:return "CORE IS FIXED";
    case ShipEdit::NoStock:return "PART OUT OF STOCK";
    case ShipEdit::NotConnected:return "MUST CONNECT TO SHIP";
    default:return "INVALID CELL";
    }
}
constexpr int kShipSize=7;
constexpr int kPartKinds=11;
struct ShipProfile { int hull=0,shield=0; float speed=0.f,pickup_radius=.23f; int mass=0,power=0,demand=0,mounts=1; std::array<float,2> muzzle_x{},muzzle_y{}; };
struct ShipyardSave {
    uint32_t magic=0x53485931u,version=1u;
    uint8_t cells[49]={};
    uint8_t stock[kPartKinds]={};
    uint32_t checksum=0;
};
static_assert(sizeof(ShipyardSave)==72,"Shipyard save must preserve portable 72-byte layout");
inline uint32_t ShipyardChecksum(const ShipyardSave& s){
    uint32_t h=2166136261u;
    auto mix=[&h](uint8_t b){h=(h^b)*16777619u;};
    for(unsigned i=0;i<4;++i)mix(static_cast<uint8_t>((s.magic>>(8u*i))&255u));
    for(unsigned i=0;i<4;++i)mix(static_cast<uint8_t>((s.version>>(8u*i))&255u));
    for(uint8_t b:s.cells)mix(b);
    for(uint8_t b:s.stock)mix(b);
    return h;
}
class Shipyard {
public:
    Shipyard(){
        stock_={0,0,8,3,2,2,1,2,2,2,2};
        cells_[index(3,3)]=static_cast<uint8_t>(Part::Core);
        cells_[index(3,2)]=static_cast<uint8_t>(Part::Frame);
        cells_[index(3,1)]=static_cast<uint8_t>(Part::Thruster);
        cells_[index(4,3)]=static_cast<uint8_t>(Part::Reactor);
        cells_[index(2,3)]=static_cast<uint8_t>(Part::Mount);
        cells_[index(3,4)]=static_cast<uint8_t>(Part::Shield);
        stock_[static_cast<unsigned>(Part::Frame)]--;
        stock_[static_cast<unsigned>(Part::Thruster)]--;
        stock_[static_cast<unsigned>(Part::Reactor)]--;
        stock_[static_cast<unsigned>(Part::Mount)]--;
        stock_[static_cast<unsigned>(Part::Shield)]--;
    }
    static bool inside(int x,int y){return x>=0&&x<kShipSize&&y>=0&&y<kShipSize;}
    Part at(int x,int y)const{return inside(x,y)?static_cast<Part>(cells_[index(x,y)]):Part::Empty;}
    unsigned stock(Part p)const{return static_cast<unsigned>(p)<kPartKinds?stock_[static_cast<unsigned>(p)]:0u;}
    int count(Part p)const{int n=0;for(uint8_t b:cells_)if(b==static_cast<uint8_t>(p))++n;return n;}
    bool place(int x,int y,Part p){
        const unsigned id=static_cast<unsigned>(p);
        if(!inside(x,y)||id<=1u||id>=kPartKinds||!stock_[id]||at(x,y)!=Part::Empty)return false;
        cells_[index(x,y)]=static_cast<uint8_t>(p);
        if(!connected()){cells_[index(x,y)]=0;return false;}
        --stock_[id];return true;
    }
    bool remove(int x,int y){
        if(!inside(x,y))return false;
        const Part p=at(x,y);
        if(p==Part::Empty||p==Part::Core)return false;
        cells_[index(x,y)]=0;
        if(!connected()){cells_[index(x,y)]=static_cast<uint8_t>(p);return false;}
        ++stock_[static_cast<unsigned>(p)];return true;
    }
    // One click changes an existing fitted part to the selected brush.
    // Previously the first click only removed it, which made the editor
    // appear unable to change parts. Replacement is atomic: no intermediate
    // disconnected ship or duplicated stock can be saved.
    ShipEdit edit(int x,int y,Part selected){
        if(!inside(x,y)||static_cast<unsigned>(selected)<=1u
            ||static_cast<unsigned>(selected)>=kPartKinds)return ShipEdit::Invalid;
        const Part before=at(x,y);
        if(before==Part::Core)return ShipEdit::CoreLocked;
        if(before==selected){
            return remove(x,y)?ShipEdit::Removed:ShipEdit::NotConnected;
        }
        if(stock(selected)==0u)return ShipEdit::NoStock;
        if(before==Part::Empty){
            return place(x,y,selected)?ShipEdit::Placed:ShipEdit::NotConnected;
        }
        const unsigned oldId=static_cast<unsigned>(before);
        const unsigned newId=static_cast<unsigned>(selected);
        cells_[index(x,y)]=static_cast<uint8_t>(selected);
        --stock_[newId];
        ++stock_[oldId];
        return ShipEdit::Replaced;
    }
    bool connected()const{
        std::array<uint8_t,49> seen{};
        std::array<int,49> work{};
        int front=0,back=0;
        const int root=index(3,3);
        if(cells_[root]!=static_cast<uint8_t>(Part::Core))return false;
        seen[root]=1;work[back++]=root;
        while(front<back){
            const int cell=work[front++],x=cell%7,y=cell/7;
            const int dx[]={-1,1,0,0},dy[]={0,0,-1,1};
            for(int dir=0;dir<4;++dir){
                const int nx=x+dx[dir],ny=y+dy[dir];
                if(inside(nx,ny)){
                    const int j=index(nx,ny);
                    if(cells_[j]&&!seen[j]){seen[j]=1;work[back++]=j;}
                }
            }
        }
        for(int j=0;j<49;++j)if(cells_[j]&&!seen[j])return false;
        return true;
    }
    ShipProfile profile()const{
        ShipProfile p{};p.mounts=0;
        for(int i=0;i<49;++i){
            const uint8_t b=cells_[i];
            switch(static_cast<Part>(b)){
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
                    p.muzzle_x[p.mounts]=(i%7-3)*.062f;
                    p.muzzle_y[p.mounts]=(3-i/7)*.061f;
                    ++p.mounts;
                }
                break;
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
        if(!connected()||count(Part::Core)!=1||count(Part::Thruster)<1||
            count(Part::Reactor)<1||count(Part::Mount)<1)return false;
        const auto p=profile();
        return p.power>=p.demand&&p.mass<=18*count(Part::Thruster);
    }
    ShipyardSave snapshot()const{
        ShipyardSave s;
        for(unsigned i=0;i<49;++i)s.cells[i]=cells_[i];
        for(unsigned i=0;i<kPartKinds;++i)s.stock[i]=stock_[i];
        s.checksum=ShipyardChecksum(s);return s;
    }
    bool restore(const ShipyardSave& s){
        if(s.magic!=0x53485931u||s.version!=1u||s.checksum!=ShipyardChecksum(s))return false;
        for(uint8_t cell:s.cells)if(cell>=kPartKinds)return false;
        Shipyard candidate=*this;
        for(unsigned i=0;i<49;++i)candidate.cells_[i]=s.cells[i];
        for(unsigned i=0;i<kPartKinds;++i)candidate.stock_[i]=s.stock[i];
        if(candidate.count(Part::Core)!=1||candidate.stock_[0]!=0||candidate.stock_[1]!=0||
            !candidate.connected())return false;
        const Shipyard starter;
        for(unsigned i=1;i<kPartKinds;++i){
            const Part part=static_cast<Part>(i);
            if(candidate.count(part)+static_cast<int>(candidate.stock_[i])!=starter.count(part)+static_cast<int>(starter.stock(part)))return false;
        }
        *this=candidate;return true;
    }
private:
    static int index(int x,int y){return y*7+x;}
    std::array<uint8_t,49> cells_{};
    std::array<uint8_t,kPartKinds> stock_{};
};
}}
#endif
