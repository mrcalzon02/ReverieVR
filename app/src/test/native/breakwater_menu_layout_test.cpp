#include "breakwater_menu_layout.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <iostream>
using namespace reverie::breakwater::menu;
int main(){
    for(int i=0;i<8;++i){
        const Rect c=ShopCard(i);
        assert(Inside(c,kPanel,.10f));
        assert(HitShop(c.x,c.y)==i);
        assert(HitShop(c.x+.63f,c.y)==-1);
        for(int j=i+1;j<8;++j){
            const Rect o=ShopCard(j);
            assert(!c.Contains(o.x,o.y));
        }
    }
    for(int action=8;action<=9;++action){
        const Rect c=ShopAction(action);
        assert(Inside(c,kPanel,.10f));
        assert(HitShop(c.x,c.y)==action);
    }
    for(int i=0;i<4;++i){
        const Rect c=DifficultyCard(i);
        assert(Inside(c,kDifficultyPanel,.10f));
        assert(HitDifficulty(c.x,c.y)==i);
    }
    // The exact opening threat counts are triple the original complement;
    // campaign day/night period counts remain unchanged.
    for(int tier=0;tier<4;++tier) {
        const char *hint=DifficultyHint(tier);
        char count[20]={};
        std::snprintf(count,sizeof(count),"%d FOES",3*(2+tier));
        assert(std::strstr(hint,count)!=nullptr);
        std::snprintf(count,sizeof(count),"%d DAY WAVES",12+2*tier);
        assert(std::strstr(hint,count)!=nullptr);
        assert(4.f*FittedScale(hint,5.65f,.045f)*std::strlen(hint)<5.65f);
        const float baseline=.64f, height=.040f*2.60f;
        const Rect difficulty=DifficultyCard(tier);
        assert(baseline-height>difficulty.y+difficulty.height*.5f);
        assert(baseline+height<.98f-.052f*2.60f);
    }
    for(int i=0;i<3;++i) {
        const Rect slot=SlotCard(i);
        assert(Inside(slot,kDifficultyPanel,.10f));
        assert(HitSlot(slot.x,slot.y)==i);
    }
    for(int i=0;i<2;++i) {
        const Rect action=DebriefAction(i);
        assert(Inside(action,kPanel,.10f));
        assert(HitDebrief(action.x,action.y)==i);
    }
    assert(Inside(kResetCard,kPanel,.03f));
    assert(HitShop(kResetCard.x,kResetCard.y)==10);
    assert(std::strstr(DifficultyHint(-1),"POINT AT")!=nullptr);
    assert(HitShop(0.f,-1.18f)==-1); // footer is a label, not a button
    assert(HitShop(NAN,0.f)==-1);
    assert(HitDifficulty(0.f,NAN)==-1);
    const char *labels[]={"FIRE","POWER","CLIP","AA 3/3","ARTY","AIR",
                          "TURRET","REPAIR","MISSILE","CANNON","UPGRADES"};
    for(const char *label:labels){
        const float width=1.23f;
        const float scale=FittedScale(label,width,.052f);
        assert(scale>0.f && scale<=.052f);
        assert(std::strlen(label)*4.f*scale<=width-.119f);
    }
    assert(FittedScale("UPGRADES",ShopAction(9).width,.052f)>0.f);
    assert(!Inside({0.f,-1.70f,1.0f,.10f},kPanel)); // old clipped footer
    // New textual footer extends at most 3 glyph rows above/below center.
    assert(kShopCreditsY-kShopCreditsScale*2.60f > kPanel.y-kPanel.height*.5f);
    // Conservative glyph box covers old strokes and new proportional Lato.
    const float maxHintExtent=2.60f*kShopHintScale;
    const float maxStatusExtent=2.60f*.040f;
    assert(kShopHintY-maxHintExtent >
           kShopRowY[0]+kCardHeight*.5f);
    assert(kShopHintY+maxHintExtent <
           kShopStatusY-maxStatusExtent);
    assert(kShopCreditsY-2.60f*kShopCreditsScale >
           kPanel.y-kPanel.height*.5f);
    // Raised optical-footprint bound, not merely the physical sea plane.
    assert(kPanel.y-kPanel.height*.5f >= -.7101f);
    assert(kPanel.y-kPanel.height*.5f >= -1.1001f);
    assert(kShopRowY[1]-kCardHeight*.5f > kActionY+kActionHeight*.5f);
    assert(kActionY-kActionHeight*.5f > kShopCreditsY+2.60f*kShopCreditsScale);
    assert(kShopCreditsY-kShopCreditsScale*2.60f > -1.10f);
    std::cout<<"Breakwater compact menu bounds, text fitting and target parity passed\n";
}
