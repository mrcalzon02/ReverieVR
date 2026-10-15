#ifndef REVERIE_BREAKWATER_CHECKPOINT_H
#define REVERIE_BREAKWATER_CHECKPOINT_H

#include "reverie_native_sdk.h"

namespace reverie { namespace breakwater {

// The existing ABI-v1 host atomically replaces this module-private slot.
// A successful zero-byte write invalidates an old shop checkpoint without
// changing the shared host ABI or the 80-byte campaign save format.
constexpr char kCheckpointSlot[] = "breakwater-v1.bin";

inline bool ClearCheckpoint(const ReverieNativeHostV1 *host) {
    return ReverieNativeHostSupportsSaveV1(host) &&
           host->write_save(kCheckpointSlot, nullptr, 0u) ==
               REVERIE_NATIVE_SAVE_OK;
}

inline bool IsClearedCheckpoint(int32_t read_result, uint32_t size) {
    return read_result == REVERIE_NATIVE_SAVE_OK && size == 0u;
}

}} // namespace reverie::breakwater
#endif
