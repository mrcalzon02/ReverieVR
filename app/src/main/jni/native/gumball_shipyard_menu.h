#ifndef REVERIE_GUMBALL_SHIPYARD_MENU_H
#define REVERIE_GUMBALL_SHIPYARD_MENU_H
#include <algorithm>
namespace reverie { namespace gumball {
// Game-private controller focus. Grid remains fully accessible via arrows,
// and the App/Back button can jump to a remembered palette selection.
struct YardCursor {
    int x=4,y=2; // connected empty starter cell, not the fixed Core
    int lastX=4,lastY=2;
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
    void pointAt(int gx,int gy){
        if(gx>=0&&gx<=6&&gy>=0&&gy<=6){
            x=gx;y=gy;lastX=gx;lastY=gy;
        }else if(gx>=7&&gx<=8&&gy>=0&&gy<=4){
            x=gx;y=gy;toolX=gx;toolY=gy;
        }
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
