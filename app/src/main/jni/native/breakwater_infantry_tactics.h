#ifndef REVERIE_BREAKWATER_INFANTRY_TACTICS_H
#define REVERIE_BREAKWATER_INFANTRY_TACTICS_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

constexpr float kInfantrySalvoPeriod=4.0f;

// Preserve the original single rifleman on each early rowboat. Larger
// landing parties add two lateral maneuver troops and a rear support gunner,
// assigned from passenger order with no RNG or save-schema changes.
inline InfantryTactic AssignInfantryTactic(UnitType vessel,uint8_t passenger) {
    if(vessel!=UnitType::LandingCraft)return InfantryTactic::Rifle;
    switch(passenger%4u) {
        case 1: return InfantryTactic::FlankLeft;
        case 2: return InfantryTactic::FlankRight;
        case 3: return InfantryTactic::Support;
        default: return InfantryTactic::Rifle;
    }
}
inline bool InfantryFlanking(InfantryTactic tactic) {
    return tactic==InfantryTactic::FlankLeft ||
           tactic==InfantryTactic::FlankRight;
}
inline float InfantryLaneX(float start,InfantryTactic tactic) {
    if(!std::isfinite(start))return 0.0f;
    const float move=tactic==InfantryTactic::FlankLeft?-.85f:
                     tactic==InfantryTactic::FlankRight?.85f:0.0f;
    return std::max(-3.1f,std::min(3.1f,start+move));
}
inline float InfantryLineZ(InfantryTactic tactic) {
    return tactic==InfantryTactic::Support?-1.68f:-1.34f;
}
inline float InfantrySpeed(InfantryTactic tactic) {
    return tactic==InfantryTactic::Support?.105f:
           InfantryFlanking(tactic)?.135f:.13f;
}
constexpr float kInfantryLateralSpeed=.19f;
inline uint8_t InfantryShotDamage(InfantryTactic tactic) {
    return tactic==InfantryTactic::Support?1u:2u;
}
inline float InfantrySecondsToReady(const Infantry &unit) {
    if(!unit.active || !std::isfinite(unit.z) ||
       !std::isfinite(unit.x) ||
       (InfantryFlanking(unit.tactic) && !std::isfinite(unit.lane_x)))
        return -1.0f;
    const float zLeft=std::max(0.0f,
        (InfantryLineZ(unit.tactic)-unit.z)/InfantrySpeed(unit.tactic));
    const float xLeft=InfantryFlanking(unit.tactic)?
        std::abs(unit.lane_x-unit.x)/kInfantryLateralSpeed:0.0f;
    return std::max(zLeft,xLeft);
}
inline bool InfantryReady(const Infantry &unit) {
    if(!unit.active || !std::isfinite(unit.z) ||
       !std::isfinite(unit.x) ||
       unit.z<InfantryLineZ(unit.tactic)-.00001f)return false;
    return !InfantryFlanking(unit.tactic) ||
        (std::isfinite(unit.lane_x) &&
         std::abs(unit.lane_x-unit.x)<.00001f);
}
// The actual Campaign::TickStep event stores its firing-mask at the
// moment of damage, before subsequent substeps/arrivals can change it.
static_assert(Campaign::kMaxInfantry<=64,
              "infantry fire roster must fit the fixed 64-bit mask");
inline bool InfantryFiringPose(
    const Infantry &soldier,uint64_t mask,size_t index
) {
    return index<Campaign::kMaxInfantry &&
        (mask & (uint64_t{1}<<index))!=0 && InfantryReady(soldier);
}
inline void AdvanceInfantry(Infantry &unit,float seconds) {
    if(!unit.active || !std::isfinite(seconds) || seconds<=0.0f ||
       !std::isfinite(unit.x) || !std::isfinite(unit.z))return;
    unit.z=std::min(InfantryLineZ(unit.tactic),
                    unit.z+InfantrySpeed(unit.tactic)*seconds);
    if(!InfantryFlanking(unit.tactic) || !std::isfinite(unit.lane_x))return;
    const float step=kInfantryLateralSpeed*seconds;
    if(unit.x<unit.lane_x)
        unit.x=std::min(unit.lane_x,unit.x+step);
    else if(unit.x>unit.lane_x)
        unit.x=std::max(unit.lane_x,unit.x-step);
}

}} // namespace reverie::breakwater
#endif
