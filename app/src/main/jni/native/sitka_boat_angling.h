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
        if(!std::isfinite(spot.radius)||spot.radius<=0.f||
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
inline std::string BoatRodHint(BoatRodStatus status,const Fishing& fishing){
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
        case BoatRodStatus::Underway:return "SLOW DOWN / THRUST IDLE TO CAST";
        case BoatRodStatus::NoDepth:return "FISHING NEEDS SURVEY SOUNDING";
        case BoatRodStatus::NoAuthoredSite:return "SAIL TO AN AUTHORED FISHING SITE";
        case BoatRodStatus::Ready:return "SELECT: CAST ROD / VOL UP: CANCEL";
    }
    return "FISHING UNAVAILABLE";
}
}} // reverie::sitka
#endif
