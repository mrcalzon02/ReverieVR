#ifndef REVERIE_BREAKWATER_SURFACE_AIM_H
#define REVERIE_BREAKWATER_SURFACE_AIM_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

// Presentation only: never changes CombatTarget, collision spheres, enemy
// health, player damage or the actual weapon's projectile arc. A marker is
// shown only after the existing authoritative targeting ray hits this unit.
struct SurfaceAimFeedback {
    bool visible=false;
    bool infantry=false;
    bool vulnerable=false;
    UnitType type=UnitType::Rowboat;
    int16_t hp=0;
    int16_t max_hp=0;
    float x=0.0f,y=0.0f,z=0.0f;
    float half_width=0.0f;
};

inline int16_t SurfaceBaseHp(UnitType type) {
    return type==UnitType::Aircraft ? 0 : EnemyStartingHp(type);
}

inline SurfaceAimFeedback InspectSurfaceEnemy(
    const Enemy &enemy, WeaponKind weapon
) {
    SurfaceAimFeedback result{};
    const int16_t maxHp=SurfaceBaseHp(enemy.type);
    if(!enemy.active || maxHp<=0 || enemy.hp<=0 ||
        !std::isfinite(enemy.x) || !std::isfinite(enemy.z))return result;
    result.visible=true;
    result.vulnerable=true; // All five mounted weapons can hit surface hulls.
    result.type=enemy.type;
    result.hp=static_cast<int16_t>(std::min<int>(enemy.hp,maxHp));
    result.max_hp=maxHp;
    result.x=enemy.x;result.y=-.63f;result.z=enemy.z;
    result.half_width=enemy.type==UnitType::Rowboat?.52f:
                      enemy.type==UnitType::LandingCraft?.70f:.76f;
    (void)weapon; // Weapon changes ballistics, not surface vulnerability.
    return result;
}

inline SurfaceAimFeedback InspectSurfaceInfantry(
    const Infantry &soldier, WeaponKind weapon
) {
    SurfaceAimFeedback result{};
    if(!soldier.active || !std::isfinite(soldier.x) ||
       !std::isfinite(soldier.z))return result;
    result.visible=true;
    result.infantry=true;
    result.vulnerable=WeaponFor(weapon).anti_infantry;
    result.x=soldier.x;result.y=-.66f;result.z=soldier.z;
    result.half_width=.24f;
    return result;
}

inline SurfaceAimFeedback InspectSurfaceAim(
    const Campaign &game,int target
) {
    if(game.phase()!=Phase::Combat || target<0)return {};
    if(target<static_cast<int>(Campaign::kMaxEnemies))
        return InspectSurfaceEnemy(game.enemies()[static_cast<size_t>(target)],
                                   game.selected_weapon());
    const int infantryIndex=target-static_cast<int>(Campaign::kMaxEnemies);
    if(infantryIndex<0 ||
       infantryIndex>=static_cast<int>(Campaign::kMaxInfantry))return {};
    return InspectSurfaceInfantry(
        game.infantry_units()[static_cast<size_t>(infantryIndex)],
        game.selected_weapon());
}

inline float SurfaceHullFraction(const SurfaceAimFeedback &aim) {
    if(!aim.visible || aim.infantry || aim.max_hp<=0)return 0.0f;
    return std::max(0.0f,std::min(1.0f,
        static_cast<float>(aim.hp)/static_cast<float>(aim.max_hp)));
}
}} // namespace reverie::breakwater
#endif
