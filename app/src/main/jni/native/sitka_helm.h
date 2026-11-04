#ifndef REVERIE_SITKA_HELM_H
#define REVERIE_SITKA_HELM_H
// CPU-only boat-local helm model. Never mutate headset pose or render-eye state.
#include "sitka_boat_navigation.h"
#include "sitka_environment.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
namespace reverie { namespace sitka {
enum class HelmItem : uint8_t {
    Battery, Sonar, Gps, Lights, Roof, Radar, Overhead, MastCamera,
    RodRacks, Livewell, Outriggers, Winch, Outboard, Diesels, Foghorn, Wipers
};
struct HelmPart {
    const char* model;
    unsigned price,hull,requires_bits;
    float x,y,z; // wheel column is invariant boat-local origin
};
constexpr unsigned HelmMask(HelmItem item){return 1u<<unsigned(item);}
inline constexpr std::array<HelmPart,16> kParts = {{
    {"battery-dash",80,0,0,0.f,-.12f,.03f},
    {"depth-finder",130,0,HelmMask(HelmItem::Battery),.52f,.06f,.08f},
    {"chartplotter",160,0,HelmMask(HelmItem::Battery),-.52f,.06f,.08f},
    {"running-lights",45,0,HelmMask(HelmItem::Battery),.5f,-.22f,.1f},
    {"pilot-roof",360,1,HelmMask(HelmItem::Battery),0.f,1.75f,.4f},
    {"weather-radar",220,1,HelmMask(HelmItem::Roof),-.7f,.82f,.22f},
    {"overhead-status",160,1,HelmMask(HelmItem::Roof),0.f,1.48f,.55f},
    {"mast-camera",250,1,HelmMask(HelmItem::Roof),0.f,2.25f,-.28f},
    {"rod-racks",90,1,0,.78f,-.25f,-.95f},
    {"aerated-livewell",145,1,HelmMask(HelmItem::Battery),0.f,-.38f,-1.2f},
    {"outriggers",600,2,HelmMask(HelmItem::RodRacks),1.05f,.15f,-1.6f},
    {"hydraulic-winch",1800,3,HelmMask(HelmItem::Outriggers),0.f,.25f,-3.2f},
    {"hydraulic-outboard",650,1,0,0.f,-.35f,-2.f},
    {"twin-diesels",1800,3,HelmMask(HelmItem::Outboard),0.f,-.55f,-2.7f},
    {"foghorn",85,1,HelmMask(HelmItem::Battery),-.42f,-.25f,.1f},
    {"windshield-wipers",95,2,HelmMask(HelmItem::Battery),.45f,.85f,.58f}
}};
struct HelmProgress {
    unsigned hull=0,owned=0;
    bool Has(HelmItem item)const{return (owned&HelmMask(item))!=0u;}
    bool Purchase(HelmItem item,uint32_t& credits){
        const unsigned id=unsigned(item);
        if(id>=kParts.size()||hull>3u||Has(item))return false;
        const HelmPart& p=kParts[id];
        if(hull<p.hull||credits<p.price||
           (owned&p.requires_bits)!=p.requires_bits)return false;
        credits-=p.price;owned|=HelmMask(item);return true;
    }
    bool UpgradeHull(uint32_t& credits){
        constexpr unsigned costs[]={300u,1400u,4200u};
        if(hull>=3u||credits<costs[hull])return false;
        credits-=costs[hull++];return true;
    }
};
struct DeckBounds {
    float half_width,bow,stern,inset;
    bool Contains(double x,double z)const{
        return std::isfinite(x)&&std::isfinite(z)&&
            x>=-half_width+inset&&x<=half_width-inset&&
            z>=stern+inset&&z<=bow-inset;
    }
};
inline constexpr std::array<DeckBounds,4> kDecks={{
    {.64f,.8f,-1.05f,.12f},{1.02f,1.7f,-2.25f,.14f},
    {1.42f,2.55f,-3.8f,.16f},{2.72f,5.2f,-9.4f,.22f}
}};
struct HelmMount { const char* model;float x,y,z; };
struct HelmLayout {
    std::array<HelmMount,19> mounts{};
    unsigned count=0u;
    DeckBounds deck=kDecks[0];
    void Add(const char* key,float x,float y,float z){
        if(count<mounts.size())mounts[count++]={key,x,y,z};
    }
};
inline HelmLayout AssembleHelm(const HelmProgress& progress){
    HelmLayout out;
    const unsigned hull=std::min(progress.hull,3u);
    const char* hulls[]={"dinghy-hull","skiff-hull","pilothouse-hull","trawler-hull"};
    out.deck=kDecks[hull];
    out.Add(hulls[hull],0.f,-.75f,-.2f);
    out.Add("steering-column-and-wheel",0.f,0.f,0.f);
    out.Add("brass-compass",0.f,.32f,.08f);
    for(unsigned i=0;i<kParts.size();++i)
        if(progress.Has(HelmItem(i))){
            const HelmPart& p=kParts[i];out.Add(p.model,p.x,p.y,p.z);
        }
    return out;
}
struct DeckPassenger {
    double x=0.,z=-.4;
    bool Walk(double next_x,double next_z,const DeckBounds& deck){
        if(!deck.Contains(next_x,next_z))return false;
        x=next_x;z=next_z;return true;
    }
    void Constrain(const DeckBounds& deck){
        x=std::clamp(x,double(-deck.half_width+deck.inset),
                       double(deck.half_width-deck.inset));
        z=std::clamp(z,double(deck.stern+deck.inset),
                       double(deck.bow-deck.inset));
    }
    std::array<double,2> World(const BoatPose& boat)const{
        const double s=std::sin(boat.heading),c=std::cos(boat.heading);
        return {{boat.east+c*x+s*z,boat.north-s*x+c*z}};
    }
};
struct WheelHand {double x=0.,y=0.,z=0.;bool grip=false;};
struct WheelFeedback {unsigned ticks=0;bool center=false,detach=false;};
class HelmWheel {
public:
    double angle=0.; // 1.5 turns in either direction
    double RudderTarget()const{return std::clamp(angle/9.42477796076938,-1.,1.);}
    WheelFeedback Update(const WheelHand& left,const WheelHand& right){
        WheelFeedback out;
        const bool l=Reach(left),r=Reach(right);
        out.detach=(left.grip&&!l)||(right.grip&&!r);
        const unsigned mode=unsigned(l)+2u*unsigned(r);
        if(!mode){mode_=0u;return out;}
        const double current=mode==3u?
            std::atan2(right.y-left.y,right.x-left.x):
            std::atan2(mode==1u?left.y:right.y,mode==1u?left.x:right.x);
        if(mode!=mode_){mode_=mode;previous_=current;return out;}
        const double delta=std::atan2(std::sin(current-previous_),
                                      std::cos(current-previous_));
        previous_=current;
        if(!std::isfinite(delta)||std::abs(delta)>1.)return out;
        const double before=angle;
        angle=std::clamp(angle+delta,-9.42477796076938,9.42477796076938);
        constexpr double notch=.17453292519943295;
        out.ticks=unsigned(std::min(12.,
            std::abs(std::floor(angle/notch)-std::floor(before/notch))));
        out.center=(before<0.&&angle>=0.)||(before>0.&&angle<=0.);
        return out;
    }
private:
    unsigned mode_=0;double previous_=0.;
    static bool Reach(const WheelHand& h){
        if(!h.grip||!std::isfinite(h.x)||!std::isfinite(h.y)||
           !std::isfinite(h.z))return false;
        const double radius=std::hypot(h.x,h.y);
        return radius>=.11&&radius<=.31&&std::abs(h.z)<=.19;
    }
};
class HelmRudder {
public:
    double degrees=0.;
    double Advance(double dt,double target){
        if(!std::isfinite(dt)||dt<=0.||dt>.1||
           !std::isfinite(target)||target< -1.||target>1.)
            return degrees/35.;
        pending_+=dt;
        constexpr double tick=1./30.;
        while(pending_+1.e-10>=tick){
            pending_-=tick;
            degrees+=std::clamp(target*35.-degrees,-42.*tick,42.*tick);
        }
        return degrees/35.;
    }
    void Reset(){degrees=0.;pending_=0.;}
private:
    double pending_=0.;
};
struct HelmInstruments {
    bool sonar=false,gps=false,weather_radar=false,overhead=false;
    double rudder=0.,rpm=0.,fuel=1.,wind_angle=0.,wind_knots=0.;
    std::array<std::string,3> sounder{};
};
inline HelmInstruments HelmReadouts(const HelmProgress& progress,
        const BoatPose& boat,const Weather& weather,const DepthReading& depth,
        double rudder,double throttle,double fuel){
    HelmInstruments out;
    out.sonar=progress.Has(HelmItem::Sonar);
    out.gps=progress.Has(HelmItem::Gps);
    out.weather_radar=progress.Has(HelmItem::Radar);
    out.overhead=progress.Has(HelmItem::Overhead);
    out.rudder=rudder;
    out.rpm=boat.launched?std::max(0.,throttle)*2300.:0.;
    out.fuel=std::clamp(fuel,0.,1.);
    out.wind_angle=std::atan2(std::sin(double(weather.heading)-boat.heading),
                              std::cos(double(weather.heading)-boat.heading));
    out.wind_knots=double(weather.wind)*1.94384449244;
    if(out.sonar&&boat.launched)out.sounder=BoatSounderLabels(depth);
    return out;
}
}} // namespace reverie::sitka
#endif
