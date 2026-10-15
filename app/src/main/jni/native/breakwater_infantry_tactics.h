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
// Riflemen from larger landing parties can occupy existing live shore
// barricades. No new unit, collision shape, RNG or saved-state field.
inline void AssignInfantryCover(
    Infantry &unit,
    const std::array<ShoreCover,Campaign::kMaxCover>& walls,
    uint8_t passenger
) {
    unit.open_lane_x=unit.lane_x;
    unit.cover_slot=-1;
    unit.regrouping=false;
    unit.cover_line_z=0.0f;
    if(!unit.active || unit.tactic!=InfantryTactic::Rifle ||
       !std::isfinite(unit.x))return;
    float bestDistance=1.18f;
    int chosen=-1;
    for(size_t i=0;i<walls.size();++i) {
        const ShoreCover& wall=walls[i];
        if(wall.hp<=0 || !std::isfinite(wall.x) ||
           !std::isfinite(wall.z))continue;
        const float distance=std::abs(unit.x-wall.x);
        if(distance<bestDistance) {
            bestDistance=distance;
            chosen=static_cast<int>(i);
        }
    }
    if(chosen<0)return;
    const ShoreCover &wall=walls[static_cast<size_t>(chosen)];
    unit.cover_slot=static_cast<int8_t>(chosen);
    unit.cover_line_z=wall.z-.29f;
    const float offset=passenger%2u==0u?.15f:-.15f;
    unit.lane_x=std::max(-3.1f,std::min(3.1f,wall.x+offset));
}
inline void RefreshInfantryCover(
    Infantry &unit,const std::array<ShoreCover,Campaign::kMaxCover>& walls
) {
    if(unit.cover_slot<0)return;
    const int slot=static_cast<int>(unit.cover_slot);
    if(slot<static_cast<int>(walls.size()) &&
       walls[static_cast<size_t>(slot)].hp>0)return;
    // Only a valid former cover order may relocate: malformed slots and
    // non-rifle roles fail back to the original open firing lane.
    if(slot<static_cast<int>(walls.size()) && unit.active &&
       unit.tactic==InfantryTactic::Rifle && std::isfinite(unit.x) &&
       std::isfinite(unit.open_lane_x) &&
       std::isfinite(walls[static_cast<size_t>(slot)].x)) {
        // Close-set Blackcap barricades form a secondary defensive line;
        // require proximity both to the unit AND to its former station.
        // Iron Quay's separated barriers must not become a phantom route
        // simply because a soldier was still approaching the first wall.
        // Pick from the actual surviving barriers, never a ghost wall.
        float nearest=1.90f;
        int replacement=-1;
        for(size_t i=0;i<walls.size();++i) {
            const ShoreCover &candidate=walls[i];
            if(static_cast<int>(i)==slot || candidate.hp<=0 ||
               !std::isfinite(candidate.x) || !std::isfinite(candidate.z) ||
               std::abs(candidate.x-walls[static_cast<size_t>(slot)].x)>1.90f)
                continue;
            const float distance=std::abs(unit.x-candidate.x);
            if(distance<nearest) {
                nearest=distance;
                replacement=static_cast<int>(i);
            }
        }
        if(replacement>=0) {
            const ShoreCover &next=walls[static_cast<size_t>(replacement)];
            unit.cover_slot=static_cast<int8_t>(replacement);
            unit.cover_line_z=next.z-.29f;
            unit.lane_x=std::max(-3.1f,std::min(3.1f,
                next.x+(unit.x<=next.x?-.15f:.15f)));
            unit.regrouping=false;
            return;
        }
    }
    // No usable nearby wall. Travel back to the original exposed lane;
    // regrouping must remain set until lateral motion really completes.
    unit.cover_slot=-1;
    unit.regrouping=true;
    unit.cover_line_z=0.0f;
    unit.lane_x=unit.open_lane_x;
}
inline bool InfantryCoverOrdered(const Infantry &unit) {
    return unit.cover_slot>=0 && unit.tactic==InfantryTactic::Rifle &&
        std::isfinite(unit.cover_line_z);
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
inline float InfantryLineZ(const Infantry &unit) {
    return InfantryCoverOrdered(unit)?unit.cover_line_z:
                                      InfantryLineZ(unit.tactic);
}
inline bool InfantryNeedsLane(const Infantry &unit) {
    return InfantryFlanking(unit.tactic) || InfantryCoverOrdered(unit) ||
           unit.regrouping;
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
       (InfantryNeedsLane(unit) && !std::isfinite(unit.lane_x)))
        return -1.0f;
    const float zLeft=std::max(0.0f,
        (InfantryLineZ(unit)-unit.z)/InfantrySpeed(unit.tactic));
    const float xLeft=InfantryNeedsLane(unit)?
        std::abs(unit.lane_x-unit.x)/kInfantryLateralSpeed:0.0f;
    return std::max(zLeft,xLeft);
}
inline bool InfantryReady(const Infantry &unit) {
    if(!unit.active || !std::isfinite(unit.z) ||
       !std::isfinite(unit.x) ||
       unit.z<InfantryLineZ(unit)-.00001f)return false;
    return !InfantryNeedsLane(unit) ||
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
    unit.z=std::min(InfantryLineZ(unit),
                    unit.z+InfantrySpeed(unit.tactic)*seconds);
    if(!InfantryNeedsLane(unit) || !std::isfinite(unit.lane_x))return;
    const float step=kInfantryLateralSpeed*seconds;
    if(unit.x<unit.lane_x)
        unit.x=std::min(unit.lane_x,unit.x+step);
    else if(unit.x>unit.lane_x)
        unit.x=std::max(unit.lane_x,unit.x-step);
    if(unit.regrouping && unit.x==unit.lane_x)
        unit.regrouping=false;
}

}} // namespace reverie::breakwater
#endif
