#ifndef REVERIE_GUMBALL_INPUT_FOCUS_H
#define REVERIE_GUMBALL_INPUT_FOCUS_H
// Shell/Quick Menu input ownership is separate from a *legacy* base-only
// host without a controller-ray ABI extension. Never advance a timed sortie
// or buy ship parts while the host explicitly withdraws its valid pointer.
#include "reverie_native_sdk.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
namespace reverie { namespace gumball {
class GuestInputFocus {
public:
    void RequireNeutral() { need_neutral_=true; }
    bool Allow(const ReverieNativeInputV1* input) {
        if(!ReverieNativeInputHasBaseV1(input))return false;
        if(ReverieNativeInputHasPointerV1(input)) {
            ReverieNativeInputV1 sample{};
            std::memcpy(&sample,input,
                std::min<size_t>(input->struct_size,sizeof(sample)));
            if(!ReverieNativeSanitizePointerV1(&sample)) {
                need_neutral_=true;return false;
            }
        }
        if(need_neutral_) {
            // The release frame itself is not a simulation tick. Only a
            // subsequent fresh press can interact; held pad/Back is not a
            // silent substitute for a released controller.
            if(!input->primary_down&&!input->secondary_down&&
               std::isfinite(input->move_x)&&std::isfinite(input->move_y)&&
               std::fabs(input->move_x)<=.20f&&
               std::fabs(input->move_y)<=.20f)need_neutral_=false;
            return false;
        }
        return true;
    }
private:
    bool need_neutral_=false;
};
}} // reverie::gumball
#endif
