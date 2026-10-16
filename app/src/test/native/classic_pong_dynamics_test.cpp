#include "classic_pong_dynamics.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::classics::pong;
float Speed(Rebound b){return std::hypot(b.vx,b.vy);}
int main(){
    static_assert(kServeHorizontalSpeed>0.80f,"Serve speed regression");
    assert(PaddleVelocity(1.f,0.01f)==kMaxCapturedPaddleSpeed);
    assert(PaddleVelocity(-1.f,0.01f)==-kMaxCapturedPaddleSpeed);
    assert(PaddleVelocity(1.f,0.f)==0.f);
    const auto stationary=Bounce(-0.92f,0.10f,0.17f,0.f,true);
    const auto assist=Bounce(-0.92f,0.10f,0.17f,1.9f,true);
    const auto against=Bounce(-0.92f,0.10f,0.17f,-1.9f,true);
    assert(assist.vy>stationary.vy && stationary.vy>against.vy);
    assert(Speed(assist)>Speed(stationary)&&Speed(stationary)>Speed(against));
    assert(assist.vx>0.f && Speed(assist)<=kMaxBallSpeed);
    assert(Bounce(0.90f,-0.20f,-0.1f,-1.3f,false).vx<0.f);
    const auto flat=Bounce(-0.9f,0.f,0.f,0.f,true);
    assert(std::abs(flat.vy)>=0.07f); // no perfectly straight volley
    Rebound r={-kServeHorizontalSpeed,kServeVerticalSpeed};
    for(int i=0;i<100;++i){
        const bool left=(i%2)==0;
        r=Bounce(r.vx,r.vy,0.f,0.f,left);
        assert(std::isfinite(r.vx)&&std::isfinite(r.vy));
        assert(Speed(r)<=kMaxBallSpeed+0.0001f);
        assert(Speed(r)>=kMinBallSpeed-0.0001f);
    }
    assert(Speed(r)>1.60f); // sustained play can actually become fast
    assert(CpuTarget(.7f,.8f,.7f,true,1,0)<=.78f);
    assert(CpuTarget(.7f,.8f,-.7f,true,1,0)==0.f);
    assert(CpuTarget(.7f,.8f,.7f,false,1,0)==0.f);
    std::cout<<"Pong swing transfer, inverse braking, rebound bounds, and CPU target passed\n";
}
