#ifndef REVERIE_OMEGA_FRONT_ACCORD_H
#define REVERIE_OMEGA_FRONT_ACCORD_H
#include "omega_front_intelligence.h"
#include <cstdint>

namespace reverie { namespace omega {

// Diplomatic alternatives to operations 4-6. No invented combat results:
// the frontier's last real battle report is deliberately left untouched.
class Accord {
public:
    static constexpr uint8_t kPermittedMask=0x38u; // operation indices 3,4,5
    uint8_t mask() const {return mask_;}
    int last_operation() const {return last_;}
    bool completed(int operation) const {
        return operation>=3 && operation<=5 && (mask_ & (1u << operation))!=0;
    }
    static uint8_t Faction(int operation) {
        return operation==3?4u:operation==4?2u:operation==5?6u:0u;
    }
    static int Sector(int operation) {
        return operation==3?9:operation==4?7:operation==5?4:-1;
    }
    // Can be executed from a neighboring friendly camp, not a magical
    // instant teleport to occupied territory. The parley is a prerequisite.
    // Resources are checked first, then all mutations are atomic.
    bool Conclude(int operation,int from,int target,bool parleyed,
                  uint8_t legal_owner,int8_t &relation,int &rations,
                  int &salvage,int &trust,uint16_t &wreck,uint8_t &claims){
        if(operation<3||operation>5||completed(operation)||
           target!=Sector(operation)||!Intelligence::Adjacent(from,target)||
           !parleyed||trust<35||claims>100||relation>100||relation<-100)
            return false;
        const int rations_cost=operation==5?4:2;
        const int salvage_cost=operation==4?6:operation==5?3:0;
        const int relation_floor=operation==3?-15:operation==4?-25:0;
        if(relation<relation_floor||rations<rations_cost||salvage<salvage_cost)
            return false;
        if(operation==3){
            // Scrapline grants limited physical salvage rights; it keeps
            // the yard and its unrecovered materials.
            if(legal_owner!=4||wreck<6||salvage>999994)return false;
        }else if(operation==4){
            // Meridian retains its charter gate and receives engineering
            // stock rather than donating a fully equipped checkpoint.
            if(legal_owner!=2||wreck>65529)return false;
        }else{
            // A neutral ford under a Legionary military claim remains
            // neutral. The nearby Ninth Camp remains Lost Ninth property.
            if(legal_owner!=0&&legal_owner!=6)return false;
            if(wreck>65532)return false;
        }
        rations-=rations_cost;
        salvage-=salvage_cost;
        if(operation==3){wreck-=6;salvage+=6;}
        else wreck=static_cast<uint16_t>(wreck+salvage_cost);
        const int delta=operation==3?4:operation==4?5:4;
        relation=static_cast<int8_t>(relation>100-delta?100:relation+delta);
        claims=static_cast<uint8_t>(claims>=7?claims-7:0);
        mask_|=static_cast<uint8_t>(1u<<operation);
        last_=operation;
        return true;
    }
    // A checksum can be recomputed by adversarial input; reject semantic
    // combinations that were impossible in the actual operation sequence.
    bool Restore(uint8_t mask,int last,int progress){
        if(progress<0||progress>6||(mask & ~kPermittedMask)!=0 ||
           (mask && (last<3||last>5)) ||(!mask && last!=-1) ||
           (last>=0 && ((mask & (1u<<last))==0||last>=progress)))return false;
        for(int op=3;op<=5;++op)if((mask & (1u<<op)) && op>=progress)return false;
        for(int op=5;op>last;--op)if(mask & (1u<<op))return false;
        if(mask && (mask & (1u<<last))==0)return false;
        mask_=mask;last_=last;return true;
    }
private:
    uint8_t mask_=0;
    int last_=-1;
};

}} // namespace reverie::omega
#endif
