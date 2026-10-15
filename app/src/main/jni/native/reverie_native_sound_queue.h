#ifndef REVERIE_NATIVE_SOUND_QUEUE_H
#define REVERIE_NATIVE_SOUND_QUEUE_H

#include "reverie_native_module.h"
#include <cstdint>

// Host-internal bounded event packet. Small cue IDs (1..5) are appended
// oldest first and encoded as consecutive bytes in a uint64_t. The caller
// provides synchronization; this class does not allocate, play audio, or
// change the public ABI. One packet is drained once per native update.
struct ReverieNativeSoundQueueV1 {
    uint64_t packed=0u;
    uint32_t count=0u;
    bool Push(uint32_t cue) {
        if(cue<REVERIE_NATIVE_SOUND_WEAPON_MACHINE ||
           cue>REVERIE_NATIVE_SOUND_MAX_V1 || count>=8u) return false;
        packed |= static_cast<uint64_t>(cue) << (8u*count++);
        return true;
    }
    uint64_t Drain() {
        const uint64_t result=packed;
        Clear();
        return result;
    }
    void Clear() {packed=0u;count=0u;}
};
#endif
