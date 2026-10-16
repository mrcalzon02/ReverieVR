#ifndef REVERIE_OMEGA_FRONT_INTELLIGENCE_H
#define REVERIE_OMEGA_FRONT_INTELLIGENCE_H
#include <array>
#include <cstdint>

namespace reverie { namespace omega {

// Compact first-theater intelligence. No heaps, clocks, floating-point RNG,
// or hidden map re-rolls. Intel never confers property ownership or victory.
class Intelligence {
public:
    static constexpr int kCells=16;
    static constexpr int kFactions=17;
    static constexpr uint8_t Unknown=0, Terrain=1, Surveyed=2;
    Intelligence(){ObserveArrival(6);}
    static bool Valid(int index){return index>=0 && index<kCells;}
    static bool Adjacent(int from,int to){
        return Valid(from) && Valid(to) &&
            ((from%4==to%4 && (from/4-to/4==1 || to/4-from/4==1)) ||
             (from/4==to/4 && (from%4-to%4==1 || to%4-from%4==1)));
    }
    uint8_t level(int sector) const{return Valid(sector)?level_[sector]:Unknown;}
    bool parleyed(int sector) const{return Valid(sector)&&parleyed_[sector]!=0;}
    void ObserveArrival(int sector){
        if(!Valid(sector))return;
        level_[sector]=Surveyed;
        for(int j=0;j<kCells;++j)if(Adjacent(sector,j) && level_[j]==Unknown)
            level_[j]=Terrain;
    }
    bool Survey(int from,int target,int &rations){
        if(!Adjacent(from,target)||level_[target]==Surveyed || rations<2)return false;
        rations-=2;
        level_[target]=Surveyed;
        return true;
    }
    // A negotiated one-time concession: Hearthwell spends 4 actual salvage
    // and one ration. The faction receives the salvage in local stores, its
    // claim pressure decreases, and bilateral relations improve. This does
    // not complete a mission, take a sector, or create wreck mass from nothing.
    bool Parley(int from,int target,uint8_t owner,int &rations,int &salvage,
                int &trust,int8_t &relation,uint16_t &local_wreck,uint8_t &claims){
        if(!Adjacent(from,target)||owner<1||owner>kFactions ||
           level_[target]!=Surveyed || parleyed_[target] || rations<1 ||
           salvage<4 || trust<20 || relation<=-70 || local_wreck>65531)return false;
        --rations;salvage-=4;
        local_wreck=static_cast<uint16_t>(local_wreck+4);
        trust-=1;
        relation=static_cast<int8_t>(relation>92?100:relation+8);
        claims=static_cast<uint8_t>(claims>=6?claims-6:0);
        parleyed_[target]=1;
        return true;
    }
    bool Restore(const std::array<uint8_t,kCells> &levels,
                 const std::array<uint8_t,kCells> &talks,int current){
        if(!Valid(current)||levels[current]!=Surveyed)return false;
        for(int i=0;i<kCells;++i){
            if(levels[i]>Surveyed || talks[i]>1 || (talks[i] && levels[i]!=Surveyed))
                return false;
        }
        level_=levels;parleyed_=talks;
        return true;
    }
    const std::array<uint8_t,kCells>& levels() const{return level_;}
    const std::array<uint8_t,kCells>& talks() const{return parleyed_;}
private:
    std::array<uint8_t,kCells> level_{};
    std::array<uint8_t,kCells> parleyed_{};
};

}} // namespace reverie::omega
#endif
