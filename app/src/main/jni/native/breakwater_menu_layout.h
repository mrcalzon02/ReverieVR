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
// Compact vertical footprint (-0.71..1.55 m at z=-3) preserves optical
// breathing room below the last label. This is a source geometry bound,
// NOT proof of a specific headset's per-eye visible field.
constexpr Rect kPanel={0.f,0.42f,6.10f,2.26f};
constexpr Rect kDifficultyPanel={0.f,0.42f,6.10f,2.26f};
constexpr float kColumnX[4]={-2.10f,-0.70f,0.70f,2.10f};
constexpr float kShopRowY[2]={0.34f,-0.04f};
constexpr float kCardWidth=1.23f,kCardHeight=0.36f;
constexpr float kActionY=-0.36f;
constexpr float kActionWidth=1.60f,kActionHeight=0.24f;
// Text baselines are part of the same shop layout contract: a tooltip that
// fits horizontally must not overlap the top cards or status heading.
constexpr float kShopStatusY=0.84f;
constexpr float kShopHintY=0.63f;
constexpr float kShopHintScale=0.032f;
constexpr float kShopCreditsY=-0.60f;
constexpr float kShopCreditsScale=0.033f;
inline Rect ShopCard(int index) {
    return index>=0 && index<8 ?
        Rect{kColumnX[index%4],kShopRowY[index/4],kCardWidth,kCardHeight} :
        Rect{0.f,0.f,0.f,0.f};
}
inline Rect DifficultyCard(int index) {
    return index>=0 && index<4 ?
        Rect{kColumnX[index],0.28f,kCardWidth,0.38f} :
        Rect{0.f,0.f,0.f,0.f};
}
inline Rect ShopAction(int action) {
    return action==8 ? Rect{0.90f,kActionY,kActionWidth,kActionHeight} :
           action==9 ? Rect{-0.90f,kActionY,kActionWidth,kActionHeight} :
           Rect{0.f,0.f,0.f,0.f};
}
constexpr Rect kResetCard={2.30f,-.56f,1.18f,.22f};
inline Rect DebriefAction(int action) {
    return action>=0 && action<2?
        Rect{action==0?-1.1f:1.1f,-.43f,1.86f,.33f}:
        Rect{0.f,0.f,0.f,0.f};
}
inline int HitDebrief(float x,float y) {
    for(int i=0;i<2;i++)if(DebriefAction(i).Contains(x,y))return i;
    return -1;
}
inline Rect SlotCard(int index) {
    return index>=0 && index<3?
        Rect{-1.79f+index*1.79f,.20f,1.52f,.43f}:
        Rect{0.f,0.f,0.f,0.f};
}
inline int HitSlot(float x,float y) {
    for(int i=0;i<3;i++)if(SlotCard(i).Contains(x,y))return i;
    return -1;
}
inline int HitShop(float x,float y) {
    for(int i=0;i<8;++i) if(ShopCard(i).Contains(x,y))return i;
    if(ShopAction(8).Contains(x,y))return 8;
    if(ShopAction(9).Contains(x,y))return 9;
    if(kResetCard.Contains(x,y))return 10;
    return -1;
}
inline const char* DifficultyHint(int choice) {
    // First-wave incoming craft have been tripled; daytime wave count
    // remains the existing long-form campaign schedule.
    switch(choice) {
        case 0: return "CADET - 6 FOES 12 DAY WAVES";
        case 1: return "REGULAR - 9 FOES 14 DAY WAVES";
        case 2: return "VETERAN - 12 FOES 16 DAY WAVES";
        case 3: return "SIEGE - 15 FOES 18 DAY WAVES";
        default: return "POINT AT A DIFFICULTY TO INSPECT";
    }
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
