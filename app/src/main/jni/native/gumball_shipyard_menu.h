#ifndef REVERIE_GUMBALL_SHIPYARD_MENU_H
#define REVERIE_GUMBALL_SHIPYARD_MENU_H
#include <algorithm>
namespace reverie { namespace gumball {
// Game-private controller focus. Grid remains fully accessible via arrows,
// and the App/Back button can jump to a remembered palette selection.
struct YardCursor {
    int x=3,y=3;
    int lastX=3,lastY=3;
    int toolX=7,toolY=0;
    bool palette()const{return x>=7;}
    int toolIndex()const{return (y*2)+(x-7);}
    void move(int dx,int dy) {
        if(dx!=0){
            if(palette()){
                if(dx<0&&x==7){x=lastX;y=lastY;}
                else x=std::max(7,std::min(8,x+dx));
            }else{
                if(dx>0&&x==6){lastX=x;lastY=y;x=7;y=toolY;}
                else {
                    x=std::max(0,std::min(6,x+dx));
                    lastX=x;lastY=y;
                }
            }
        }else if(dy!=0){
            y=std::max(0,std::min(palette()?4:6,y+dy));
            if(palette()){toolX=x;toolY=y;}
            else {lastX=x;lastY=y;}
        }
        if(palette()){toolX=x;toolY=y;}
    }
    void switchFocus(){
        if(palette()){x=lastX;y=lastY;}
        else{lastX=x;lastY=y;x=toolX;y=toolY;}
    }
    // Applying a selected palette tool should return immediately to the
    // previous edited cell, not strand the cursor on the far-right controls.
    void toolChosen(){if(palette()){toolX=x;toolY=y;x=lastX;y=lastY;}}
};
}}
#endif
