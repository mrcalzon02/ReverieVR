#ifndef REVERIE_OMEGA_FRONT_OBLIGATIONS_H
#define REVERIE_OMEGA_FRONT_OBLIGATIONS_H

#include "omega_front_accord.h"
#include <array>
#include <cstdint>

namespace reverie { namespace omega {

// An accord is a past political choice. An obligation is a promise that can
// subsequently be kept or broken. Neither outcome changes battle results.
// Source-only initial theater policy, not a global diplomatic AI.
class Obligations {
public:
    enum Status : uint8_t { None=0, Due=1, Honored=2, Broken=3, Legacy=4 };
    static constexpr int kFirst=3, kLast=5, kDelayDays=3;
    Status status(int operation) const {
        return Valid(operation) ? status_[operation-kFirst] : None;
    }
    int start_day(int operation) const {
        return Valid(operation) ? static_cast<int>(signed_[operation-kFirst]) : 0;
    }
    int due_day(int operation) const {
        return status(operation)==Due ? start_day(operation)+kDelayDays : -1;
    }
    bool Start(int operation,int day) {
        if(!Valid(operation)||day<1||day>65000||status(operation)!=None)return false;
        const int i=operation-kFirst;
        status_[i]=Due;
        signed_[i]=static_cast<uint16_t>(day);
        return true;
    }
    // Prior versions recorded a signed treaty but promised no future dues.
    // Loading those saves must never retroactively demand a payment.
    bool Migrate(uint8_t accord_mask) {
        if((accord_mask & ~Accord::kPermittedMask)!=0)return false;
        for(int op=kFirst;op<=kLast;++op)
            if(accord_mask & (1u<<op))status_[op-kFirst]=Legacy;
        return true;
    }
    // Only eligible three-sector accords may have obligations. All invariants
    // checked before mutating: a forged checksum is not sufficient admission.
    bool Restore(const std::array<uint8_t,3>& stages,
                 const std::array<uint16_t,3>& days,
                 uint8_t accord_mask,int current_day) {
        if(current_day<1||current_day>100000 ||
           (accord_mask & ~Accord::kPermittedMask)!=0)return false;
        for(int i=0;i<3;++i) {
            const uint8_t stage=stages[i];
            const uint16_t day=days[i];
            const bool accord=(accord_mask & (1u<<(kFirst+i)))!=0;
            if(stage>Legacy || (stage!=None)!=accord)return false;
            if((stage==None||stage==Legacy) && day!=0)return false;
            if(stage==Due && (day<1||day>65000||day>current_day||
                              current_day>=static_cast<int>(day)+kDelayDays))return false;
            if((stage==Honored||stage==Broken) &&
               (day<1||day>65000||static_cast<int>(day)+kDelayDays>current_day))return false;
        }
        for(int i=0;i<3;++i){status_[i]=static_cast<Status>(stages[i]);signed_[i]=days[i];}
        return true;
    }
    // Returns true on a one-time settlement (whether honored or breached).
    // Resources are transferred to physical faction stores; never conjured.
    bool Settle(int operation,int day,int &rations,int &salvage,int &trust,
                int8_t &relation,uint16_t &wreck,uint16_t &food,uint8_t &claims) {
        if(!Valid(operation)||status(operation)!=Due ||
           day<due_day(operation))return false;
        const int i=operation-kFirst;
        const bool ration_debt=operation==5;
        const bool affordable=ration_debt ? (rations>=2 && food<=65533) :
                                           (salvage>=2 && wreck<=65533);
        if(affordable){
            if(ration_debt){rations-=2;food=static_cast<uint16_t>(food+2);}
            else {salvage-=2;wreck=static_cast<uint16_t>(wreck+2);}
            relation=ClampRelation(static_cast<int>(relation)+3);
            claims=static_cast<uint8_t>(claims>=3?claims-3:0);
            status_[i]=Honored;
        }else{
            // The faction sees a default, but receives no phantom payment.
            relation=ClampRelation(static_cast<int>(relation)-15);
            trust=trust>=4?trust-4:0;
            claims=static_cast<uint8_t>(claims>=88?100:claims+12);
            status_[i]=Broken;
        }
        return true;
    }
    std::array<uint8_t,3> stages() const {
        return {{static_cast<uint8_t>(status_[0]),static_cast<uint8_t>(status_[1]),
                 static_cast<uint8_t>(status_[2])}};
    }
    const std::array<uint16_t,3>& days() const{return signed_;}
private:
    static bool Valid(int op){return op>=kFirst&&op<=kLast;}
    static int8_t ClampRelation(int v){return static_cast<int8_t>(v>100?100:v< -100?-100:v);}
    std::array<Status,3> status_{{None,None,None}};
    std::array<uint16_t,3> signed_{};
};

}} // namespace reverie::omega
#endif
