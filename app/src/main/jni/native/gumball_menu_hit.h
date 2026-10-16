#ifndef REVERIE_GUMBALL_MENU_HIT_H
#define REVERIE_GUMBALL_MENU_HIT_H
#include <cmath>
#include "gumball_menu_layout.h"
namespace reverie { namespace gumball {
struct MenuHit {int id=-1,x=-1,y=-1;};
inline bool MenuInside(float x,float y,float cx,float cy,float w,float h){
    return std::isfinite(x)&&std::isfinite(y)&&
        std::fabs(x-cx)<=w*.5f&&std::fabs(y-cy)<=h*.5f;
}
// The pad owns focus until the ray actually moves far enough from its
// last pointer-owned position. Do not advance the reference on ignored
// sub-threshold frames: incremental movement must eventually accumulate.
struct MenuPointerFocus {
    bool seen=false,padFocus=false;
    float referenceX=0.f,referenceY=0.f;
    bool take(float x,float y,bool padMoving){
        if(!std::isfinite(x)||!std::isfinite(y))return false;
        if(!seen){
            seen=true;referenceX=x;referenceY=y;
            return !padMoving&&!padFocus;
        }
        if(padMoving)return false;
        if(padFocus){
            if(std::fabs(x-referenceX)<=.075f&&
               std::fabs(y-referenceY)<=.075f)return false;
            padFocus=false;
        }
        referenceX=x;referenceY=y;
        return true;
    }
};
inline MenuHit ChartHit(float x,float y){
    for(int i=0;i<3;++i){
        if(MenuInside(x,y,kChartRouteX[i],.22f,1.44f,1.09f))
            return {i,-1,-1};
        if(MenuInside(x,y,kChartActionX[i],-.94f,1.43f,.55f))
            return {i==0?12:(i==1?10:11),-1,-1};
    }
    return {};
}
inline MenuHit LoadoutHit(float x,float y){
    for(int i=0;i<4;++i)
        if(MenuInside(x,y,kLoadoutUpgradeX[i],.53f,1.12f,.65f))
            return {i,-1,-1};
    for(int i=0;i<3;++i)
        if(MenuInside(x,y,kLoadoutWeaponX[i],-.38f,1.43f,.67f))
            return {i+4,-1,-1};
    if(MenuInside(x,y,0.f,-1.10f,2.30f,.44f))return {7,-1,-1};
    return {};
}
inline MenuHit SupportHit(float x,float y){
    for(int i=0;i<9;++i)
        if(MenuInside(x,y,0.f,.88f-.28f*static_cast<float>(i),4.35f,.26f))
            return {i,-1,-1};
    return {};
}
inline MenuHit YardHit(float x,float y){
    for(int gy=0;gy<7;++gy)for(int gx=0;gx<7;++gx)
        if(MenuInside(x,y,-1.47f+(gx-3)*.28f,.075f+(3-gy)*.28f,
                      .270f,.270f))
            return {0,gx,gy};
    for(int i=0;i<10;++i)
        if(MenuInside(x,y,.23f+(i%2)*1.34f,
                      .79f-(i/2)*.345f,1.20f,.32f))
            return {1,7+(i%2),i/2};
    return {};
}
}}
#endif
