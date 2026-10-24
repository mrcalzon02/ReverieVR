#include "classic_chess_panel_layout.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using namespace reverie::chesspanel;
static bool Near(float a,float b){return std::fabs(a-b)<0.0003f;}
int main(){
    // Both wings are outside the standard 8x8 board and rotate 45 degrees.
    assert(Near(kCos45*kCos45+kSin45*kSin45,1.f));
    for(int side:{-1,1}){
        for(float u:{-.55f,0.f,.55f}){
            float x=side*kCenterX+u,z=-3.04f;
            Transform(side,x,z);
            assert((side<0&&x< -1.6f)||(side>0&&x>1.6f));
            const float y=.88f;
            const float origin[3]={0.f,0.f,0.f};
            const float direction[3]={x,y,z};
            float localX=999.f,localY=999.f;
            assert(Hit(side,origin,direction,&localX,&localY));
            assert(Near(localX,u));
            assert(Near(localY,y));
            // Outer edges of both wings tilt toward the observer.
            if(u*side>0.f)assert(z>kPanelZ);
            if(u*side<0.f)assert(z<kPanelZ);
        }
    }
    float x=0.f,y=0.f;
    const float origin[3]={0.f,0.f,0.f};
    const float parallel[3]={kCos45,0.f,-kSin45};
    assert(!Hit(-1,origin,parallel,&x,&y));
    assert(!Hit(0,origin,parallel,&x,&y));
    std::puts("Chess 45-degree wing geometry and pointer-ray roundtrip passed");
}
