#include "reverie_native_asset_scope.h"
#include <cassert>
#include <cstdio>
using reverie::native::assets::MaximumBytes;
int main(){
    constexpr unsigned bwmMax=8u+2048u*27u;
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/concrete_block_1.bwm")==bwmMax);
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/wooden_crate_1.bwm")==bwmMax);
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/ammo_box_mp_1.bwm")==bwmMax);
    assert(MaximumBytes("breakwater-battery",
        "native/omega-front/terrain/dirt.png")==0);
    assert(MaximumBytes("omega-front",
        "native/breakwater/beckenburg/wooden_crate_1.bwm")==0);
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/../../bad.bwm")==0);
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/foo/bar.bwm")==0);
    assert(MaximumBytes("breakwater-battery",
        "native/breakwater/beckenburg/foo.png")==0);
    assert(MaximumBytes("between-deliveries-red-ledger",
        "native/red-ledger/beckenburg/chair.rlprop")>0);
    assert(MaximumBytes("omega-front",
        "native/omega-front/terrain/ash_waste.png")>0);
    assert(MaximumBytes("classic-chess",
        "native/breakwater/beckenburg/wooden_crate_1.bwm")==0);
    std::puts("PASS: Breakwater BWM1 scoped reads plus Red Ledger/Omega compatibility");
    return 0;
}
