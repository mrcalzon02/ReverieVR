#include "sitka_shore_stream.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>
using namespace reverie::sitka;

int main(){
    auto features=ParseShoreLayout(
        "SITKA-SHORE-1\n"
        "P|lincoln_real_outline|storefront|1|7|0,0;16,0;16,4;11,4;11,9;0,9\n"
        "B|crescent_dock|pier|400|0|0|4|20|1|0\n"
        "R|bridge_ramp|bridge_ramp|1050|0|6|7|22|1|0|4\n");
    // Production fallback: real footprints do not authorize fabricated
    // storefront window layouts or canopy placement.
    ShoreStreamPlanner production(features);
    assert(production.Update(12,5,0,0));
    production.Commit();
    assert(production.tier(0)==ShoreStreamPlanner::Massing);
    assert(production.detailed_count()==0u);
    // Explicitly approved illustrative P1 is exercised only in test code.
    ShoreStreamPlanner plan(features,{"lincoln_real_outline"});
    assert(!plan.Update(NAN,0,0,0));
    assert(plan.Update(12,5,0,0));
    const auto detailed=plan.BuildPending();
    assert(detailed.vertices.size()>0u&&detailed.indices.size()>0u);
    plan.Commit();
    assert(plan.visible_count()==2u && plan.detailed_count()==1u);
    assert(plan.tier(0)==ShoreStreamPlanner::Detailed);
    assert(plan.tier(1)==ShoreStreamPlanner::Massing);
    assert(plan.tier(2)==ShoreStreamPlanner::Hidden);
    const auto revision=plan.revision();
    for(int i=0;i<100;++i)assert(!plan.Update(13.,6.,0,0));
    assert(plan.revision()==revision);
    // Geospatial origin is double precision, not a guessed pixel grid.
    assert(!plan.Update(1000000.+12,1000000.+5,0,0));
    assert(!plan.Update(1000000.+12,1000000.+5,1000000.,1000000.));
    // 100m is inside near-exit 112m, so keep P1 details.
    assert(!plan.Update(116,5,0,0));
    assert(plan.Update(150,5,0,0));
    const auto mass=plan.BuildPending();
    assert(mass.vertices.size()<detailed.vertices.size());
    plan.Commit();
    assert(plan.tier(0)==ShoreStreamPlanner::Massing);
    // One failed upload must preserve old resident tiers and retry later.
    assert(plan.Update(800,0,0,0));
    const auto pending=plan.BuildPending();
    assert(!pending.indices.empty());
    plan.Reject();
    assert(plan.tier(0)==ShoreStreamPlanner::Massing);
    assert(plan.retry_frames()==ShoreStreamPlanner::kRetryFrames);
    for(unsigned i=0;i<ShoreStreamPlanner::kRetryFrames;++i)
        assert(!plan.Update(800,0,0,0));
    assert(plan.Update(800,0,0,0));
    plan.Commit();
    assert(plan.tier(0)==ShoreStreamPlanner::Hidden);
    assert(plan.visible_count()==2u);
    assert(plan.Update(1050,0,0,0));
    plan.Commit();
    assert(plan.tier(2)==ShoreStreamPlanner::Detailed);
    // Scene generation is stable across resets, including concave rooftops.
    plan.Reset();
    assert(plan.Update(12,5,0,0));
    const auto restored=plan.BuildPending();
    assert(restored.vertices.size()==detailed.vertices.size());
    assert(restored.indices==detailed.indices);
    for(size_t i=0;i<restored.vertices.size();++i){
        const auto& a=restored.vertices[i],&b=detailed.vertices[i];
        assert(a.x==b.x&&a.y==b.y&&a.z==b.z);
        assert(a.r==b.r&&a.g==b.g&&a.b==b.b);
    }
    plan.Commit();
    assert(plan.revision()==1u);
    // Rotated 20x2 pier intersects the view near its corner, not only centre.
    ShoreFeature rotated{};
    rotated.material="pier";rotated.width=20;rotated.depth=2;
    rotated.yaw=3.14159265f/4;rotated.x=80;rotated.z=80;
    auto bounds=ShoreFeatureBounds(rotated);
    assert(bounds.low_x<74 && bounds.high_x>86);
    assert(bounds.low_z<74 && bounds.high_z>86);
    assert(ShoreBoundDistanceSquared(bounds,80,80)==0.);
    return 0;
}
