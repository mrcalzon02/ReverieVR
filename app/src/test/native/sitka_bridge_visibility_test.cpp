#include "sitka_bridge_visibility.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <vector>
using namespace reverie::sitka;
int main(){
    BridgePlaneMesh mesh=BuildBridgePlanes(
        {{0.f,20.f,0.f},{0.f,22.f,100.f},{0.f,20.f,400.f}},
        11.5824f,.6f,30.48f);
    BridgeVisibility view(mesh);
    assert(!view.visible());
    assert(!view.Update(2000,2000,0,0));
    // Right next to the far end of the sourced structure, not its centre.
    assert(view.Update(50,400,0,0));
    assert(view.transitions()==1u);
    // Hysteresis preserves visibility while crossing the enter band.
    assert(view.Update(1260,400,0,0));
    assert(view.transitions()==1u);
    assert(!view.Update(1480,400,0,0));
    assert(view.transitions()==2u);
    assert(!view.Update(1260,400,0,0));
    // World projection origin is subtracted exactly once.
    assert(view.Update(480050,6300400,480000,6300000));
    assert(view.visible());
    assert(view.Update(NAN,0,0,0)); // invalid camera input keeps prior tier
    assert(view.Update(480050,6300400,480000,6300000));
    assert(view.transitions()==3u);
    view.Reset();
    assert(!view.visible()&&view.transitions()==0u);
    assert(!view.Update(5000,0,0,0));
    BridgePlaneMesh empty;
    bool rejected=false;
    try{BridgeVisibility invalid(empty);(void)invalid;}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    return 0;
}
