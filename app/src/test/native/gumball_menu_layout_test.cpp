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
