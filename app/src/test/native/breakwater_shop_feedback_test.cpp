#include "breakwater_shop_feedback.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::breakwater::shopfeedback;
int main() {
    char msg[70]={};
    Describe(msg,sizeof(msg),false,0,180,500,false,false);
    assert(std::strcmp(msg,"FASTER FIRE - BUY 180 CR")==0);
    Describe(msg,sizeof(msg),false,3,750,500,false,false);
    assert(std::strcmp(msg,"AIR DEFENSE - NEED 750 CR")==0);
    Describe(msg,sizeof(msg),false,3,-1,500,false,false);
    assert(std::strcmp(msg,"AIR DEFENSE - MAXED")==0);
    Describe(msg,sizeof(msg),true,4,1600,1800,true,false);
    assert(std::strcmp(msg,"NAVAL CANNON - EQUIP")==0);
    Describe(msg,sizeof(msg),true,4,1600,1800,true,true);
    assert(std::strcmp(msg,"NAVAL CANNON - ACTIVE")==0);
    Describe(msg,sizeof(msg),true,5,540,540,false,false);
    assert(std::strcmp(msg,"SAM MISSILE - BUY 540 CR")==0);
    for(int i=0;i<8;++i) {
        assert(Item(false,i)!=nullptr);
        assert((Item(true,i)!=nullptr)==(i<6));
    }
    Describe(msg,sizeof(msg),true,8,0,0,false,false);
    assert(std::strcmp(msg,"START THE NEXT WAVE")==0);
    Describe(msg,sizeof(msg),false,9,0,0,false,false);
    assert(std::strcmp(msg,"OPEN ARMORY")==0);
    Describe(msg,sizeof(msg),true,9,0,0,false,false);
    assert(std::strcmp(msg,"SHOW UPGRADES")==0);
    Describe(msg,sizeof(msg),true,6,0,0,false,false);
    assert(std::strcmp(msg,"AIM AT A CARD TO INSPECT")==0);
    char guarded[9]={};
    guarded[8]='Z';
    Describe(guarded,8,false,0,450,10,false,false);
    assert(guarded[7]=='\0' && guarded[8]=='Z');
    Describe(nullptr,0,false,0,0,0,false,false);
    std::cout<<"Breakwater shop descriptions: labels, status, bounds passed\n";
}
