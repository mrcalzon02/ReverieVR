#include "gumball_menu_hit.h"
#include "gumball_shipyard_menu.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::gumball;
int main(){
    for(int i=0;i<3;++i){
        assert(ChartHit(kChartRouteX[i],.22f).id==i);
        assert(ChartHit(kChartActionX[i],-.94f).id==
               (i==0?12:(i==1?10:11)));
    }
    for(int i=0;i<4;++i)
        assert(LoadoutHit(kLoadoutUpgradeX[i],.53f).id==i);
    for(int i=0;i<3;++i)
        assert(LoadoutHit(kLoadoutWeaponX[i],-.38f).id==i+4);
    assert(LoadoutHit(0.f,-1.10f).id==7);
    for(int i=0;i<9;++i)
        assert(SupportHit(0.f,.88f-.28f*static_cast<float>(i)).id==i);
    assert(ChartHit(0.f,-1.55f).id==-1);
    assert(LoadoutHit(2.4f,1.4f).id==-1);
    assert(YardHit(NAN,0.f).id==-1);

    // A controller pointing at blank menu space must NEVER activate the
    // stale selected purchase/mission/ship cell. Pad-only mode is retained.
    const MenuHit missed{};
    assert(!MenuClickAllowed(true,false,false,missed));
    assert(MenuClickAllowed(true,false,false,ChartHit(kChartRouteX[1],.22f)));
    assert(MenuClickAllowed(true,false,false,LoadoutHit(kLoadoutWeaponX[2],-.38f)));
    assert(MenuClickAllowed(true,false,false,SupportHit(0.f,.88f)));
    assert(MenuClickAllowed(true,false,false,YardHit(-1.47f,.075f)));
    assert(MenuClickAllowed(true,true,false,missed));  // pad owns focus
    assert(MenuClickAllowed(true,false,true,missed));  // pad currently moving
    assert(MenuClickAllowed(false,false,false,missed)); // no controller ray
    // Pointer targets line up with EVERY physical ship cell.
    for(int y=0;y<7;++y)for(int x=0;x<7;++x){
        const MenuHit hit=YardHit(-1.47f+(x-3)*.28f,
                                  .075f+(3-y)*.28f);
        assert(hit.id==0&&hit.x==x&&hit.y==y);
    }
    for(int i=0;i<14;++i){
        const MenuHit hit=YardHit(.23f+(i%2)*1.34f,
                                  .79f-(i/2)*.270f);
        assert(hit.id==1&&hit.x==7+(i%2)&&hit.y==i/2);
    }
    assert(YardHit(-2.49f,-1.60f).id==-1);
    for(int i=10;i<14;++i){
        YardCursor sliceControl;
        sliceControl.pointAt(7+i%2,i/2);
        assert(sliceControl.palette()&&sliceControl.toolIndex()==i);
        assert(sliceControl.chosenPart()==Part::Empty);
    }
    YardCursor cursor;
    assert(cursor.x==4&&cursor.y==2&&!cursor.palette());
    cursor.switchFocus();
    assert(cursor.palette()&&cursor.toolIndex()==0);
    cursor.move(1,0);
    cursor.move(0,1);
    assert(cursor.toolIndex()==3);
    cursor.toolChosen();
    assert(!cursor.palette()&&cursor.x==4&&cursor.y==2);
    cursor.pointAt(6,6);
    cursor.move(1,0);
    assert(cursor.palette());
    assert(cursor.toolIndex()==2); // x=7, remembered palette row one
    cursor.toolChosen();
    assert(cursor.x==6&&cursor.y==6);
    cursor.switchFocus();
    cursor.pointAt(8,4);
    assert(cursor.palette()&&cursor.toolIndex()==9);
    cursor.switchFocus();
    assert(cursor.x==6&&cursor.y==6);
    // All grid cells and palette buttons stay reachable through pointer
    // without requiring held primary or simulated touchpad direction.
    cursor.pointAt(8,0);
    assert(cursor.chosenPart()==Part::Armor);
    cursor.pointAt(7,4);
    assert(cursor.chosenPart()==Part::Cargo);
    cursor.pointAt(8,4); // Return is not an equipment brush.
    assert(cursor.chosenPart()==Part::Empty);
    cursor.pointAt(0,0);
    assert(cursor.chosenPart()==Part::Empty);
    assert(cursor.x==0&&cursor.y==0);
    cursor.pointAt(-1,20);
    assert(cursor.x==0&&cursor.y==0);
    // Route->Loadout/Allies and other screen transitions must not let a
    // stationary ray instantly hover a button occupying the old location.
    MenuPointerFocus transition;
    assert(transition.take(0.f,-.94f,false));
    transition.screenChanged();
    assert(transition.padFocus&&!transition.seen);
    assert(!transition.take(0.f,-.94f,false));
    assert(!transition.take(0.f,-.90f,false));
    assert(transition.take(0.f,-.80f,false));
    assert(!transition.padFocus);
    // A slowly moving pointer must eventually retake focus from the pad.
    // Previously each sub-threshold frame reset the movement origin,
    // making a continuously moving ray permanently lose pointer authority.
    MenuPointerFocus focus;
    assert(focus.take(0.f,0.f,false));
    focus.padFocus=true;
    for(int i=1;i<=7;++i)
        assert(!focus.take(static_cast<float>(i)*.01f,0.f,false));
    assert(focus.padFocus);
    assert(focus.take(.09f,0.f,false)&&!focus.padFocus);
    focus.padFocus=true;
    assert(!focus.take(.3f,0.f,true)); // moving pad keeps its authority
    assert(!focus.take(.14f,0.f,false)); // below movement threshold
    assert(focus.take(.18f,0.f,false)); // accumulated, ray regains focus
    focus.padFocus=true;
    assert(!focus.take(NAN,0.f,false)); // invalid ray cannot activate
    assert(focus.padFocus);
    std::cout<<"Gumball direct-click menus and shipyard focus passed\n";
}
