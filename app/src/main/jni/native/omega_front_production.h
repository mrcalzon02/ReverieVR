#ifndef REVERIE_OMEGA_FRONT_PRODUCTION_H
#define REVERIE_OMEGA_FRONT_PRODUCTION_H

#include <cstdint>

namespace reverie { namespace omega {

struct DailyYield {
    uint16_t food;
    uint16_t water;
};

// One campaign-day of civilian work at an existing inhabited Eden site.
// Stocks remain physically in that site; this does not create command rations,
// teleport food, or supply uninhabited newly emerged terrain.
struct CivilProduction {
    static constexpr uint16_t Store(uint16_t stock, uint16_t added) {
        return static_cast<uint16_t>(static_cast<uint32_t>(stock)+added>65535u
            ?65535u:static_cast<uint32_t>(stock)+added);
    }
    static constexpr DailyYield Produce(int sector, unsigned ecology,
            unsigned stability, unsigned anomaly, unsigned residents,
            unsigned claims, int pumpHealth) {
        if(ecology!=0 || stability<35 || anomaly!=0 || residents==0)
            return {0,0};
        // Insecure or recently transformed farmland works at reduced output.
        const unsigned divisor=1u+(stability<60?1u:0u)+(claims>=60?1u:0u);
        if(sector==10) {
            // Lantern Pump employs operators, not farmers. Its output stops
            // when the REAL waterworks is destroyed, not at a distant site.
            const unsigned pumped=pumpHealth>0
                ?static_cast<unsigned>(1+(pumpHealth+3)/4):0u;
            return {0,static_cast<uint16_t>(pumped/divisor)};
        }
        const unsigned grown=1u+residents/40u;
        const unsigned drawn=1u+residents/90u;
        return {static_cast<uint16_t>((grown>12u?12u:grown)/divisor),
                static_cast<uint16_t>((drawn>12u?12u:drawn)/divisor)};
    }
};

}} // namespace reverie::omega
#endif
