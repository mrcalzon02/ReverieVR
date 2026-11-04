#include "omega_front_portrait_catalog.h"
#include <cassert>
#include <cstring>
#include <set>
#include <string>
#include <iostream>
using namespace reverie::omega_art;
int main(){
    assert(kPortraitFactionCount==5 && kPortraitsPerFaction==20);
    assert(PortraitAt(-1,0)==nullptr && PortraitAt(5,0)==nullptr);
    assert(PortraitAt(0,-1)==nullptr && PortraitAt(0,20)==nullptr);
    assert(std::strcmp(kPortraitFactionIds[0],"F06")==0);
    assert(std::strcmp(kPortraitFactionIds[4],"F10")==0);
    assert(std::strcmp(PortraitAt(0,0)->role,"legionary_shield")==0);
    assert(std::strcmp(PortraitAt(4,0)->role,"ghostline_sharpshooter")==0);
    std::set<std::string> paths;
    for(int faction=0;faction<kPortraitFactionCount;++faction)
        for(int slot=0;slot<kPortraitsPerFaction;++slot){
            const PortraitEntry *p=PortraitAt(faction,slot);
            assert(p && p->role && *p->role && p->path);
            const std::string path(p->path);
            assert(path.find("native/omega-front/ui/icons/")==0);
            assert(path.size()>4 && path.substr(path.size()-4)==".png");
            assert(paths.insert(path).second);
        }
    assert(paths.size()==100);
    std::cout<<"PASS: Omega Front 100 unique menu portraits\n";
}
