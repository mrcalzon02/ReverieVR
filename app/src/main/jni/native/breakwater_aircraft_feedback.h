#ifndef REVERIE_BREAKWATER_AIRCRAFT_FEEDBACK_H
#define REVERIE_BREAKWATER_AIRCRAFT_FEEDBACK_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

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
