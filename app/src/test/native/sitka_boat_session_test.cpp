#include "../../main/jni/native/sitka_boat_session.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
using namespace reverie::sitka;
static void U32(std::string& b,uint32_t v){
    for(unsigned i=0;i<4u;++i)b.push_back(static_cast<char>(v>>(i*8u)));
}
static void F64(std::string& b,double v){
    uint64_t raw=0u;std::memcpy(&raw,&v,8u);
    for(unsigned i=0;i<8u;++i)b.push_back(static_cast<char>(raw>>(i*8u)));
}
static void F32(std::string& b,float v){
    uint32_t raw=0u;std::memcpy(&raw,&v,4u);U32(b,raw);
}
static std::string Tile(unsigned col,float elevation){
    std::string bytes="SITKAB01";
    U32(bytes,3u);U32(bytes,3u);
    F64(bytes,500000.+200.*col);
    F64(bytes,6200500.);
    F64(bytes,100.);F64(bytes,100.);
    U32(bytes,BathyDatumId("NAVD88"));U32(bytes,0u);
    for(int i=0;i<9;++i){F32(bytes,elevation);bytes.push_back(char(1));}
    return bytes;
}
static std::string LaunchDocument(double east=500190.){
    std::ostringstream o;o.imbue(std::locale::classic());
    o<<std::setprecision(17)
     <<"SITKA-SKIFF-LAUNCH-1\n"
     <<"datum NAVD88\n"
     <<"water test-harbor\n"
     <<"east "<<east<<"\n"
     <<"north 6200400\n"
     <<"heading 1.5707963267948966\n"
     <<"draft 0.35\n"
     <<"radius 4\n";
    return o.str();
}
static bool Bad(const std::string& text,uint32_t datum){
    try{(void)ParseSkiffLaunch(text,datum);return false;}
    catch(const std::invalid_argument&){return true;}
}
int main(){
    // Actual native touch/joystick sign, idle deadzone and release.
    assert(SkiffForwardThrottle(-1.)==1.);
    assert(SkiffForwardThrottle(1.)==0.);
    assert(SkiffForwardThrottle(0.)==0.);
    assert(SkiffForwardThrottle(-.08)==0.);
    assert(SkiffForwardThrottle(.08)==0.);
    assert(std::abs(SkiffForwardThrottle(-.56)-.5)<1.e-10);
    assert(SkiffForwardThrottle(std::numeric_limits<double>::quiet_NaN())==0.);
    const uint32_t datum=BathyDatumId("NAVD88");
    assert(Bad(LaunchDocument(),BathyDatumId("MLLW")));
    assert(Bad(LaunchDocument()+"surprise\n",datum));
    assert(Bad("SITKA-SKIFF-LAUNCH-1\ndatum NAVD88\n"
               "water ../../secret\neast 500190\nnorth 6200400\n"
               "heading 0\ndraft .35\nradius 4\n",datum));
    assert(Bad(LaunchDocument(1e9),datum));
    const auto berth=ParseSkiffLaunch(LaunchDocument(),datum);
    assert(berth.water_id=="test-harbor");
    const double ox=500000.,oz=6200000.;
    std::vector<Water> waters(1u);
    waters[0].id="test-harbor";
    waters[0].shape="SQUARE";
    waters[0].x=200.;waters[0].z=400.;
    waters[0].width=600.;waters[0].depth=300.;
    waters[0].y=0.f;
    assert(SitkaWorldWater(waters,ox,oz,berth.east,berth.north));
    assert(!SitkaWorldWater(waters,ox,oz,berth.east,6200800.));
    assert(!SitkaWorldWater(waters,0.,0.,berth.east,berth.north));
    assert(!SitkaNamedWorldWater(waters,"other",ox,oz,
                                 berth.east,berth.north));
    SeabedGeneration settings;
    settings.datum_id=datum;
    settings.water_reference_m=0.;
    settings.seed=44556677u;
    settings.influence_radius_m=1000.;
    ProceduralSeabed generated(settings);
    const auto dir=ParseBathyStreamIndex(
        "SITKA-BATHY-INDEX-2\n"
        "datum NAVD88\n"
        "tiles 2\n"
        "bathymetry-r00-c00.sitkab 500000 6200500 100 100 3 3\n"
        "bathymetry-r00-c01.sitkab 500200 6200500 100 100 3 3\n");
    BathyStreamCache cache(dir);
    auto reader=[&](const std::string& filename,std::string& bytes){
        if(filename=="bathymetry-r00-c00.sitkab")
            bytes=Tile(0u,-10.f);
        else if(filename=="bathymetry-r00-c01.sitkab")
            bytes=Tile(1u,-3.f);
        else return false;
        return true;
    };
    SitkaSkiffSession session(berth);
    assert(!session.Active());
    assert(session.InstrumentLines()[0]=="DEPTH NO DATA");
    assert(session.Update(.1,0.,0.,true,500100.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::Rejected);
    // A second held frame is not another launch attempt.
    assert(session.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::None);
    session.Update(.1,0.,0.,false,500190.,6200400.,
                   cache,generated,waters,ox,oz);
    // No loaded NOAA tile yet: real unsurveyed game water still navigable.
    assert(session.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::Launched);
    assert(session.Active());
    assert(session.InstrumentLines()[1].find("PROCEDURAL")!=std::string::npos);
    assert(session.InstrumentLines()[2]=="SIMULATED - NOT FOR NAVIGATION");
    assert(std::abs(session.Pose().east-500190.)<1e-8);
    session.Update(.1,0.,0.,false,500190.,6200400.,
                   cache,generated,waters,ox,oz);
    assert(cache.Pump(session.Pose().east,session.Pose().north,4.,0.,reader));
    // The skiff owns travel; a wildly different headset pose must NEVER
    // make the boat jump or choose a different bathymetry tile.
    for(int i=0;i<60;++i){
        const auto event=session.Update(.1,1.,0.,false,
            1000.+i*10.,-2000.,cache,generated,waters,ox,oz);
        assert(event!=SkiffSessionEvent::InvalidInput);
        const bool loaded=cache.Pump(session.Pose().east,session.Pose().north,
                                    session.EastVelocity(),
                                    session.NorthVelocity(),reader);
        (void)loaded;
    }
    assert(session.Active());
    assert(session.Pose().east>500190.);
    assert(std::abs(session.Pose().north-6200400.)<1e-7);
    assert(session.Pose().speed_mps<=4.);
    assert(session.InstrumentLines()[1].find("SURVEY")!=std::string::npos);
    // The source directory was prefetched, then chosen over procedural
    // bathymetry when crossing x=500200, with no second app-side I/O.
    assert(cache.stats().loaded==2u);
    assert(cache.stats().resident_tiles==2u);
    assert(session.Pose().east>500200.);
    assert(session.Sounder().reading().quality==BedQuality::Survey);
    // Cannot remotely dock from an arbitrary location while underway.
    assert(session.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::Rejected);
    assert(session.Active());
    // Source data can reveal a newly unsafe shoal; the game must stop
    // rather than infer the old procedural depth across source priority.
    BathyStreamCache shoals(dir);
    auto shallow=[&](const std::string& filename,std::string& bytes){
        bytes=filename=="bathymetry-r00-c00.sitkab"?
            Tile(0u,-.4f):Tile(1u,-.4f);
        return true;
    };
    assert(shoals.Pump(500190.,6200400.,0.,0.,shallow));
    SitkaSkiffSession grounded(berth);
    assert(grounded.Update(.1,0.,0.,true,500190.,6200400.,
        shoals,generated,waters,ox,oz)==SkiffSessionEvent::Rejected);
    assert(!grounded.Active());
    // Calmly dock at the berth; no duplicate action during a held chord.
    SitkaSkiffSession docked(berth);
    assert(docked.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::Launched);
    assert(docked.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::None);
    docked.Update(.1,0.,0.,false,500190.,6200400.,
                  cache,generated,waters,ox,oz);
    assert(docked.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::Docked);
    assert(!docked.Active());
    assert(docked.InstrumentLines()[0]=="DEPTH NO DATA");
    assert(docked.Update(.1,0.,0.,true,500190.,6200400.,
        cache,generated,waters,ox,oz)==SkiffSessionEvent::None);
    std::cout<<"PASS Sitka projected-origin water mask, author-approved skiff launch, held button gating, sourced/procedural sounder, boat-authority prefetch, grounding and docking\n";
}
