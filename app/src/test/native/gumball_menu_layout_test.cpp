#include "gumball_menu_layout.h"
#include <cassert>
#include <iostream>
using namespace reverie::gumball;
int main(){
    const int left=kChartLoadout,middle=kChartShipyard,right=kChartAllies;
    assert(ChartNavigate(0,0,1)==left);
    assert(ChartNavigate(1,0,1)==middle);
    assert(ChartNavigate(2,0,1)==right);
    assert(ChartNavigate(left,0,-1)==0);
    assert(ChartNavigate(middle,0,-1)==1);
    assert(ChartNavigate(right,0,-1)==2);
    assert(ChartNavigate(0,1,0)==1);
    assert(ChartNavigate(2,1,0)==0);
    assert(ChartNavigate(left,1,0)==middle);
    assert(ChartNavigate(middle,1,0)==right);
    assert(ChartNavigate(right,1,0)==left);
    assert(ChartNavigate(left,-1,0)==right);
    assert(ChartNavigate(1,0,-1)==1);
    for(const int id:{0,1,2,10,11,12}){
        const int next=ChartNavigate(id,1,0);
        assert(ChartNavigate(next,-1,0)==id);
        const int vertical=ChartNavigate(id,0,ChartBottom(id)?-1:1);
        assert(ChartNavigate(vertical,0,ChartBottom(id)?1:-1)==id);
    }
    // Strategic map: hover/highlight never launches, first click arms only.
    RouteLaunchGate route;
    assert(route.armed==-1 && !route.ready(0));
    assert(route.press(0)==RoutePress::Armed);
    assert(route.armed==0 && !route.ready(0));
    assert(route.press(0)==RoutePress::Waiting); // accidental double-click
    for(int tick=0;tick<6;++tick)route.advance(.05f);
    assert(!route.ready(0));
    route.advance(.05f);
    assert(route.ready(0)); // selection lock ready after 350ms
    assert(route.armed==0); // no automatic game launch when delay elapses
    assert(route.press(0)==RoutePress::Launch);
    assert(route.armed==-1); // must explicitly rearm on return
    assert(route.press(1)==RoutePress::Armed);
    route.advance(.05f);
    route.focus(2); // changing lanes cancels the original confirmation
    assert(route.armed==-1&&!route.ready(1));
    assert(route.press(2)==RoutePress::Armed);
    for(int tick=0;tick<7;++tick)route.advance(.05f);
    assert(route.ready(2));
    route.focus(kChartLoadout); // utility focus also cancels armed route
    assert(route.armed==-1);
    assert(route.press(kChartShipyard)==RoutePress::Ignored);
    assert(route.armed==-1);
    route.press(1);
    route.reset(); // exiting to shipyard/loadout or ending a sortie
    assert(route.armed==-1&&!route.ready(1));
    assert(route.press(-1)==RoutePress::Ignored);
    // Invalid, skipped and oversized time increments cannot bypass delay.
    assert(route.press(0)==RoutePress::Armed);
    route.advance(-1.f);route.advance(5.f);route.advance(0.f);
    assert(!route.ready(0));
    for(int tick=0;tick<7;++tick)route.advance(.05f);
    assert(route.ready(0));
    assert(route.press(0)==RoutePress::Launch);
    // Navigation keeps all routes reachable without a mouse or headset ray.
    for(int n=0;n<3;++n){
        RouteLaunchGate item;
        assert(item.press(n)==RoutePress::Armed);
        assert(item.press(n)==RoutePress::Waiting);
        for(int k=0;k<7;++k)item.advance(.05f);
        assert(item.press(n)==RoutePress::Launch);
    }
    // The App/Minus edge can exit Loadout or Allies but never hijacks
    // the Shipyard's separate grid/palette switch.
    assert(PrepSecondaryBack(true,false,false,true,false));
    assert(PrepSecondaryBack(false,true,false,true,false));
    assert(!PrepSecondaryBack(false,false,true,true,false));
    assert(!PrepSecondaryBack(true,false,true,true,false));
    assert(!PrepSecondaryBack(true,false,false,true,true));
    assert(!PrepSecondaryBack(true,false,false,false,false));
    assert(!PrepSecondaryBack(false,false,false,true,false));
    // Four upgrades, three weapons and a Return action must all be reachable.
    const int selectUpgrades[]={0,1,2,3};
    for(const int x:selectUpgrades){
        const int weapon=LoadoutNavigate(x,0,1);
        assert(weapon>=4&&weapon<=6);
        assert(LoadoutNavigate(weapon,0,-1)==(x>2?2:x));
    }
    assert(LoadoutNavigate(0,1,0)==1);
    assert(LoadoutNavigate(3,1,0)==0);
    assert(LoadoutNavigate(4,-1,0)==6);
    assert(LoadoutNavigate(6,1,0)==4);
    assert(LoadoutNavigate(5,0,1)==7);
    assert(LoadoutNavigate(7,0,-1)==5);
    assert(LoadoutNavigate(7,1,0)==7);
    // Card and button widths fit within the game-private 5-unit canvas.
    for(int i=0;i<3;++i){
        assert(kChartRouteX[i]>-2.5f&&kChartRouteX[i]<2.5f);
        assert(kChartActionX[i]>-2.5f&&kChartActionX[i]<2.5f);
        if(i>0)assert(kChartRouteX[i]-kChartRouteX[i-1]>1.44f);
    }
    for(int i=1;i<4;++i)
        assert(kLoadoutUpgradeX[i]-kLoadoutUpgradeX[i-1]>1.12f);
    for(int i=1;i<3;++i)
        assert(kLoadoutWeaponX[i]-kLoadoutWeaponX[i-1]>1.43f);
    std::cout<<"gumball chart and loadout navigation: passed\n";
}
