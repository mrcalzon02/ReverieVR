#ifndef REVERIE_BREAKWATER_COMBAT_INPUT_H
#define REVERIE_BREAKWATER_COMBAT_INPUT_H
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class CombatHudTarget : uint8_t {
    None, Artillery, Airstrike, Sam, Weapon0, Weapon1, Weapon2,
    Weapon3, Weapon4
};
enum class CombatInputRoute : uint8_t {
    Gun, NoAction, Artillery, Airstrike, Sam, Weapon0, Weapon1,
    Weapon2, Weapon3, Weapon4
};

inline CombatHudTarget HitCombatSupportHud(float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y))
        return CombatHudTarget::None;
    if(y>.65f && y<1.25f) {
        if(x<-1.05f && x>-3.4f)return CombatHudTarget::Artillery;
        if(x>1.05f && x<3.4f)return CombatHudTarget::Airstrike;
        if(std::abs(x)<.61f)return CombatHudTarget::Sam;
    }
    if(y>-1.65f && y<-1.23f) {
        for(uint8_t id=0;id<5;id++)
            if(std::abs(x-(-2.4f+id*1.2f))<.52f)
                return static_cast<CombatHudTarget>(
                    static_cast<uint8_t>(CombatHudTarget::Weapon0)+id);
    }
    return CombatHudTarget::None;
}

constexpr CombatInputRoute RouteCombatInput(CombatHudTarget target, bool edge) {
    if (target == CombatHudTarget::None) return CombatInputRoute::Gun;
    if (!edge) return CombatInputRoute::NoAction;
    return static_cast<CombatInputRoute>(static_cast<uint8_t>(target));
}
// A Select press belongs to its first UI action until release. This also
// prevents a held shop START press from firing the gun in the next phase.
class CombatInputLatch {
public:
    CombatInputRoute Route(CombatHudTarget target, bool edge) {
        if (ui_press_held_) return CombatInputRoute::NoAction;
        const CombatInputRoute route = RouteCombatInput(target, edge);
        if(route!=CombatInputRoute::Gun &&
           route!=CombatInputRoute::NoAction)ui_press_held_=true;
        return route;
    }
    void CaptureUiPress() { ui_press_held_ = true; }
    void Release() { ui_press_held_ = false; }
private:
    bool ui_press_held_ = false;
};
}} // namespace reverie::breakwater
#endif
