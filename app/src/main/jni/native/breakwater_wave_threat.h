#ifndef REVERIE_BREAKWATER_WAVE_THREAT_H
#define REVERIE_BREAKWATER_WAVE_THREAT_H
#include "breakwater_campaign.h"
#include <algorithm>
#include <cstdint>
namespace reverie { namespace breakwater { namespace waves {
// Progression is the location/day/night/difficulty threat grade.
// The first two grades stay rowboat-only. Later waves in the SAME period
// gain up to three pressure bands without raising the active-unit cap.
// Using the existing per-spawn roll preserves seeded replay and save shape.
inline unsigned PressureBand(unsigned wave_index,unsigned waves_this_period) {
    if(waves_this_period==0u||wave_index==0u)return 0u;
    const unsigned index=std::min(wave_index,waves_this_period)-1u;
    return std::min(3u,(index*4u)/waves_this_period);
}
// Seeded, distinct wave operations use only already-existing enemy models.
// The first quarter of a day/night period teaches the player the location;
// the remaining waves mix heavier combined-arms assaults.
enum class Operation : uint8_t {
    Mixed, LandingPush, NavalScreen, AirRaid, Flagship
};
inline Operation Plan(unsigned progression,unsigned wave,unsigned total) {
    if(wave==0u || total==0u || wave>total || progression<3u)
        return Operation::Mixed;
    if(wave==total)
        return progression>=10u?Operation::Flagship:Operation::Mixed;
    if(PressureBand(wave,total)==0u)return Operation::Mixed;
    if(progression>=8u && wave%7u==0u)return Operation::AirRaid;
    if(progression>=5u && wave%6u==0u)return Operation::NavalScreen;
    if(wave%5u==0u)return Operation::LandingPush;
    return Operation::Mixed;
}
inline const char *OperationName(Operation op) {
    switch(op) {
        case Operation::LandingPush: return "LANDING PUSH";
        case Operation::NavalScreen: return "NAVAL SCREEN";
        case Operation::AirRaid: return "AIR RAID";
        case Operation::Flagship: return "FLAGSHIP ASSAULT";
        default: return "MIXED APPROACH";
    }
}
// Total attackers per wave, not concurrent attackers. Keep the current
// 12 surface/air slots, 60 infantry and 20 armored cars unchanged.
inline uint8_t SpawnBudget(uint8_t location,uint8_t day,bool night,
                           uint8_t tier,unsigned wave,unsigned total) {
    const unsigned base=2u+std::min<unsigned>(tier,3u)+location+
        (day>0?static_cast<unsigned>(day-1u):0u)+(night?1u:0u);
    const unsigned pressure=PressureBand(wave,total);
    return static_cast<uint8_t>(std::min(42u,3u*base+
        3u*pressure+(wave==total && pressure>0u?2u:0u)));
}
// Arrival cadence tightens in steps but remains deterministic and bounded,
// never spawning another entity if all twelve live slots are occupied.
inline float SpawnGapSeconds(unsigned wave,unsigned total) {
    return .65f-.055f*static_cast<float>(PressureBand(wave,total));
}
inline UnitType SelectInvasionUnit(unsigned progression,unsigned wave_index,
                                   unsigned waves_this_period,uint32_t roll) {
    if(progression<3u)return UnitType::Rowboat;
    const unsigned pressure=PressureBand(wave_index,waves_this_period);
    const unsigned grade=progression+pressure;
    // Air and artillery remain gated by the campaign threat grade, but
    // a late wave's tighter formations make them increasingly likely.
    if(grade>=8u&&roll%(6u-pressure)==0u)return UnitType::Aircraft;
    // In band three, aircraft and gunboats both used roll % 3:
    // earlier aircraft selection made later gunboats impossible.
    // Mix independent high seeded bits only for this overlap.
    const uint32_t gunboat_roll=(pressure==3u && grade>=8u)?
        (roll^(roll>>16u)):roll;
    if(grade>=5u&&gunboat_roll%(4u-pressure/2u)==0u)
        return UnitType::Gunboat;
    if(roll%(3u-pressure/2u)==0u)return UnitType::LandingCraft;
    return UnitType::Rowboat;
}
}}}
#endif
