#include "breakwater_menu_layout.h"
#include <cassert>
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
    assert(-1.18f-.046f*2.5f > kPanel.y-kPanel.height*.5f);
    std::cout<<"Breakwater compact menu bounds, text fitting and target parity passed\n";
}
