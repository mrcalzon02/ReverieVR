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
                             depth.quality==BedQuality::Composite?"COMPOSITE":
                             depth.quality==BedQuality::Procedural?"PROCEDURAL":"UNKNOWN";
    return {{water.str(),clearance.str()+" / "+source,disclaimer}};
}

// Physical vessel classes share the same 30Hz authority but have distinct
// game-tuned turn, inertia, footprint and keel-clearance requirements.
struct VesselDynamics {
    double length_m,beam_m,min_draft_m;
    double max_speed_mps,accel_mps2,brake_mps2,yaw_rate_rad_s;
};
inline constexpr std::array<VesselDynamics,4> kVesselDynamics={{
    {2.4,1.2,.24,2.5,.65,1.70,.85},  // open dinghy
    {6.1,2.3,.35,4.0,.80,1.50,.60},  // owned fishing skiff
    {8.0,2.9,.65,4.9,.60,1.15,.37},  // midgame pilothouse
    {14.4,4.8,1.10,5.2,.40,.95,.18} // commercial crab boat
}};
inline bool BoatInputValid(double axis){
    return std::isfinite(axis)&&axis>=-1.&&axis<=1.;
}
class SkiffNavigation {
public:
    BoatPose boat;
    BoatDepthFinder sounder;
    // Tier selection is permitted only at dock; it does not teleport or
    // transform the vessel while the player is operating it.
    bool SetHullTier(unsigned tier){
        if(boat.launched||tier>=kVesselDynamics.size())return false;
        tier_=tier;dynamics_=kVesselDynamics[tier];return true;
    }
    unsigned HullTier()const{return tier_;}
    const VesselDynamics& Dynamics()const{return dynamics_;}
    template<class BathySampler>
    bool Launch(double east,double north,double heading,
                double draft_m,const BathySampler& atlas,
                double surface_elevation_m,bool approved_water){
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(heading)||!std::isfinite(draft_m)||
           draft_m<=0.||draft_m>2.0||boat.launched)return false;
        const double actual_draft=std::max(draft_m,dynamics_.min_draft_m);
        const DepthReading reading=FindBoatDepth(atlas,east,north,
                surface_elevation_m,actual_draft,approved_water);
        if(reading.status!=DepthStatus::Safe || reading.under_keel_m<.5)
            return false;
        boat={east,north,heading,0.,actual_draft,true};
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
            const double goal=std::max(0.,throttle)*dynamics_.max_speed_mps;
            const double rate=(goal>boat.speed_mps?
                dynamics_.accel_mps2:dynamics_.brake_mps2)*tick;
            const double speed=boat.speed_mps+
                std::clamp(goal-boat.speed_mps,-rate,rate);
            if(speed<1.e-10){boat.speed_mps=0.;continue;}
            // At a standstill, turning the wheel changes the rudder but
            // cannot spin a commercial vessel around its center.
            const double new_heading=boat.heading+rudder*tick*
                dynamics_.yaw_rate_rad_s*std::min(1.,speed/
                                                 dynamics_.max_speed_mps);
            const double sine=std::sin(new_heading),cosine=std::cos(new_heading);
            const double east=boat.east+sine*speed*tick;
            const double north=boat.north+cosine*speed*tick;
            // Hull footprint rather than center point: bow, stern and both
            // beams all need approved water and under-keel depth. A large
            // trawler must stop before its BOW crosses a sourced shoreline.
            const std::array<std::array<double,2>,5> samples={{
                {{0.,0.}},
                {{0.,dynamics_.length_m*.5}},
                {{0.,-dynamics_.length_m*.5}},
                {{dynamics_.beam_m*.5,0.}},
                {{-dynamics_.beam_m*.5,0.}}
            }};
            BoatEvent rejected=BoatEvent::None;
            for(const auto& point:samples){
                const double se=east+cosine*point[0]+sine*point[1];
                const double sn=north-sine*point[0]+cosine*point[1];
                if(!check_water(se,sn)){
                    rejected=BoatEvent::StoppedLand;break;
                }
                const DepthReading reading=FindBoatDepth(
                    atlas,se,sn,sea_surface_elevation_m,boat.draft_m,true);
                if(reading.status!=DepthStatus::Safe||
                   reading.under_keel_m<.5){
                    rejected=reading.status==DepthStatus::NoCoverage?
                        BoatEvent::StoppedNoData:
                        (reading.status==DepthStatus::Dry||
                         reading.status==DepthStatus::OutsideWater?
                         BoatEvent::StoppedLand:BoatEvent::StoppedShallow);
                    break;
                }
            }
            if(rejected!=BoatEvent::None){
                boat.speed_mps=0.;
                pending_seconds_=0.; // Never build up thrust into land.
                event=rejected;
                break; // Preserve last safe position AND heading.
            }
            if(east!=boat.east||north!=boat.north)event=BoatEvent::Advanced;
            boat.heading=new_heading;
            boat.speed_mps=speed;
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
    unsigned tier_=1u; // prior SkiffNavigation behavior: the fishing skiff
    VesselDynamics dynamics_=kVesselDynamics[1];
};
}}
#endif
