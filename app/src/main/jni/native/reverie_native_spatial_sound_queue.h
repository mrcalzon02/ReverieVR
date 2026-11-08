#ifndef REVERIE_NATIVE_SPATIAL_SOUND_QUEUE_H
#define REVERIE_NATIVE_SPATIAL_SOUND_QUEUE_H
#include "reverie_native_module.h"
// Separate, bounded two-event queue. Legacy cue queue remains byte-identical.
// 24 bits/event: cue ID, left gain, right gain. Pure host utility.
#include <cmath>
#include <cstdint>
struct ReverieNativeSpatialSoundQueue {
    uint64_t packed=0u;
    unsigned count=0u;
    bool Push(uint32_t cue,float left,float right){
        const bool supported=(cue>=1u&&cue<=21u)||
            (cue>=REVERIE_NATIVE_SOUND_BREAKWATER_AMBIENCE_SURF&&
             cue<=REVERIE_NATIVE_SOUND_BREAKWATER_GUN_TRAVERSE);
        if(!supported||count>=2u||!std::isfinite(left)||
           !std::isfinite(right)||left<0.f||left>1.f||
           right<0.f||right>1.f)return false;
        const uint64_t bits=cue|
            (static_cast<uint64_t>(std::lround(left*255.f))<<8u)|
            (static_cast<uint64_t>(std::lround(right*255.f))<<16u);
        packed|=bits<<(24u*count++);
        return true;
    }
    uint64_t Drain(){const auto value=packed;Clear();return value;}
    void Clear(){packed=0u;count=0u;}
};
#endif
