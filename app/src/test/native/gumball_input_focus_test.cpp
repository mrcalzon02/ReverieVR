#include "gumball_input_focus.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using reverie::gumball::GuestInputFocus;

static ReverieNativeInputV1 Ray(){
    ReverieNativeInputV1 input{};
    input.struct_size=sizeof(input);input.delta_seconds=.05f;
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[2]=-1.f;return input;
}
int main(){
    GuestInputFocus gate;
    auto valid=Ray();
    assert(gate.Allow(&valid));
    auto shell=Ray();shell.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    shell.primary_down=shell.secondary_down=1u;
    shell.move_x=1.f;
    for(int frame=0;frame<180;++frame)assert(!gate.Allow(&shell));
    valid.primary_down=valid.secondary_down=1u;valid.move_x=1.f;
    for(int frame=0;frame<10;++frame)assert(!gate.Allow(&valid));
    valid.primary_down=0u;assert(!gate.Allow(&valid));
    valid.secondary_down=0u;assert(!gate.Allow(&valid));
    valid.move_x=0.f;
    assert(!gate.Allow(&valid)); // neutral rearm does not advance game
    assert(gate.Allow(&valid)); // next tick is a genuine active frame
    valid.primary_down=1u;assert(gate.Allow(&valid));

    // Invalid full pointer rays also suspend the game. No mutable input
    // from a bad optional ABI tail is accepted on recovery.
    auto nan=Ray();
    nan.pointer_direction[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!gate.Allow(&nan));
    valid.primary_down=1u;assert(!gate.Allow(&valid));
    valid.primary_down=0u;valid.move_y=.7f;assert(!gate.Allow(&valid));
    valid.move_y=0.f;assert(!gate.Allow(&valid));
    assert(gate.Allow(&valid));

    gate.RequireNeutral();
    valid.secondary_down=1u;assert(!gate.Allow(&valid));
    valid.secondary_down=0u;assert(!gate.Allow(&valid));
    assert(gate.Allow(&valid));

    // Older gamepad/button-only host frames must remain playable without
    // a pointer tail. Explicit full POINTER_NONE is different.
    GuestInputFocus legacy;
    auto base=Ray();base.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    base.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    base.primary_down=1u;
    assert(legacy.Allow(&base));
    legacy.RequireNeutral();assert(!legacy.Allow(&base));
    base.primary_down=0u;assert(!legacy.Allow(&base));
    assert(legacy.Allow(&base));
    base.primary_down=1u;assert(legacy.Allow(&base));
    base.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE-1u;
    assert(!legacy.Allow(&base));
    assert(!legacy.Allow(nullptr));
    std::cout<<"PASS Gumball shell focus freezes gameplay, neutral rearm and legacy input\n";
}
