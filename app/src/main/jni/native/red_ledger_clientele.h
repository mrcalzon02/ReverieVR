#ifndef REVERIE_RED_LEDGER_CLIENTELE_H
#define REVERIE_RED_LEDGER_CLIENTELE_H
#include "red_ledger_trade.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace reverie { namespace redledger { namespace clientele {
// Provisional author-requested service/cleanliness progression. No trash,
// cleaning minigame or cosmetic test: each successful sale changes the venue
// and builds a durable supplier-specific clientele history.
class State {
public:
    static constexpr size_t kSerializedSize=40u;
    int32_t cleanliness()const{return cleanliness_;}
    int32_t services()const{return services_;}
    int32_t affinity(int faction)const{
        return faction>=0&&faction<trade::kFactions?affinity_[faction]:0;
    }
    bool comfortable(PatronKind patron)const{
        return trade::AcceptsCleanliness(patron,cleanliness_);
    }
    // Returns bounded reputation premium to apply BEFORE the committed sale.
    // Customers below cleanliness expectations still pay; their repeat-
    // clientele reputation bonus is reduced, not their already-taken cash.
    int reputation_premium(PatronKind patron,int raw)const{
        return comfortable(patron)?raw:0;
    }
    void served(PatronKind patron,int drink,int snack,int comfort_tier){
        if(services_<1000000)++services_;
        cleanliness_=trade::CleanlinessAfterService(
            cleanliness_,patron,comfort_tier);
        const int faction=trade::FactionFor(patron);
        if(faction<0||faction>=trade::kFactions)return;
        const bool liked=drink==trade::FavoriteDrink(patron)
            ||snack==trade::FavoriteSnack(patron);
        const int change=liked?2:-1;
        int32_t &value=affinity_[static_cast<size_t>(faction)];
        value=value+change>100000?100000:
              value+change< -100000?-100000:value+change;
    }
    // Real paid interiors provide modest between-day recovery; zero tiers
    // do not magically clean a poor starter bar.
    void next_day(int total_comfort_tiers){
        const int tiers=total_comfort_tiers<0?0:
            total_comfort_tiers>20?20:total_comfort_tiers;
        cleanliness_=trade::ClampCleanliness(cleanliness_+tiers);
    }
    bool serialize(uint8_t* out,size_t size)const{
        if(!out||size<kSerializedSize)return false;
        out[0]='C';out[1]='L';out[2]='I';out[3]='1';
        const int32_t values[8]={
            cleanliness_,services_,
            affinity_[0],affinity_[1],affinity_[2],
            affinity_[3],affinity_[4],affinity_[5]};
        for(size_t i=0;i<8;++i)
            for(int b=0;b<4;++b)
                out[4u+i*4u+static_cast<size_t>(b)]=
                    static_cast<uint8_t>(static_cast<uint32_t>(values[i])>>(b*8));
        uint32_t digest=2166136261u;
        for(size_t i=0;i<36u;++i)digest=(digest^out[i])*16777619u;
        for(int b=0;b<4;++b)
            out[36u+static_cast<size_t>(b)]=static_cast<uint8_t>(digest>>(b*8));
        return true;
    }
    bool deserialize(const uint8_t* data,size_t size){
        if(!data||size!=kSerializedSize||
           data[0]!='C'||data[1]!='L'||data[2]!='I'||data[3]!='1')return false;
        uint32_t digest=2166136261u;
        for(size_t i=0;i<36u;++i)digest=(digest^data[i])*16777619u;
        for(int b=0;b<4;++b)
            if(data[36u+static_cast<size_t>(b)]!=static_cast<uint8_t>(digest>>(b*8)))
                return false;
        int32_t values[8]={};
        for(size_t i=0;i<8;++i){
            uint32_t raw=0;
            for(int b=0;b<4;++b)
                raw|=static_cast<uint32_t>(data[4u+i*4u+static_cast<size_t>(b)])<<(b*8);
            values[i]=static_cast<int32_t>(raw);
        }
        if(values[0]<0||values[0]>100||values[1]<0||values[1]>1000000)
            return false;
        for(size_t i=2;i<8;++i)
            if(values[i]<-100000||values[i]>100000)return false;
        State candidate;
        candidate.cleanliness_=values[0];
        candidate.services_=values[1];
        for(int i=0;i<trade::kFactions;++i)
            candidate.affinity_[static_cast<size_t>(i)]=values[i+2];
        *this=candidate;
        return true;
    }
private:
    int32_t cleanliness_=trade::StartingCleanliness();
    int32_t services_=0;
    std::array<int32_t,trade::kFactions> affinity_{};
};
}}}
#endif
