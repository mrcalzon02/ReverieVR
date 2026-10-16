#ifndef REVERIE_OMEGA_FRONT_BORDER_H
#define REVERIE_OMEGA_FRONT_BORDER_H

#include "omega_front_obligations.h"
#include <algorithm>
#include <cstdint>

namespace reverie { namespace omega {

// A first-theater response to entering a border after defaulting on its
// treaty. The route stays passable: this is an actual finite confiscation and
// political consequence, not a new permanent progression softlock.
class BorderPatrol {
public:
    static constexpr uint8_t kMask=Accord::kPermittedMask;
    uint8_t charged_mask() const {return charged_;}
    uint8_t grandfathered_mask() const {return grandfathered_;}
    int last_operation() const {return last_;}
    bool inspected(int operation) const {
        return Valid(operation) && (charged_ & (1u<<operation))!=0;
    }
    // Called on real entry only. Any ration seized is credited to the local
    // sector's existing food stores; shortage does not invent a delivery.
    bool Enforce(int operation,int from,int destination,
                 Obligations::Status status,int &rations,int &trust,
                 int8_t &relation,uint16_t &food,uint8_t &claims) {
        if(!Valid(operation) || destination!=Accord::Sector(operation) ||
           !Intelligence::Adjacent(from,destination) ||
           status!=Obligations::Broken ||
           ((charged_|grandfathered_) & (1u<<operation))!=0 ||
           rations<0 || trust<0 || trust>100 ||
           relation< -100 || relation>100 || claims>100) return false;
        const int capacity=static_cast<int>(UINT16_MAX)-food;
        const int seized=std::min({2,rations,capacity});
        rations-=seized;
        food=static_cast<uint16_t>(food+seized);
        relation=static_cast<int8_t>(std::max(-100,static_cast<int>(relation)-4));
        trust=std::max(0,trust-2);
        claims=static_cast<uint8_t>(std::min(100,static_cast<int>(claims)+8));
        charged_|=static_cast<uint8_t>(1u<<operation);
        last_=operation;
        return true;
    }
    // Prior schema v5 recorded defaults but had no border seizures.
    // Grandfather old broken obligations rather than charging retroactively.
    void Migrate(const Obligations &debts) {
        for(int op=3;op<=5;++op)
            if(debts.status(op)==Obligations::Broken)
                grandfathered_|=static_cast<uint8_t>(1u<<op);
    }
    bool Restore(uint8_t charged,uint8_t grandfathered,int last,
                 const Obligations &debts) {
        if(((charged|grandfathered)&~kMask)!=0 ||
           (charged&grandfathered)!=0 ||
           (last< -1 || last>5) ||
           (charged==0 ? last!=-1 : !Valid(last) ||
              (charged&(1u<<last))==0)) return false;
        for(int op=3;op<=5;++op)
            if(((charged|grandfathered)&(1u<<op)) &&
               debts.status(op)!=Obligations::Broken) return false;
        charged_=charged;grandfathered_=grandfathered;last_=last;
        return true;
    }
private:
    static bool Valid(int op){return op>=3 && op<=5;}
    uint8_t charged_=0,grandfathered_=0;
    int last_=-1;
};

}} // namespace reverie::omega
#endif
