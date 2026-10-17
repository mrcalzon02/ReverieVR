#ifndef REVERIE_GUMBALL_FLIGHT_CONTROL_H
#define REVERIE_GUMBALL_FLIGHT_CONTROL_H
#include <algorithm>
#include <cmath>
namespace reverie { namespace gumball {
// Same world-space gameplay plane as the Candy arcade renderer. Uses the
// existing controller orientation ray: no new Android input ABI.
struct FlightAim {float x=0.f,y=0.f;bool valid=false;};
struct FlightAxes {float x=0.f,y=0.f;};
constexpr float kFlightPointerSpeedFactor=2.2f;
inline FlightAim FlightAimFromRay(const float origin[3],const float direction[3]){
    if(!origin||!direction)return {};
    for(int i=0;i<3;++i)
        if(!std::isfinite(origin[i])||!std::isfinite(direction[i]))
            return {};
    if(std::fabs(direction[2])<.001f)return {};
    const float distance=(-2.94f-origin[2])/direction[2];
    if(!std::isfinite(distance)||distance<=0.f||distance>=30.f)return {};
    const float x=origin[0]+distance*direction[0];
    const float y=origin[1]+distance*direction[1];
    if(!std::isfinite(x)||!std::isfinite(y))return {};
    return {std::max(-1.9f,std::min(1.9f,x)),
            std::max(-1.15f,std::min(.65f,y)),true};
}
// Follow the ray without teleporting: thrusters and installed engines cap
// the ship's per-frame travel. The game convention has positive Y axis
// meaning "move down", hence the inverted target delta.
inline FlightAxes FlightAxesToTarget(float x,float y,const FlightAim& target,
                                    float dt,float speed){
    if(!target.valid||!std::isfinite(x)||!std::isfinite(y)
       ||!std::isfinite(dt)||!std::isfinite(speed)
       ||dt<=0.f||speed<=0.f)return {};
    const float maxStep=std::min(dt,.05f)*speed;
    if(maxStep<=0.f)return {};
    const float dx=target.x-x,dy=y-target.y;
    return {std::max(-1.f,std::min(1.f,dx/maxStep)),
            std::max(-1.f,std::min(1.f,dy/maxStep))};
}
}}
#endif
