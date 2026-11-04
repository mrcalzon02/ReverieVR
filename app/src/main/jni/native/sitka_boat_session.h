#ifndef REVERIE_SITKA_BOAT_SESSION_H
#define REVERIE_SITKA_BOAT_SESSION_H
// Source-only Sitka skiff mode: projected geographic authority, sampled
// surveyed/procedural bathymetry and deterministic 30 Hz boat motion.
// No user GPS, physical sonar, inferred boat start or per-eye mutation.
#include "sitka_boat_navigation.h"
#include "sitka_bathymetry_stream.h"
#include "sitka_procedural_seabed.h"
#include "sitka_environment.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace reverie { namespace sitka {
struct SkiffLaunch {
    uint32_t datum_id=0u;
    std::string water_id;
    double east=0.,north=0.,heading=0.,draft_m=.35,radius_m=4.;
};
inline bool ValidSkiffWaterId(const std::string& value){
    if(value.empty()||value.size()>48u)return false;
    for(char c:value)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return false;
    return true;
}
inline SkiffLaunch ParseSkiffLaunch(const std::string& source,
                                   uint32_t expected_datum){
    constexpr char magic[]="SITKA-SKIFF-LAUNCH-1\n";
    if(source.size()<sizeof(magic)||source.size()>4096u||
       source.compare(0,sizeof(magic)-1u,magic)!=0)
        throw std::invalid_argument("invalid Sitka skiff launch version");
    std::istringstream in(source.substr(sizeof(magic)-1u));
    in.imbue(std::locale::classic());
    std::string key,datum,trailing;
    SkiffLaunch out;
    if(!(in>>key>>datum)||key!="datum"||
       (out.datum_id=BathyDatumId(datum))==0u||
       out.datum_id!=expected_datum)
        throw std::invalid_argument("Sitka skiff launch datum mismatch");
    if(!(in>>key>>out.water_id)||key!="water"||
       !ValidSkiffWaterId(out.water_id))
        throw std::invalid_argument("invalid skiff water reference");
    if(!(in>>key>>out.east)||key!="east"||
       !(in>>key>>out.north)||key!="north"||
       !(in>>key>>out.heading)||key!="heading"||
       !(in>>key>>out.draft_m)||key!="draft"||
       !(in>>key>>out.radius_m)||key!="radius"||
       (in>>trailing)||
       !std::isfinite(out.east)||!std::isfinite(out.north)||
       !std::isfinite(out.heading)||!std::isfinite(out.draft_m)||
       !std::isfinite(out.radius_m)||
       std::abs(out.east)>1.e8||std::abs(out.north)>1.e8||
       std::abs(out.heading)>6.283185307179586||
       out.draft_m<=0.||out.draft_m>2.||
       out.radius_m<.5||out.radius_m>12.)
        throw std::invalid_argument("invalid skiff launch pose/draft");
    return out;
}
// All authored Sitka water footprints are LOCAL to the terrain origin,
// while bathymetry, launch and skiff position are projected world metres.
inline const Water* SitkaWorldWater(const std::vector<Water>& waters,
    double origin_east,double origin_north,
    double projected_east,double projected_north){
    if(!std::isfinite(origin_east)||!std::isfinite(origin_north)||
       !std::isfinite(projected_east)||!std::isfinite(projected_north))
        return nullptr;
    const double east=projected_east-origin_east;
    const double north=projected_north-origin_north;
    if(std::abs(east)>20000.||std::abs(north)>20000.)return nullptr;
    for(const Water& water:waters)
        if(FootprintContains(water,static_cast<float>(east),
                            static_cast<float>(north)))
            return &water;
    return nullptr;
}
inline const Water* SitkaNamedWorldWater(const std::vector<Water>& waters,
    const std::string& id,double origin_east,double origin_north,
    double projected_east,double projected_north){
    const double east=projected_east-origin_east;
    const double north=projected_north-origin_north;
    if(!std::isfinite(east)||!std::isfinite(north)||
       std::abs(east)>20000.||std::abs(north)>20000.)return nullptr;
    for(const Water& water:waters)
        if(water.id==id&&FootprintContains(water,static_cast<float>(east),
                                           static_cast<float>(north)))
            return &water;
    return nullptr;
}
enum class SkiffSessionEvent : uint8_t {
    None, Launched, Docked, Rejected, Advanced, BlockedLand,
    BlockedShallow, BlockedNoData, InvalidInput
};
class SitkaSkiffSession {
public:
    explicit SitkaSkiffSession(SkiffLaunch launch)
        : launch_(std::move(launch)){
        if(!launch_.datum_id||!ValidSkiffWaterId(launch_.water_id)||
           !std::isfinite(launch_.east)||!std::isfinite(launch_.north)||
           !std::isfinite(launch_.heading)||!std::isfinite(launch_.draft_m)||
           !std::isfinite(launch_.radius_m)||launch_.draft_m<=0.||
           launch_.draft_m>2.||launch_.radius_m<.5||launch_.radius_m>12.)
            throw std::invalid_argument("invalid Sitka skiff berth");
    }
    bool Active()const{return navigation_.boat.launched;}
    const BoatPose& Pose()const{return navigation_.boat;}
    const BoatDepthFinder& Sounder()const{return navigation_.sounder;}
    std::array<std::string,3> InstrumentLines()const{
        return BoatSounderLabels(navigation_.sounder.reading());
    }
    double EastVelocity()const{
        return Active()?std::sin(Pose().heading)*Pose().speed_mps:0.;
    }
    double NorthVelocity()const{
        return Active()?std::cos(Pose().heading)*Pose().speed_mps:0.;
    }
    const SkiffLaunch& LaunchPoint()const{return launch_;}
    // Primary+secondary chord is provisional source-only mode action.
    // Rising edge only: holding buttons must never relaunch/dock twice.
    // The guest must suppress these held buttons from fishing controls.
    // Successful dock requires returning near the authored berth slowly.
    SkiffSessionEvent Update(double dt,double throttle,double rudder,
        bool both_down,double listener_east,double listener_north,
        BathyStreamCache& sourced,const ProceduralSeabed& procedural,
        const std::vector<Water>& waters,double terrain_origin_east,
        double terrain_origin_north){
        if(!std::isfinite(dt)||dt<=0.||dt>.1||
           !BoatInputValid(throttle)||!BoatInputValid(rudder)||
           sourced.directory().datum_id!=launch_.datum_id||
           procedural.settings().datum_id!=launch_.datum_id)
            return SkiffSessionEvent::InvalidInput;
        const bool toggle=both_down&&!chord_held_;
        chord_held_=both_down;
        const auto water=[&](double east,double north){
            return SitkaWorldWater(waters,terrain_origin_east,
                terrain_origin_north,east,north)!=nullptr;
        };
        MixedSeabed<BathyStreamCache> bottom(sourced,procedural);
        if(toggle){
            if(Active()){
                const double e=Pose().east-launch_.east;
                const double n=Pose().north-launch_.north;
                if(e*e+n*n>launch_.radius_m*launch_.radius_m||
                   Pose().speed_mps>.4)
                    return SkiffSessionEvent::Rejected;
                navigation_.Dock();
                return SkiffSessionEvent::Docked;
            }
            const double e=listener_east-launch_.east;
            const double n=listener_north-launch_.north;
            if(!std::isfinite(e)||!std::isfinite(n)||
               e*e+n*n>launch_.radius_m*launch_.radius_m)
                return SkiffSessionEvent::Rejected;
            const Water* berth=SitkaNamedWorldWater(waters,launch_.water_id,
                terrain_origin_east,terrain_origin_north,
                launch_.east,launch_.north);
            if(!berth||!navigation_.Launch(launch_.east,launch_.north,
                launch_.heading,launch_.draft_m,bottom,berth->y,true))
                return SkiffSessionEvent::Rejected;
            // Avoid reporting NO DATA for the first quarter second after
            // an already-safe launch. Real depth still requires valid water.
            navigation_.sounder.Prime(FindBoatDepth(bottom,
                launch_.east,launch_.north,berth->y,launch_.draft_m,true));
            return SkiffSessionEvent::Launched;
        }
        if(!Active())return SkiffSessionEvent::None;
        const Water* current=SitkaWorldWater(waters,terrain_origin_east,
            terrain_origin_north,Pose().east,Pose().north);
        // This is a game surface (not a realtime NOAA tide prediction).
        const double sea_level=current?current->y:
            procedural.settings().water_reference_m;
        const BoatEvent ev=navigation_.Update(dt,
            std::max(0.,throttle),rudder,sea_level,bottom,water);
        switch(ev){
        case BoatEvent::Advanced:return SkiffSessionEvent::Advanced;
        case BoatEvent::StoppedLand:return SkiffSessionEvent::BlockedLand;
        case BoatEvent::StoppedShallow:return SkiffSessionEvent::BlockedShallow;
        case BoatEvent::StoppedNoData:return SkiffSessionEvent::BlockedNoData;
        case BoatEvent::InvalidInput:return SkiffSessionEvent::InvalidInput;
        default:return SkiffSessionEvent::None;
        }
    }
private:
    SkiffLaunch launch_;
    SkiffNavigation navigation_;
    bool chord_held_=false;
};
}} // namespace
#endif
