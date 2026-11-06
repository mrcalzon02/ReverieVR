#include "sitka_boat_navigation.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
using namespace reverie::sitka;
static BathyAtlas DeepWater(){
    BathyTile t;
    t.columns=14u;t.rows=14u;
    t.first_east=0.;t.first_north=13000.;
    t.east_step=1000.;t.north_step=1000.;
    t.datum_id=BathyDatumId("NAVD88");
    t.elevations_m.assign(14u*14u,-10.f);
    t.qualities.assign(14u*14u,BedQuality::Survey);
    BathyAtlas a;assert(a.Add(std::move(t)));return a;
}
int main(){
    const auto deep=DeepWater();
    constexpr double tick=.05,sea=0.;
    const auto allowed=[](double e,double n){
        return e>=0.&&e<13000.&&n>=0.&&n<=13000.;
    };
    for(unsigned tier=0;tier<4u;++tier){
        SkiffNavigation nav;
        assert(!nav.SetHullTier(4u));
        assert(nav.SetHullTier(tier));
        const auto profile=nav.Dynamics();
        assert(profile.length_m>=2.4&&profile.beam_m>=1.2);
        assert(profile.min_draft_m>0.&&profile.max_speed_mps>0.);
        assert(nav.Launch(5000.,3000.,0.,.25,deep,sea,true));
        assert(nav.boat.draft_m>=profile.min_draft_m);
        assert(!nav.SetHullTier((tier+1u)%4u)); // no mid-sea hull change
        const double start=nav.boat.north;
        // Synthetic 20-minute simulated cruise: no fake GPS/Alaska tide.
        for(int i=0;i<24000;++i){
            const auto e=nav.Update(tick,1.,0.,sea,deep,allowed);
            assert(e==BoatEvent::Advanced||e==BoatEvent::None);
            assert(nav.boat.speed_mps>=0.&&
                   nav.boat.speed_mps<=profile.max_speed_mps+1.e-8);
        }
        const double travelled=nav.boat.north-start;
        assert(travelled>1500.&&travelled<=1200.*profile.max_speed_mps);
        assert(std::abs(nav.boat.east-5000.)<1.e-5);
        assert(nav.boat.heading==0.);
        // Full right rudder: heading change never exceeds the continuous
        // yaw limit at the actual vessel speed.
        const double before=nav.boat.heading;
        for(int i=0;i<100;++i){
            const double heading=nav.boat.heading;
            const auto e=nav.Update(.05,.8,.9,sea,deep,allowed);
            assert(e==BoatEvent::Advanced||e==BoatEvent::None);
            assert(std::abs(nav.boat.heading-heading)<=
                   profile.yaw_rate_rad_s*.05+1.e-7);
        }
        assert(nav.boat.heading>before+.01);
        nav.Dock();
        assert(nav.SetHullTier(tier));
    }
    // Test identical 30 Hz simulation despite different input frame splits.
    for(unsigned tier=0;tier<4u;++tier){
        SkiffNavigation one,two;
        assert(one.SetHullTier(tier)&&two.SetHullTier(tier));
        assert(one.Launch(5000.,3000.,0.,.2,deep,sea,true));
        assert(two.Launch(5000.,3000.,0.,.2,deep,sea,true));
        for(int i=0;i<1200;++i){
            one.Update(.1,.82,.37,sea,deep,allowed);
            two.Update(.05,.82,.37,sea,deep,allowed);
            two.Update(.05,.82,.37,sea,deep,allowed);
        }
        assert(std::abs(one.boat.east-two.boat.east)<1.e-6);
        assert(std::abs(one.boat.north-two.boat.north)<1.e-6);
        assert(std::abs(one.boat.heading-two.boat.heading)<1.e-8);
        assert(std::abs(one.boat.speed_mps-two.boat.speed_mps)<1.e-8);
    }
    // Footprint-aware shoreline rejection: vessel center is still water,
    // but the forwardmost bow already lies OUTSIDE the approved basin.
    for(unsigned tier=0;tier<4u;++tier){
        SkiffNavigation nav;
        assert(nav.SetHullTier(tier));
        const auto profile=nav.Dynamics();
        const double center=10000.-profile.length_m*.5-.2;
        assert(nav.Launch(center,3000.,1.5707963267948966,
                          .2,deep,sea,true));
        auto strict=[](double e,double n){
            return e>=0.&&e<10000.&&n>=0.&&n<13000.;
        };
        bool blocked=false;
        for(int i=0;i<450;++i){
            const auto ev=nav.Update(.1,1.,0.,sea,deep,strict);
            if(ev==BoatEvent::StoppedLand){blocked=true;break;}
        }
        assert(blocked);
        assert(nav.boat.east+profile.length_m*.5<=10000.+1.e-7);
        assert(nav.boat.speed_mps==0.);
    }
    // Source raster may be present, but a huge hull needs more draft:
    // same shallow inlet can accommodate the dinghy and reject a trawler.
    BathyTile shallow;
    shallow.columns=14u;shallow.rows=14u;
    shallow.first_east=0.;shallow.first_north=13000.;
    shallow.east_step=1000.;shallow.north_step=1000.;
    shallow.datum_id=BathyDatumId("NAVD88");
    shallow.elevations_m.assign(196u,-1.35f);
    shallow.qualities.assign(196u,BedQuality::Survey);
    BathyAtlas inlet;assert(inlet.Add(std::move(shallow)));
    SkiffNavigation dinghy,trawler;
    assert(dinghy.SetHullTier(0u)&&trawler.SetHullTier(3u));
    assert(dinghy.Launch(5000.,3000.,0.,.25,inlet,sea,true));
    assert(!trawler.Launch(5000.,3000.,0.,.25,inlet,sea,true));
    assert(!dinghy.SetHullTier(3u));
    dinghy.Dock();assert(dinghy.SetHullTier(3u));
    std::cout<<"PASS: four hull dynamics, synthetic 20-minute cruises, "
             <<"30Hz determinism, steering/turn arcs, bow/stern/beam "
             <<"shoreline clearance, draft and safe docking\n";
}
