#ifndef REVERIE_CLASSIC_CHESS_PANEL_LAYOUT_H
#define REVERIE_CLASSIC_CHESS_PANEL_LAYOUT_H
// Symmetric 45-degree 3D wings. Same transform governs geometry and hit testing.
#include <cmath>
namespace reverie { namespace chesspanel {
constexpr float kCenterX=2.22f;
constexpr float kCos45=0.7071067811865476f;
constexpr float kSin45=0.7071067811865476f;
constexpr float kTowardPlayer=.28f;
constexpr float kPanelZ=-3.04f+kTowardPlayer;
inline void Transform(int side,float &x,float &z) {
    const float dx=x-static_cast<float>(side)*kCenterX;
    x=static_cast<float>(side)*kCenterX+kCos45*dx;
    z+=kTowardPlayer+static_cast<float>(side)*kSin45*dx;
}
inline bool Hit(int side,const float origin[3],const float direction[3],
                float *localX,float *localY) {
    if(!origin||!direction||!localX||!localY||
       (side!=-1&&side!=1))return false;
    const float nx=-static_cast<float>(side)*kSin45,nz=kCos45;
    const float denom=nx*direction[0]+nz*direction[2];
    if(std::fabs(denom)<.00001f)return false;
    const float cx=static_cast<float>(side)*kCenterX;
    const float t=(nx*(cx-origin[0])+nz*(kPanelZ-origin[2]))/denom;
    if(!(t>0.f&&t<30.f))return false;
    const float x=origin[0]+t*direction[0];
    const float z=origin[2]+t*direction[2];
    *localX=(x-cx)*kCos45+(z-kPanelZ)*static_cast<float>(side)*kSin45;
    *localY=origin[1]+t*direction[1];
    return std::isfinite(*localX)&&std::isfinite(*localY);
}
}}
#endif
