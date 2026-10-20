#ifndef REVERIE_BREAKWATER_AIRCRAFT_FEEDBACK_H
#define REVERIE_BREAKWATER_AIRCRAFT_FEEDBACK_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace reverie { namespace breakwater {

// SAM is a controller-ray weapon, not a surface-boat selector. Compare
// ONLY live aircraft against the same 0.78-radius aircraft hit spheres used
// by ordinary ghost-ray targeting; the nearest valid intersection wins.
// One pure selection drives both the HUD bracket and the actual launch.
inline int NearestSamAircraft(
    const std::array<Enemy,Campaign::kMaxEnemies> &enemies,
    const float *origin,const float *direction) {
    if(!origin || !direction)return -1;
    float nearest=std::numeric_limits<float>::infinity();
    int target=-1;
    for(size_t i=0;i<enemies.size();++i) {
        const Enemy &enemy=enemies[i];
        if(!enemy.active || enemy.hp<=0 ||
           enemy.type!=UnitType::Aircraft)continue;
        const float center[]={enemy.x,1.6f,enemy.z};
        float depth=0.0f;
        if(RaySphereSurfaceDistance(origin,direction,center,.78f,&depth) &&
           depth<nearest) {
            nearest=depth;
            target=static_cast<int>(i);
        }
    }
    return target;
}

// The UI never guesses which sprite is targeted: it uses the same final
// CombatTarget enemy slot chosen by the native ray intersection path.
struct AircraftAimFeedback {
    bool visible = false;
    bool equipped = false;
    AircraftPattern pattern = AircraftPattern::None;
    int16_t hp = 0;
    float x = 0.0f;
    float z = 0.0f;
};

// Pure guard. No target, disabled target, land unit, or terminal phase
// produces an apparent aircraft lock. An unupgraded gun warns but does
// not fabricate damageability.
inline AircraftAimFeedback InspectAircraftAim(const Campaign &game, int slot) {
    AircraftAimFeedback result{};
    if (game.phase() != Phase::Combat || slot < 0 ||
        slot >= static_cast<int>(Campaign::kMaxEnemies)) return result;
    const Enemy &enemy = game.enemies()[static_cast<size_t>(slot)];
    if (!enemy.active || enemy.type != UnitType::Aircraft ||
        enemy.hp <= 0 || !std::isfinite(enemy.x) ||
        !std::isfinite(enemy.z)) return result;
    result.visible = true;
    result.equipped = game.stats().anti_air;
    result.pattern = enemy.aircraft_pattern;
    result.hp = enemy.hp;
    result.x = enemy.x;
    result.z = enemy.z;
    return result;
}

// A purchased SAM is itself a valid aircraft weapon, even without the
// permanent AA upgrade. Never show AA REQUIRED while a SAM lock is armed.
inline AircraftAimFeedback InspectSamAim(const Campaign &game,int slot) {
    AircraftAimFeedback result=InspectAircraftAim(game,slot);
    if(result.visible)result.equipped=game.sam_charges()>0;
    return result;
}

// Actual aircraft HP delta, never a merely fired round or a guessed hit.
// Record the original coordinates even when a lethal hit deactivates the
// aircraft. Advance the effect once per input update, not once per eye.
class AircraftHitFeedback {
public:
    static constexpr float kDuration = .55f;

    bool Record(const Enemy &before, const Enemy &after, bool fired) {
        if (!fired || !before.active || before.type != UnitType::Aircraft ||
            before.hp <= 0 || after.hp >= before.hp ||
            !std::isfinite(before.x) || !std::isfinite(before.z)) return false;
        damage_ = before.hp - std::max<int>(0,after.hp);
        if (damage_ <= 0) return false;
        x_ = before.x;
        z_ = before.z;
        destroyed_ = !after.active;
        remaining_ = kDuration;
        return true;
    }

    void Tick(float seconds) {
        if (!std::isfinite(seconds) || seconds <= 0.0f) return;
        remaining_ = std::max(0.0f,remaining_ - seconds);
    }

    bool active() const { return remaining_ > 0.0f; }
    int damage() const { return damage_; }
    bool destroyed() const { return destroyed_; }
    float x() const { return x_; }
    float z() const { return z_; }
    float remaining() const { return remaining_; }
private:
    float remaining_ = 0.0f;
    float x_ = 0.0f;
    float z_ = 0.0f;
    int damage_ = 0;
    bool destroyed_ = false;
};
}} // namespace reverie::breakwater

#endif
