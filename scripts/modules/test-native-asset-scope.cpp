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
        "native/shared/v1/models/psx/wooden_crate_1.bwm")==bwmMax);
    assert(MaximumBytes("between-deliveries-red-ledger",
        "native/shared/v1/models/psx/wooden_crate_1.bwm")==bwmMax);
    assert(MaximumBytes("omega-front",
        "native/shared/v1/materials/coastal/quay_concrete.png")==16384u);
    assert(MaximumBytes("classic-chess",
        "native/shared/v1/materials/coastal/quay_concrete.png")==0u);
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
    assert(MaximumBytes("sitka","sitka/boat/launch.txt")==4096u);
    assert(MaximumBytes("sitka","sitka/structures/oconnell.bridge")==8192u);
    assert(MaximumBytes("sitka","sitka/structures/oconnell_side.png")==131072u);
    assert(MaximumBytes("sitka",
        "native/shared/v1/materials/coastal/quay_concrete.png")==16384u);
    assert(MaximumBytes("sitka",
        "native/shared/v1/materials/coastal/reinforced_concrete.png")==16384u);
    assert(MaximumBytes("sitka",
        "native/shared/v1/materials/coastal/dock_planks.png")==0u);
    assert(MaximumBytes("sitka",
        "native/shared/v1/models/psx/wooden_crate_1.bwm")==0u);
    assert(MaximumBytes("breakwater-battery",
        "sitka/structures/oconnell_side.png")==0u);
    assert(MaximumBytes("sitka",
        "sitka/structures/../oconnell_side.png")==0u);
    assert(MaximumBytes("sitka",
        "sitka/bathymetry/bathymetry-r01-c02.sitkab")==
        56u+513u*513u*5u);
    assert(MaximumBytes("sitka",
        "sitka/bathymetry/../fishing/sites.txt")==0u);
    assert(MaximumBytes("omega-front",
        "sitka/bathymetry/bathymetry-r01-c02.sitkab")==0u);
    std::puts("PASS: Breakwater BWM1 scoped reads plus Red Ledger/Omega compatibility");
    return 0;
}
