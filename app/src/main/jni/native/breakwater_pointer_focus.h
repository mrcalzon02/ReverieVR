#ifndef REVERIE_BREAKWATER_POINTER_FOCUS_H
#define REVERIE_BREAKWATER_POINTER_FOCUS_H
#include <cmath>

namespace reverie { namespace breakwater {
// Shell Quick Menu and headset pause own all gameplay input until the guest
// sees a valid pointer AND a complete neutral release. Back/Select and the
// touchpad share the shell, so held analog cannot accidentally cycle guns.
// The neutral frame itself is consumed: the next action must be deliberate.
class PointerFocusGate {
public:
    void RequireRelease() { awaiting_neutral_=true; }
    bool Filter(bool pointer_available,bool &primary,bool &secondary,
                float move_x=0.f,float move_y=0.f) {
        if(!pointer_available) {
            RequireRelease();
            primary=secondary=false;
            return false;
        }
        if(awaiting_neutral_) {
            const bool neutral=!primary&&!secondary &&
                std::isfinite(move_x)&&std::isfinite(move_y) &&
                std::fabs(move_x)<=.20f && std::fabs(move_y)<=.20f;
            primary=secondary=false;
            if(neutral)awaiting_neutral_=false;
            return false;
        }
        return true;
    }
private:
    bool awaiting_neutral_=false;
};
}} // reverie::breakwater
#endif
