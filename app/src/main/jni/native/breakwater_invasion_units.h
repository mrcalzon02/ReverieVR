#ifndef REVERIE_BREAKWATER_INVASION_UNITS_H
#define REVERIE_BREAKWATER_INVASION_UNITS_H
#include "breakwater_campaign.h"
#include "breakwater_wave_threat.h"
#include <cstdint>
namespace reverie { namespace breakwater { namespace invasion {
// Deterministic enemy allocation: late-war heavy landing ships do not
// replace the earliest rowboat, gunboat or aircraft introductions.
inline UnitType SelectUnit(unsigned progression,uint8_t wave_index,
                           uint8_t waves_in_period,uint32_t roll,
                           uint8_t spawn_ordinal) {
    // Resolve existing pressure-band composition before promoting landers.
    // The old roll % 4 heavy branch inadvertently stole all gunboats
    // in pressure bands using the same divisor.
    const UnitType baseline=waves::SelectInvasionUnit(
        progression,wave_index,waves_in_period,roll);
    if(progression<10u || wave_index<=waves_in_period/2u)return baseline;
    if(wave_index==waves_in_period && spawn_ordinal==0u)
        return UnitType::ArmoredTransport;
    // Only upgrade ordinary landing vessels, not gunboats or aircraft.
    if((baseline==UnitType::Rowboat ||
        baseline==UnitType::LandingCraft) &&
       ((roll>>16u)&3u)==0u)return UnitType::ArmoredTransport;
    return baseline;
}
// An intact armored hull holds its beachhead with the bow ramp open;
// cargo enters combat only after this exposed unloading interval.
constexpr float UnloadingSeconds(UnitType kind) {
    return kind==UnitType::ArmoredTransport?1.25f:0.0f;
}
// Armored ships have separate plating, not a misleading extension of HP.
// Heavy cannon and missiles retain their full rated penetration.
constexpr int HullDamage(UnitType kind,WeaponKind gun,int damage) {
    if(damage<=0)return 0;
    if(kind!=UnitType::ArmoredTransport)return damage;
    switch(gun) {
        case WeaponKind::MachineGun: return damage/5>0?damage/5:1;
        case WeaponKind::HeavyGun: return damage/3>0?damage/3:1;
        case WeaponKind::Flak: return damage/4>0?damage/4:1;
        case WeaponKind::DirectCannon:
        case WeaponKind::MissileBattery: return damage;
    }
    return 0;
}
constexpr int SupportDamage(UnitType kind,int damage) {
    if(kind!=UnitType::ArmoredTransport)return damage;
    // Large splash detonations can crack its upper deck but are less
    // efficient than a direct heavy shell aimed at the plated hull.
    return damage*3/4;
}
constexpr uint8_t VehicleCargo(UnitType kind,bool armored) {
    return kind==UnitType::ArmoredTransport?2u:
        (kind==UnitType::LandingCraft && armored?1u:0u);
}
constexpr uint8_t InfantryCargo(UnitType kind,uint32_t roll) {
    return kind==UnitType::ArmoredTransport?4u:
        kind==UnitType::LandingCraft?
            static_cast<uint8_t>(3u+(roll>>8u)%4u):
        kind==UnitType::Rowboat?1u:0u;
}
}}}
#endif
