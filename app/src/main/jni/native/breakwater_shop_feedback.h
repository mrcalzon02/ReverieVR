#ifndef REVERIE_BREAKWATER_SHOP_FEEDBACK_H
#define REVERIE_BREAKWATER_SHOP_FEEDBACK_H
// Game-private compact inspection line; the caller supplies authoritative
// Campaign values. Never duplicate purchase logic or allocate on each hover.
#include <cstddef>
#include <cstdio>
namespace reverie { namespace breakwater { namespace shopfeedback {
inline const char* Item(bool armory,int index) {
    static constexpr const char* upgrades[8]={
        "FASTER FIRE","MORE DAMAGE","BIGGER CLIP","AIR DEFENSE",
        "ARTILLERY","AIR STRIKE","AUTO TURRET","HULL REPAIR"
    };
    static constexpr const char* weapons[6]={
        "MACHINE GUN","HEAVY GUN","FLAK GUN","MISSILE POD",
        "NAVAL CANNON","SAM MISSILE"
    };
    if(index<0 || index>=8)return nullptr;
    if(!armory)return upgrades[index];
    return index<6?weapons[index]:nullptr;
}
inline void Describe(char* output,size_t capacity,bool armory,int focus,
                     int price,int credits,bool owned,bool active) {
    if(!output||!capacity)return;
    output[0]='\0';
    if(focus==8) {
        std::snprintf(output,capacity,"START THE NEXT WAVE");
        return;
    }
    if(focus==9) {
        std::snprintf(output,capacity,
            armory?"SHOW UPGRADES":"OPEN ARMORY");
        return;
    }
    const char *item=Item(armory,focus);
    if(!item) {
        std::snprintf(output,capacity,"AIM AT A CARD TO INSPECT");
        return;
    }
    if(armory && focus<5 && owned) {
        std::snprintf(output,capacity,"%s - %s",
                      item,active?"ACTIVE":"EQUIP");
    } else if(price<0) {
        std::snprintf(output,capacity,"%s - MAXED",item);
    } else if(price>credits) {
        std::snprintf(output,capacity,"%s - NEED %d CR",item,price);
    } else {
        std::snprintf(output,capacity,"%s - BUY %d CR",item,price);
    }
}
}}}
#endif
