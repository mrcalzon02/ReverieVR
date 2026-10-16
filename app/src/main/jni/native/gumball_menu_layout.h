#ifndef REVERIE_GUMBALL_MENU_LAYOUT_H
#define REVERIE_GUMBALL_MENU_LAYOUT_H
namespace reverie { namespace gumball {
// Pure game-private menu navigation; selectable IDs retain existing actions.
// Chart: three lanes (0-2), bottom [Loadout(12), Shipyard(10), Allies(11)].
// Loadout: four upgrade slots (0-3), three weapon slots (4-6), Return(7).
constexpr int kChartLoadout=12;
constexpr int kChartShipyard=10;
constexpr int kChartAllies=11;
inline int ChartColumn(int selection){
    switch(selection){
    case 0: case 12:return 0;
    case 1: case 10:return 1;
    case 2: case 11:return 2;
    default:return 0;
    }
}
inline bool ChartBottom(int selection){
    return selection==kChartLoadout||selection==kChartShipyard
        ||selection==kChartAllies;
}
inline int ChartFromColumn(int column,bool bottom){
    const int i=(column+3)%3;
    return bottom?(i==0?kChartLoadout:(i==1?kChartShipyard:kChartAllies)):i;
}
inline int ChartNavigate(int selection,int horizontal,int vertical){
    if(horizontal!=0)return ChartFromColumn(
        (ChartColumn(selection)+(horizontal>0?1:2))%3,ChartBottom(selection));
    if(vertical!=0)return ChartFromColumn(
        ChartColumn(selection),vertical>0);
    return selection;
}
inline int LoadoutNavigate(int selection,int horizontal,int vertical){
    if(selection<0||selection>7)selection=0;
    if(horizontal!=0){
        if(selection<4)return (selection+(horizontal>0?1:3))%4;
        if(selection<7)return 4+(selection-4+(horizontal>0?1:2))%3;
        return selection;
    }
    if(vertical>0){
        if(selection<4)return 4+(selection>2?2:selection);
        return 7;
    }
    if(vertical<0){
        if(selection==7)return 5;
        if(selection>=4)return selection-4;
    }
    return selection;
}
// Layout centers and sizes, all in native flat-mesh world units.
constexpr float kChartRouteX[3]={-1.55f,0.f,1.55f};
constexpr float kChartActionX[3]={-1.55f,0.f,1.55f};
constexpr float kLoadoutUpgradeX[4]={-1.82f,-.61f,.60f,1.81f};
constexpr float kLoadoutWeaponX[3]={-1.55f,0.f,1.55f};
}}
#endif
