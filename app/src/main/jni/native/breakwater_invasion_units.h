#ifndef REVERIE_BREAKWATER_INVASION_UNITS_H
#define REVERIE_BREAKWATER_INVASION_UNITS_H
#include "breakwater_campaign.h"
#include <cstdint>
namespace reverie { namespace breakwater { namespace invasion {
// Deterministic enemy allocation: late-war heavy landing ships do not
// replace the earliest rowboat, gunboat or aircraft introductions.
constexpr UnitType SelectUnit(unsigned progression,uint8_t wave_index,
                              uint8_t waves_in_period,uint32_t roll,
                              uint8_t spawn_ordinal) {
    if(progression<2u)return UnitType::Rowboat;
    // Heavy armored landing ships appear only from the hard late-war
    // progression onward and in the latter half of the actual period.
    // The last wave guarantees a flagship (first spawned slot), so players
    // cannot finish the campaign without encountering the new opponent.
    if(progression>=10u && wave_index>(waves_in_period/2u) &&
       ((roll%4u)==0u ||
        (wave_index==waves_in_period && spawn_ordinal==0u)))
        return UnitType::ArmoredTransport;
    if(progression>=8u && roll%6u==0u)return UnitType::Aircraft;
    if(progression>=5u && roll%4u==0u)return UnitType::Gunboat;
    if(progression>=3u && roll%3u==0u)return UnitType::LandingCraft;
    return UnitType::Rowboat;
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
