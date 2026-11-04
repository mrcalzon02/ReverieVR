#include "../../main/jni/native/sitka_boat_navigation.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace reverie::sitka;
static BathyAtlas Water(){
    BathyTile tile;
    tile.columns=4u;tile.rows=4u;tile.first_east=0.;
    tile.first_north=100.;tile.east_step=10.;tile.north_step=10.;
    tile.datum_id=BathyDatumId("NAVD88");
    tile.elevations_m.assign(16u,-4.f);
    tile.qualities.assign(16u,BedQuality::Survey);
    BathyAtlas atlas;assert(atlas.Add(std::move(tile)));return atlas;
}
static bool WaterMask(double e,double n){
    return e>=0.&&e<30.&&n>70.&&n<=100.;
}
static bool near(double a,double b){return std::abs(a-b)<1.e-6;}
int main(){
    const BathyAtlas atlas=Water();
    auto gauge=BoatSounderLabels(FindBoatDepth(atlas,10.,90.,0.,.5,true));
    assert(gauge[0]=="DEPTH 4.0 m");
    assert(gauge[1]=="KEEL 3.5 m SAFE / SURVEY");
    assert(gauge[2]=="SIMULATED - NOT FOR NAVIGATION");
    assert(BoatSounderLabels(FindBoatDepth(atlas,200.,90.,0.,.5,true))[0]==
           "DEPTH NO DATA");
    SkiffNavigation boat;
    assert(!boat.Launch(10.,90.,0.,.5,atlas,0.,false));
    assert(!boat.Launch(60.,90.,0.,.5,atlas,0.,true));
    assert(!boat.Launch(10.,90.,0.,.5,atlas,-4.1,true));
    assert(!boat.boat.launched);
    assert(boat.Launch(10.,90.,0.,.5,atlas,0.,true));
    assert(!boat.Launch(10.,90.,0.,.5,atlas,0.,true)); // no double launch
    assert(boat.Update(.1,0.,0.,0.,atlas,WaterMask)==BoatEvent::None);
    assert(near(boat.boat.speed_mps,0.));
    SkiffNavigation twin;
    assert(twin.Launch(10.,90.,0.,.5,atlas,0.,true));
    for(int i=0;i<10;++i)
        boat.Update(.1,1.,0.,0.,atlas,WaterMask);
    for(int i=0;i<20;++i)
        twin.Update(.05,1.,0.,0.,atlas,WaterMask);
    // Our first idle step advanced by 3 fixed ticks, so compare only
    // positions/speeds: neither has moved during the initial zero throttle.
    assert(near(boat.boat.east,twin.boat.east));
    assert(near(boat.boat.north,twin.boat.north));
    assert(near(boat.boat.speed_mps,twin.boat.speed_mps));
    const double position=boat.boat.north;
    assert(boat.Update(.1,std::numeric_limits<double>::quiet_NaN(),
                       0.,0.,atlas,WaterMask)==BoatEvent::InvalidInput);
    assert(near(boat.boat.north,position));
    // Low tide can expose the shallow bed without inventing a safety margin.
    assert(boat.Update(.1,1.,0.,-3.3,atlas,WaterMask)==BoatEvent::StoppedShallow);
    assert(near(boat.boat.north,position)&&near(boat.boat.speed_mps,0.));
    // A fixed shoreline boundary blocks boat travel; cannot sail onto land.
    bool blocked=false;
    for(int i=0;i<450;++i){
        const auto event=boat.Update(.1,1.,0.,0.,atlas,WaterMask);
        if(event==BoatEvent::StoppedLand||event==BoatEvent::StoppedNoData){
            blocked=true;break;
        }
    }
    assert(blocked);
    assert(boat.boat.north<=100.);
    // The sonar model itself is independent of camera pose or render eye.
    boat.Dock();
    assert(!boat.boat.launched);
    assert(boat.sounder.reading().status==DepthStatus::NoCoverage);
    assert(boat.Update(.1,1.,0.,0.,atlas,WaterMask)==BoatEvent::None);
    std::cout<<"PASS Sitka skiff fixed-step, tide draft, water-mask blocking and deterministic controls\n";
}
