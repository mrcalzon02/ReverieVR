#ifndef REVERIE_SITKA_INPUT_FOCUS_H
#define REVERIE_SITKA_INPUT_FOCUS_H

// Game-private shell ownership guard. A full native pointer prefix carrying
// POINTER_NONE or an invalid ray means the shell/Quick Menu owns input.
// The shorter legacy base ABI has no pointer and remains usable.
#include "reverie_native_sdk.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace reverie { namespace sitka {
class InputFocusGate {
public:
    // Pause/return-to-Home must require neutral input before any action.
    void RequireNeutral() { needs_neutral_=true; }
    bool Allow(const ReverieNativeInputV1* incoming) {
        if(!ReverieNativeInputHasBaseV1(incoming))return false;
        if(ReverieNativeInputHasPointerV1(incoming)){
            ReverieNativeInputV1 pointer{};
            std::memcpy(&pointer,incoming,
                std::min<size_t>(incoming->struct_size,sizeof(pointer)));
            if(!ReverieNativeSanitizePointerV1(&pointer)){
                needs_neutral_=true;
                return false;
            }
        }
        if(needs_neutral_){
            // Releasing Select alone is insufficient when Back or a
            // directional chord is still held from Quick Menu navigation.
            if(incoming->primary_down||incoming->secondary_down||
               !std::isfinite(incoming->move_x)||
               !std::isfinite(incoming->move_y)||
               std::fabs(incoming->move_x)>.20f||
               std::fabs(incoming->move_y)>.20f)return false;
            needs_neutral_=false;
        }
        return true;
    }
private:
    bool needs_neutral_=false;
};
}} // reverie::sitka
#endif
