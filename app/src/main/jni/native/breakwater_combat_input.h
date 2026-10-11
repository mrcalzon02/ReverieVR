#ifndef REVERIE_BREAKWATER_COMBAT_INPUT_H
#define REVERIE_BREAKWATER_COMBAT_INPUT_H
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class CombatHudTarget : uint8_t { None, Artillery, Airstrike };
enum class CombatInputRoute : uint8_t { Gun, NoAction, Artillery, Airstrike };

inline CombatHudTarget HitCombatSupportHud(float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        y <= .65f || y >= 1.25f) return CombatHudTarget::None;
    if (x < -1.05f && x > -3.4f) return CombatHudTarget::Artillery;
    if (x > 1.05f && x < 3.4f) return CombatHudTarget::Airstrike;
    return CombatHudTarget::None;
}

constexpr CombatInputRoute RouteCombatInput(CombatHudTarget target, bool edge) {
    if (target == CombatHudTarget::None) return CombatInputRoute::Gun;
    if (!edge) return CombatInputRoute::NoAction;
    return target == CombatHudTarget::Artillery ?
        CombatInputRoute::Artillery : CombatInputRoute::Airstrike;
}
}} // namespace reverie::breakwater
#endif
