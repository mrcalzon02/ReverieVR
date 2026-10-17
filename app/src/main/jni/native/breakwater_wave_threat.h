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
