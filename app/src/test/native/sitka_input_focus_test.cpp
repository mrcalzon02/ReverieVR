#include "../../main/jni/native/sitka_input_focus.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
using reverie::sitka::InputFocusGate;

static ReverieNativeInputV1 Controller() {
    ReverieNativeInputV1 i{};
    i.struct_size=sizeof(i);
    i.delta_seconds=.05f;
    i.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    i.pointer_direction[2]=-1.f;
    return i;
}

int main() {
    InputFocusGate gate;
    auto live=Controller();
    assert(gate.Allow(&live));
    // When the shell withdraws the full pointer prefix, nothing from a
    // previously held Select, Back or touchpad may reach the game.
    auto missing=live;
    missing.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    missing.primary_down=1;
    missing.secondary_down=1;
    missing.move_x=.9f;
    for(int frame=0;frame<90;++frame)assert(!gate.Allow(&missing));

    live.primary_down=1;live.secondary_down=1;live.move_x=.9f;
    for(int frame=0;frame<5;++frame)assert(!gate.Allow(&live));
    live.primary_down=0;
    assert(!gate.Allow(&live)); // Back and stick still held.
    live.secondary_down=0;
    assert(!gate.Allow(&live)); // Stick is not neutral.
    live.move_x=0.f;
    assert(gate.Allow(&live));
    live.primary_down=1;
    assert(gate.Allow(&live)); // New intentional action only after release.

    // An invalid tracked ray also withdraws gameplay; only a valid new
    // controller ray with neutral controls can return to the game.
    auto invalid=Controller();
    invalid.pointer_direction[2]=0.f;
    invalid.primary_down=1u;
    assert(!gate.Allow(&invalid));
    auto restored=Controller();
    restored.primary_down=1u;
    assert(!gate.Allow(&restored));
    restored.primary_down=0u;
    restored.move_y=.60f;
    assert(!gate.Allow(&restored));
    restored.move_y=0.f;
    assert(gate.Allow(&restored));

    // Pause is an explicit shell ownership boundary even if no
    // POINTER_NONE frame happened to be delivered by the host.
    gate.RequireNeutral();
    restored.secondary_down=1u;
    assert(!gate.Allow(&restored));
    restored.secondary_down=0u;
    assert(gate.Allow(&restored));

    // Legacy base-only hosts with no pointer extension remain usable.
    InputFocusGate legacy;
    auto old=Controller();
    old.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    old.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    old.primary_down=1u;
    assert(legacy.Allow(&old));
    legacy.RequireNeutral();
    assert(!legacy.Allow(&old));
    old.primary_down=0u;
    assert(legacy.Allow(&old));
    old.primary_down=1u;
    assert(legacy.Allow(&old));

    // A truncated optional pointer tail is also an old base-only frame.
    old.struct_size=offsetof(ReverieNativeInputV1,pointer_direction);
    assert(legacy.Allow(&old));
    // Invalid/truncated mandatory ABI is never accepted.
    old.struct_size=REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE-1u;
    assert(!legacy.Allow(&old));
    assert(!legacy.Allow(nullptr));

    // Full prefix whose direction is NaN must not pass as a valid ray.
    auto nanRay=Controller();
    nanRay.pointer_direction[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!gate.Allow(&nanRay));
    restored.primary_down=0;restored.secondary_down=0;
    assert(gate.Allow(&restored));
    std::cout<<"PASS Sitka shell pointer loss freezes guest actions; neutral rearm, pause, invalid ray and legacy ABI\n";
}
