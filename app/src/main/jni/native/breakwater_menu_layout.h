#ifndef REVERIE_BREAKWATER_MENU_LAYOUT_H
#define REVERIE_BREAKWATER_MENU_LAYOUT_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace reverie { namespace breakwater { namespace menu {
struct Rect {
    float x, y, width, height;
    bool Contains(float px,float py) const {
        return std::isfinite(px) && std::isfinite(py) &&
               std::abs(px-x) <= width*0.5f &&
               std::abs(py-y) <= height*0.5f;
    }
};
// Single authoritative geometry for BOTH ray hit-tests and drawn widgets.
constexpr Rect kPanel={0.f,0.28f,6.10f,2.76f};
constexpr Rect kDifficultyPanel={0.f,0.27f,6.10f,2.75f};
constexpr float kColumnX[4]={-2.10f,-0.70f,0.70f,2.10f};
constexpr float kShopRowY[2]={0.27f,-0.22f};
constexpr float kCardWidth=1.23f,kCardHeight=0.40f;
constexpr float kActionY=-0.66f;
constexpr float kActionWidth=1.60f,kActionHeight=0.30f;
// Text baselines are part of the same shop layout contract: a tooltip that
// fits horizontally must not overlap the top cards or status heading.
constexpr float kShopStatusY=0.86f;
constexpr float kShopHintY=0.61f;
constexpr float kShopHintScale=0.043f;
constexpr float kShopCreditsY=-0.95f;
constexpr float kShopCreditsScale=0.039f;
inline Rect ShopCard(int index) {
    return index>=0 && index<8 ?
        Rect{kColumnX[index%4],kShopRowY[index/4],kCardWidth,kCardHeight} :
        Rect{0.f,0.f,0.f,0.f};
}
inline Rect DifficultyCard(int index) {
    return index>=0 && index<4 ?
        Rect{kColumnX[index],-0.24f,kCardWidth,0.43f} :
        Rect{0.f,0.f,0.f,0.f};
}
inline Rect ShopAction(int action) {
    return action==8 ? Rect{0.90f,kActionY,kActionWidth,kActionHeight} :
           action==9 ? Rect{-0.90f,kActionY,kActionWidth,kActionHeight} :
           Rect{0.f,0.f,0.f,0.f};
}
inline int HitShop(float x,float y) {
    for(int i=0;i<8;++i) if(ShopCard(i).Contains(x,y))return i;
    if(ShopAction(8).Contains(x,y))return 8;
    if(ShopAction(9).Contains(x,y))return 9;
    return -1;
}
inline int HitDifficulty(float x,float y) {
    for(int i=0;i<4;++i)if(DifficultyCard(i).Contains(x,y))return i;
    return -1;
}
// classic_vector_art uses exactly 4*scale of advance per glyph.
// Keep 0.12 court units of horizontal inset on each card.
inline float FittedScale(const char *label,float width,float desired) {
    if(!label || width<=0.f || desired<=0.f)return 0.f;
    const size_t count=std::strlen(label);
    if(!count)return 0.f;
    const float available=std::max(0.f,width-0.12f);
    return std::min(desired,available/(4.f*static_cast<float>(count)));
}
inline bool Inside(const Rect &child,const Rect &parent,float margin=0.f) {
    return child.x-child.width*0.5f>=parent.x-parent.width*0.5f+margin &&
           child.x+child.width*0.5f<=parent.x+parent.width*0.5f-margin &&
           child.y-child.height*0.5f>=parent.y-parent.height*0.5f+margin &&
           child.y+child.height*0.5f<=parent.y+parent.height*0.5f-margin;
}
}}}
#endif
