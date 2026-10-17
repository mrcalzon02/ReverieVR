#ifndef REVERIE_BREAKWATER_ARMOR_H
#define REVERIE_BREAKWATER_ARMOR_H
#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater { namespace armor {
// Full size landing craft remain distinct from the smaller armored car.
// This is seeded from the existing per-spawn Hash(), with no runtime RNG.
// First-day introductions remain one-passenger rowboats.
constexpr int16_t kVehicleHp=95;
constexpr float kArmorLineZ=-1.56f;
constexpr float kArmorSpeed=.29f;
constexpr float kArmorFireInterval=3.3f;
constexpr int32_t kArmorShellDamage=4;
constexpr int32_t kArmorKillCredits=110;
constexpr float kArmorHitRadius=.43f;
constexpr float kArmorHitY=-.72f;
inline bool CarriesArmor(UnitType kind,unsigned progression,
                         uint32_t choice,unsigned spawned) {
    return kind==UnitType::LandingCraft && progression>=4u &&
           (((choice>>3u)+spawned)&1u)==0u;
}
inline uint8_t TroopPassengers(uint8_t normal,bool carries_armor) {
    return carries_armor?static_cast<uint8_t>(std::min<unsigned>(normal,3u)):normal;
}
inline int ArmorDamage(WeaponKind weapon,uint16_t damage) {
    // Rifle-caliber bullets still chip armor, heavy rounds penetrate more.
    switch(weapon) {
        case WeaponKind::MachineGun: return std::max(1,static_cast<int>(damage)/4);
        case WeaponKind::HeavyGun: return std::max(1,static_cast<int>(damage)/2);
        case WeaponKind::Flak: return std::max(1,static_cast<int>(damage)/3);
        case WeaponKind::MissileBattery:
        case WeaponKind::DirectCannon: return static_cast<int>(damage);
    }
    return 0;
}
inline void Advance(ArmoredVehicle &v,float dt) {
    if(!v.active || !std::isfinite(dt) || dt<=0.0f)return;
    v.z=std::min(kArmorLineZ,v.z+kArmorSpeed*dt);
    v.attack_timer=std::max(0.0f,v.attack_timer-dt);
}
inline bool Ready(const ArmoredVehicle &v) {
    return v.active && v.hp>0 && v.z>=kArmorLineZ &&
           v.attack_timer<=0.0f;
}
inline bool Hits(const ArmoredVehicle &v,const float* origin,
                 const float* direction,float gravity,float* depth) {
    if(!v.active || v.hp<=0)return false;
    const float center[]={v.x,kArmorHitY,v.z};
    return ProjectileHitsSphere(origin,direction,gravity,
                                center,kArmorHitRadius,depth);
}
}}}
#endif
