#ifndef REVERIE_SITKA_BOAT_NAVIGATION_H
#define REVERIE_SITKA_BOAT_NAVIGATION_H
// Deterministic, head-comfort-safe skiff simulation for Sitka's future boat mode.
// Boat mechanics are NOT actual navigational guidance; no user/device GPS used.
// Every movement sample must be inside approved water and has seabed coverage.
#include "sitka_bathymetry.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <cmath>
#include <cstdint>
namespace reverie { namespace sitka {
enum class BoatEvent : uint8_t {
    None, Launched, Advanced, StoppedNoData, StoppedLand,
    StoppedShallow, InvalidInput
};
struct BoatPose {
    double east=0.,north=0.,heading=0.;
    double speed_mps=0.;
    double draft_m=.35;   // gameplay vessel attribute, not real craft dimensions
    bool launched=false;
};
// A deterministic instrument readout model for the future Lato VR console.
// The renderer will show these strings only when a real skiff is launched.
inline std::array<std::string,3> BoatSounderLabels(const DepthReading& depth){
    const std::string disclaimer="SIMULATED - NOT FOR NAVIGATION";
    if(depth.status==DepthStatus::NoCoverage)
        return {{"DEPTH NO DATA","SOURCE UNAVAILABLE",disclaimer}};
    if(depth.status==DepthStatus::OutsideWater)
        return {{"NOT IN WATER","DEPTH UNAVAILABLE",disclaimer}};
    if(depth.status==DepthStatus::Dry)
        return {{"SEABED EXPOSED","BOAT CANNOT PASS",disclaimer}};
    std::ostringstream water,clearance;
    water.imbue(std::locale::classic());
    clearance.imbue(std::locale::classic());
    water<<std::fixed<<std::setprecision(1)
         <<"DEPTH "<<depth.water_depth_m<<" m";
    clearance<<std::fixed<<std::setprecision(1)
        <<"KEEL "<<depth.under_keel_m<<" m"
        <<(depth.status==DepthStatus::UnderKeel?" SHALLOW":" SAFE");
    const std::string source=depth.quality==BedQuality::Survey?"SURVEY":
                             depth.quality==BedQuality::Composite?"COMPOSITE":"UNKNOWN";
    return {{water.str(),clearance.str()+" / "+source,disclaimer}};
}

inline bool BoatInputValid(double axis){
    return std::isfinite(axis)&&axis>=-1.&&axis<=1.;
}
class SkiffNavigation {
public:
    BoatPose boat;
    BoatDepthFinder sounder;
    template<class BathySampler>
    bool Launch(double east,double north,double heading,
                double draft_m,const BathySampler& atlas,
                double surface_elevation_m,bool approved_water){
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(heading)||!std::isfinite(draft_m)||
           draft_m<=0.||draft_m>2.0||boat.launched)return false;
        const DepthReading reading=FindBoatDepth(atlas,east,north,
                surface_elevation_m,draft_m,approved_water);
        if(reading.status!=DepthStatus::Safe || reading.under_keel_m<.5)
            return false;
        boat={east,north,heading,0.,draft_m,true};
        pending_seconds_=0.;sounder.Reset();
        return true;
    }
    void Dock(){boat.launched=false;boat.speed_mps=0.;
                pending_seconds_=0.;sounder.Reset();}
    // Caller runs update once per simulation tick; never from render_eye.
    // CheckWater must be a geographic water-mask test at the candidate pose.
    template<class BathySampler,class CheckWater>
    BoatEvent Update(double dt,double throttle,double rudder,
                     double sea_surface_elevation_m,const BathySampler& atlas,
                     CheckWater&& check_water){
        if(!boat.launched)return BoatEvent::None;
        if(!std::isfinite(dt)||dt<=0.||dt>.1||
           !BoatInputValid(throttle)||!BoatInputValid(rudder)||
           !std::isfinite(sea_surface_elevation_m))return BoatEvent::InvalidInput;
        constexpr double tick=1./30.;
        pending_seconds_+=dt;
        BoatEvent event=BoatEvent::None;
        while(pending_seconds_+1.e-10>=tick){
            pending_seconds_-=tick;
            boat.heading+=rudder*tick*.60; // no forced camera roll/yaw
            const double goal=std::max(0.,throttle)*4.; // gentle 4m/s cap
            const double change=(goal>boat.speed_mps?.8:1.5)*tick;
            boat.speed_mps+=std::clamp(goal-boat.speed_mps,-change,change);
            const double east=boat.east+std::sin(boat.heading)*boat.speed_mps*tick;
            const double north=boat.north+std::cos(boat.heading)*boat.speed_mps*tick;
            const bool water=check_water(east,north);
            const DepthReading future=FindBoatDepth(atlas,east,north,
                sea_surface_elevation_m,boat.draft_m,water);
            if(future.status!=DepthStatus::Safe||future.under_keel_m<.5){
                boat.speed_mps=0.;
                pending_seconds_=0.; // Do not queue thrust against a blocked shoreline.
                if(future.status==DepthStatus::OutsideWater||future.status==DepthStatus::Dry)
                    event=BoatEvent::StoppedLand;
                else if(future.status==DepthStatus::NoCoverage)
                    event=BoatEvent::StoppedNoData;
                else event=BoatEvent::StoppedShallow;
                break;
            }
            if(east!=boat.east||north!=boat.north)event=BoatEvent::Advanced;
            boat.east=east;boat.north=north;
        }
        // Sounding is a bounded quarter-second ping, independent of eye draws.
        sounder.Update(dt,atlas,boat.east,boat.north,
                       sea_surface_elevation_m,boat.draft_m,
                       check_water(boat.east,boat.north));
        return event;
    }
private:
    double pending_seconds_=0.;
};
}}
#endif
