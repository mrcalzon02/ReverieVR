#include "breakwater_unit_geometry.h"
#include <cassert>
#include <cmath>
#include <array>
#include <iostream>
using namespace reverie::breakwater::unit_geometry;
int main(){
    int triangles=0, boxes=0;
    float minx=100,maxx=-100,minz=100,maxz=-100,miny=100,maxy=-100;
    auto face=[&](float ax,float ay,float az,float bx,float by,float bz,
                  float cx,float cy,float cz,float red,float green,float blue){
        for(float x:{ax,ay,az,bx,by,bz,cx,cy,cz,red,green,blue})assert(std::isfinite(x));
        for(float v:{red,green,blue})assert(v>=0&&v<=1.5f);
        minx=std::min(minx,std::min(ax,std::min(bx,cx)));
        maxx=std::max(maxx,std::max(ax,std::max(bx,cx)));
        minz=std::min(minz,std::min(az,std::min(bz,cz)));
        maxz=std::max(maxz,std::max(az,std::max(bz,cz)));
        miny=std::min(miny,std::min(ay,std::min(by,cy)));
        maxy=std::max(maxy,std::max(ay,std::max(by,cy)));
        triangles++;
    };
    auto box=[&](float x,float y,float z,float w,float h,float d,float r,float g,float b){
        for(float v:{x,y,z,w,h,d,r,g,b})assert(std::isfinite(v));
        assert(w>0&&h>0&&d>0);boxes++;
    };
    for(auto kind:{HullKind::Rowboat,HullKind::LandingCraft,HullKind::Gunboat,HullKind::HeavyTransport}){
        int t=ContouredHull(0,-8,kind,face);
        assert(t==50); // 4 sections × 2 sides × 6 triangles, plus 2 transoms
    }
    assert(triangles==200);
    assert(minz<-9.15f&&maxz>-7.f);
    assert(miny<=-1.f&&maxy>=-.7f);
    assert(minx<-.8f&&maxx>.8f);
    assert(ContouredHull(NAN,0,HullKind::Rowboat,face)==0);
    assert(ContouredHull(0,INFINITY,HullKind::Gunboat,face)==0);
    RowboatFurniture(0,-5,1,box,face);
    LandingDetails(0,-5,0,6,false,box,face);
    LandingDetails(0,-2.2f,1,4,true,box,face);
    ShoreArmor(0,-1.5f,false,box,face);
    int apcBoxes=boxes;
    ShoreArmor(0,-1.5f,true,box,face);
    assert(boxes>apcBoxes);
    SoldierEquipment(0,-1.3f,false,box,face);
    SoldierEquipment(.5f,-1.4f,true,box,face);
    assert(triangles>220&&boxes>40);
    std::cout<<"PASS: 4 closed-chine hulls, articulated ramps, rowboat, APC, tank, and soldier visual recipes\n";
}
