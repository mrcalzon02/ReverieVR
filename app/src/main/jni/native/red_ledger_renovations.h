#ifndef REVERIE_RED_LEDGER_RENOVATIONS_H
#define REVERIE_RED_LEDGER_RENOVATIONS_H
#include "red_ledger_simulation.h"
#include "red_ledger_trade.h"
#include <array>
#include <cstdint>
#include <cstddef>
namespace reverie { namespace redledger { namespace property {
// Five purchased levels across the source design's eight property categories.
// Amounts are provisional in-game cents; Unreal GDD $ amounts are not cents.
enum Category : uint8_t { Walls, Floor, Ceiling, Seating, Draft, Bottles,
                          Spirits, Snacks, Count };
constexpr const char* kNames[Count]={
    "WALLS","FLOOR","CEILING","SEATING",
    "DRAFT STORAGE","BOTTLE STORAGE","SPIRIT STORAGE","SNACK STORAGE"
};
class Renovations {
public:
    static constexpr size_t kSerializedSize=20u;
    int category() const {return selected_;}
    int tier(int category)const {
        return category>=0&&category<Count?tiers_[static_cast<size_t>(category)]:0;
    }
    bool next(){selected_=static_cast<uint8_t>((selected_+1u)%Count);return true;}
    int cost(const trade::Trade& market) const {
        const int level=tier(selected_);
        if(level>=5)return 0;
        constexpr int kTierCosts[5]={1600,3200,4800,6400,8000};
        const int sku=market.offer();
        const int faction=sku==0?-1:sku<trade::kDrinks?
            trade::kDrinkCatalog[sku].faction:
            trade::kSnackCatalog[sku-trade::kDrinks].faction;
        const int favor=faction>=0?market.favor(faction):0;
        // Five successive discount thresholds: game balance, not GDD rates.
        const int discount=favor>=50?25:favor>=35?20:favor>=20?15:
                           favor>=10?10:favor>=5?5:0;
        return (kTierCosts[level]*(100-discount)+99)/100;
    }
    bool buy(Simulation* simulation,const trade::Trade& market){
        if(!simulation||tier(selected_)>=5)return false;
        Simulation staged=*simulation;
        if(!staged.SpendSpecialStock(cost(market)))return false;
        *simulation=staged;
        ++tiers_[selected_];
        return true;
    }
    int service_premium(int drink,int snack)const {
        if(drink<0||drink>=trade::kDrinks||snack<-1||snack>=trade::kSnacks)
            return 0;
        const int comfort=tier(Walls)+tier(Floor)+tier(Ceiling)+tier(Seating);
        const int category=drink==0||((drink-1)%3)==0?Draft:
                           ((drink-1)%3)==1?Bottles:Spirits;
        // Existing checked bar payments must never exceed 1000 cents:
        // base patron <=330, existing trade premium <=465, renovation <=200.
        return (comfort*8+tier(category)*16+
               (snack>=0?tier(Snacks)*10:0))>200?200:
               (comfort*8+tier(category)*16+
               (snack>=0?tier(Snacks)*10:0));
    }
    int reputation_bonus()const {
        const int comfort=tier(Walls)+tier(Floor)+tier(Ceiling)+tier(Seating);
        return comfort>=16?2:comfort>=8?1:0;
    }
    bool serialize(uint8_t* out,size_t size)const{
        if(!out||size<kSerializedSize)return false;
        out[0]='R';out[1]='E';out[2]='N';out[3]='1';
        for(int i=0;i<Count;++i)out[4+i]=tiers_[i];
        out[12]=selected_;out[13]=out[14]=out[15]=0;
        uint32_t hash=2166136261u;
        for(size_t i=0;i<16u;++i)hash=(hash^out[i])*16777619u;
        for(int i=0;i<4;++i)out[16+i]=static_cast<uint8_t>(hash>>(i*8));
        return true;
    }
    bool deserialize(const uint8_t* bytes,size_t size){
        if(!bytes||size!=kSerializedSize||
           bytes[0]!='R'||bytes[1]!='E'||bytes[2]!='N'||bytes[3]!='1'||
           bytes[12]>=Count||bytes[13]||bytes[14]||bytes[15])return false;
        uint32_t hash=2166136261u;
        for(size_t i=0;i<16u;++i)hash=(hash^bytes[i])*16777619u;
        for(int i=0;i<4;++i)
            if(bytes[16+i]!=static_cast<uint8_t>(hash>>(i*8)))return false;
        Renovations copy;
        for(int i=0;i<Count;++i){
            if(bytes[4+i]>5u)return false;
            copy.tiers_[i]=bytes[4+i];
        }
        copy.selected_=bytes[12];
        *this=copy;return true;
    }
private:
    std::array<uint8_t,Count> tiers_{};
    uint8_t selected_=0;
};
}}}
#endif
