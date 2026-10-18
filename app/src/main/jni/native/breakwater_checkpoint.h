#ifndef REVERIE_BREAKWATER_CHECKPOINT_H
#define REVERIE_BREAKWATER_CHECKPOINT_H

#include "reverie_native_sdk.h"

namespace reverie { namespace breakwater {

// The existing ABI-v1 host atomically replaces this module-private slot.
// A successful zero-byte write invalidates an old shop checkpoint without
// changing the shared host ABI or the 80-byte campaign save format.
// New names intentionally invalidate the earlier single-save campaign.
// Slots are module-scoped and have no impact on any other ReverieVR save.
constexpr const char* kCheckpointSlots[] = {
    "breakwater-run-1-v2.bin",
    "breakwater-run-2-v2.bin",
    "breakwater-run-3-v2.bin"
};
inline const char* CheckpointSlot(uint8_t slot) {
    return slot<3u?kCheckpointSlots[slot]:nullptr;
}

inline bool ClearCheckpoint(const ReverieNativeHostV1 *host,uint8_t slot=0u) {
    const char* file=CheckpointSlot(slot);
    return file && ReverieNativeHostSupportsSaveV1(host) &&
           host->write_save(file, nullptr, 0u) ==
               REVERIE_NATIVE_SAVE_OK;
}

inline bool IsClearedCheckpoint(int32_t read_result, uint32_t size) {
    return read_result == REVERIE_NATIVE_SAVE_OK && size == 0u;
}

}} // namespace reverie::breakwater
#endif
