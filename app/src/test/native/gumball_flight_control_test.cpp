#include "gumball_flight_control.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::gumball;
int main(){
    const float origin[3]={0.f,0.f,0.f};
    const float center[3]={0.f,-.10f,-1.f};
    const auto middle=FlightAimFromRay(origin,center);
    assert(middle.valid&&std::fabs(middle.x)<.001f);
    assert(middle.y<0.f);
    const float right[3]={.3f,-.10f,-1.f};
    const float left[3]={-.3f,-.10f,-1.f};
    const auto r=FlightAimFromRay(origin,right);
    const auto l=FlightAimFromRay(origin,left);
    assert(l.valid&&r.valid&&l.x<0.f&&r.x>0.f);
    assert(std::fabs(l.x+r.x)<.001f);
    const auto straight=FlightAxesToTarget(0.f,-.85f,r,.025f,3.4f);
    assert(straight.x>0.f&&straight.x<=1.f);
    const auto opposite=FlightAxesToTarget(0.f,-.85f,l,.025f,3.4f);
    assert(opposite.x<0.f&&opposite.x>=-1.f);
    const auto faster=FlightAxesToTarget(0.f,-.85f,r,.025f,7.f);
    assert(faster.x>0.f&&faster.x<=1.f);
    const auto settled=FlightAxesToTarget(r.x,r.y,r,.025f,3.4f);
    assert(settled.x==0.f&&settled.y==0.f);
    // Same ray, different actual ship speed: faster ship closes the
    // distance sooner, without setting its position instantaneously.
    float slowX=0.f,fastX=0.f;
    for(int i=0;i<5;++i){
        slowX+=FlightAxesToTarget(slowX,-.85f,r,.025f,3.4f).x*.025f*3.4f;
        fastX+=FlightAxesToTarget(fastX,-.85f,r,.025f,6.8f).x*.025f*6.8f;
    }
    assert(slowX>0.f&&fastX>slowX);
    assert(fastX<=r.x+.0001f);
    const float sideways[3]={1.f,0.f,0.f};
    const float behind[3]={0.f,0.f,1.f};
    const float invalid[3]={NAN,0.f,-1.f};
    assert(!FlightAimFromRay(origin,sideways).valid);
    assert(!FlightAimFromRay(origin,behind).valid);
    assert(!FlightAimFromRay(origin,invalid).valid);
    const float farAim[3]={10.f,10.f,-1.f};
    const auto clamped=FlightAimFromRay(origin,farAim);
    assert(clamped.valid&&clamped.x==1.9f&&clamped.y==.65f);
    assert(FlightAxesToTarget(0.f,0.f,{},.02f,4.f).x==0.f);
    assert(FlightAxesToTarget(0.f,0.f,r,0.f,4.f).x==0.f);
    assert(FlightAxesToTarget(0.f,0.f,r,.02f,NAN).x==0.f);
    std::cout<<"Gumball tracked-controller steering, speed and safety passed\n";
}
