#ifndef REVERIE_SITKA_BOAT_ANGLING_H
#define REVERIE_SITKA_BOAT_ANGLING_H
// Boat rod-fishing reuses the exact existing Sitka rod, fish journal and
// editor-authorized fishing sites. Never create a synthetic catch location:
// real geography requires surveyed sounder depth, and only the explicitly
// NON-GEOGRAPHIC practice range may use its procedural depth.
#include "sitka_boat_navigation.h"
#include "sitka_environment.h"
#include "sitka_fishing_spots.h"
#include "sitka_fishing_session.h"
#include <cmath>
#include <limits>
#include <string>
#include <vector>
namespace reverie { namespace sitka {
enum class BoatRodStatus : uint8_t {
    NotAboard, Underway, NoDepth, NoAuthoredSite, Ready
};
struct BoatRodTarget {
    BoatRodStatus status=BoatRodStatus::NotAboard;
    FishingCastTarget cast{};
};
// The site parser validates these fields, but game-private callers and test
// fixtures must never turn a malformed in-memory record into an accepted cast.
inline bool ValidBoatRodSite(const FishingSpot& site) {
    return std::isfinite(site.east)&&std::isfinite(site.north)&&
           std::isfinite(site.radius)&&std::isfinite(site.depth_metres)&&
           site.radius>=.3f&&site.radius<=12.f&&
           site.depth_metres>=.3f&&site.depth_metres<=200.f;
}
// Short, rounded, in-game helm guidance. Only the current authored water
// footprint and real authored sites qualify; this does NOT infer a safe route,
// ocean continuity, bathymetry, GPS fix or a real-world marine bearing.
// Port/starboard are relative to the simulated bow, not the headset gaze.
inline std::string BoatRodSiteCue(const BoatPose& boat,
    const std::vector<FishingSpot>& spots,const std::vector<Water>& waters,
    double terrain_east,double terrain_north) {
    if(!boat.launched||!std::isfinite(boat.heading)||
       std::abs(boat.heading)>6.283185307179586||
       !std::isfinite(terrain_east)||!std::isfinite(terrain_north))
        return {};
    const double east=boat.east-terrain_east,north=boat.north-terrain_north;
    if(!std::isfinite(east)||!std::isfinite(north)||
       std::abs(east)>20000.||std::abs(north)>20000.)return {};
    double best_dist2=std::numeric_limits<double>::infinity();
    double best_de=0.,best_dn=0.,best_radius=0.;
    bool found=false;
    for(const FishingSpot& site:spots) {
        if(!ValidBoatRodSite(site))continue;
        const double de=double(site.east)-east,dn=double(site.north)-north;
        const double dist2=de*de+dn*dn;
        if(!std::isfinite(dist2)||dist2>=best_dist2)continue;
        for(const Water& water:waters) {
            if(water.id!=site.water_id||
               !FootprintContains(water,static_cast<float>(east),
                                        static_cast<float>(north))||
               !FootprintContains(water,site.east,site.north))continue;
            found=true;best_dist2=dist2;best_de=de;best_dn=dn;
            best_radius=site.radius;
            break;
        }
    }
    if(!found)return {};
    if(best_dist2<=best_radius*best_radius)return "ROD SITE HERE";
    const double forward=std::sin(boat.heading)*best_de+
                         std::cos(boat.heading)*best_dn;
    const double right=std::cos(boat.heading)*best_de-
                       std::sin(boat.heading)*best_dn;
    const char* direction=std::abs(forward)>=std::abs(right)?
        (forward>=0.?"AHEAD":"ASTERN"):
        (right>=0.?"STARBOARD":"PORT");
    // Whole metres keep GL text uploads from churning at every movement tick.
    return "ROD SITE "+std::to_string(
        static_cast<unsigned>(std::ceil(std::sqrt(best_dist2))))+
        "M "+direction;
}
inline BoatRodTarget BoatRodCastTarget(const BoatPose& boat,double throttle,
    const DepthReading& sounding,bool practice,
    const std::vector<FishingSpot>& spots,const std::vector<Water>& waters,
    double terrain_east,double terrain_north) {
    BoatRodTarget result;
    if(!boat.launched)return result;
    result.status=BoatRodStatus::Underway;
    if(!std::isfinite(boat.speed_mps)||!std::isfinite(throttle)||
       boat.speed_mps<0.||boat.speed_mps>.15||throttle>.01||throttle<0.)
        return result;
    result.status=BoatRodStatus::NoDepth;
    if(sounding.status!=DepthStatus::Safe||
       !std::isfinite(sounding.water_depth_m)||
       sounding.water_depth_m<.3||sounding.water_depth_m>200.||
       !(sounding.quality==BedQuality::Survey||
         (practice&&sounding.quality==BedQuality::Procedural)))
        return result;
    const double east=boat.east-terrain_east,north=boat.north-terrain_north;
    if(!std::isfinite(east)||!std::isfinite(north)||
       std::abs(east)>20000.||std::abs(north)>20000.)return result;
    result.status=BoatRodStatus::NoAuthoredSite;
    for(const FishingSpot& spot:spots){
        const double de=east-spot.east,dn=north-spot.north;
        if(!ValidBoatRodSite(spot)||!std::isfinite(de)||!std::isfinite(dn)||
           de*de+dn*dn>double(spot.radius)*spot.radius)continue;
        for(const Water& water:waters) {
            if(water.id!=spot.water_id||
               !FootprintContains(water,static_cast<float>(east),
                                        static_cast<float>(north)))continue;
            result.cast={true,spot.depth_metres,
                         static_cast<float>(east),static_cast<float>(north)};
            result.status=BoatRodStatus::Ready;
            return result;
        }
    }
    return result;
}
inline std::string BoatRodHint(BoatRodStatus status,const Fishing& fishing,
                               const std::string& site_cue={}){
    switch(fishing.phase){
        case FishingPhase::Waiting:return "WAIT FOR BITE / VOL UP: CANCEL";
        case FishingPhase::Bite:return "BITE! SELECT TO HOOK";
        case FishingPhase::Reeling:
            return "REEL "+std::to_string(unsigned(fishing.reel*100.f))+
                   "% / TENSION "+std::to_string(unsigned(fishing.tension*100.f))+"%";
        case FishingPhase::Landed:
            return std::string("LANDED ")+kFishNames[unsigned(fishing.hooked)]+
                   " / SELECT: STOW";
        case FishingPhase::Ready:break;
    }
    switch(status){
        case BoatRodStatus::NotAboard:return "BOAT ROD UNAVAILABLE";
        case BoatRodStatus::Underway:return site_cue.empty()?
            "SLOW DOWN / THRUST IDLE TO CAST":site_cue+" / IDLE TO CAST";
        case BoatRodStatus::NoDepth:return "FISHING NEEDS SURVEY SOUNDING";
        case BoatRodStatus::NoAuthoredSite:return site_cue.empty()?
            "SAIL TO AN AUTHORED FISHING SITE":site_cue;
        case BoatRodStatus::Ready:return "SELECT: CAST ROD / VOL UP: CANCEL";
    }
    return "FISHING UNAVAILABLE";
}
}} // reverie::sitka
#endif
