#ifndef REVERIE_BREAKWATER_POINTER_FOCUS_H
#define REVERIE_BREAKWATER_POINTER_FOCUS_H

namespace reverie { namespace breakwater {
// The shell sends POINTER_NONE while its Quick Menu owns input, or when the
// controller ray is unavailable. Suspend game time and require a fresh
// release before another press can reach gameplay on regained focus.
class PointerFocusGate {
public:
    void RequireRelease() {
        primary_blocked_ = true;
        secondary_blocked_ = true;
    }
    bool Filter(bool pointer_available, bool &primary, bool &secondary) {
        if (!pointer_available) {
            RequireRelease();
            primary = false;
            secondary = false;
            return false;
        }
        if (!primary) primary_blocked_ = false;
        else if (primary_blocked_) primary = false;
        if (!secondary) secondary_blocked_ = false;
        else if (secondary_blocked_) secondary = false;
        return true;
    }
private:
    bool primary_blocked_ = false;
    bool secondary_blocked_ = false;
};
}} // namespace reverie::breakwater
#endif
