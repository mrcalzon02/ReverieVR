# ReverieVR Native SDK Helpers

**Status:** implemented provisional shared helper layer. These helpers encode current ABI-v1 consumption rules but do not add or replace ABI fields.

**Authoritative implementation:** `app/src/main/jni/native/reverie_native_sdk.h`

**Synchronization:** governed by `docs/native/CONTRACT_INDEX.md`. Changes to the helper header require a same-change update here and must pass the native documentation drift guard.

## Purpose

The public ABI header describes the binary contract. The SDK helper header turns recurring safe-consumption rules into named reusable functions so games do not hand-roll structure-size/version checks.

Use the helper layer when consuming host, descriptor, input, eye, or module-API structures. Do not copy the same comparisons into each game unless a module deliberately requires a different prefix.

## Current helpers

### `ReverieNativeHostSupportsLogV1`

Returns true only when the host pointer is present, the ABI version is v1, the logging prefix is available, and the log callback is non-null.

### `ReverieNativeHostSupportsSaveV1`

Returns true only when the host pointer is present, the ABI version is v1, the save-service prefix is available, and both read/write save callbacks are non-null.

A module that only needs logging should not require the larger save prefix.

### `ReverieNativeDescriptorHasMandatoryV1`

Validates the ABI version and requires the exact frozen `REVERIE_NATIVE_DESCRIPTOR_V1_SIZE`. The descriptor is embedded before callback pointers in the v1 API table and therefore cannot grow in place without breaking callback offsets.

### `ReverieNativeInputHasBaseV1`

Validates the base input prefix through timing, movement axes, and primary/secondary buttons.

Use this for modules that do not consume the pointer tail.

### `ReverieNativeInputHasPointerV1`

Validates the input prefix through pointer kind, origin, and direction.

Use this before reading any pointer fields.

### `ReverieNativeEyeHasMatricesV1`

Validates the eye prefix through the current view/projection matrices.

### `ReverieNativeApiHasMandatoryV1`

Validates the mandatory module API-table prefix and ABI version. The host still separately verifies descriptor identity, GLES requirement, and required callbacks.

## Canonical module-start pattern

A new native game should normally:

1. include `reverie_native_sdk.h`;
2. choose the smallest host capability it actually requires;
3. keep non-GL state allocation in `create`;
4. create/recreate GPU resources only from the GL-context callback;
5. choose `ReverieNativeInputHasBaseV1` or `ReverieNativeInputHasPointerV1` according to real input needs;
6. use `ReverieNativeEyeHasMatricesV1` before rendering;
7. mutate game state in `update`, not `render_eye`;
8. release GL resources in `release_gl_context`;
9. expose a static `ReverieNativeModuleApiV1` and the canonical v1 entry symbol.

The helper functions centralize capability checks, not game policy. They do not own save schemas, rendering, simulation, interaction targets, or recovery behavior.

## Minimal skeleton

```cpp
#include "reverie_native_sdk.h"

struct GameState {
    const ReverieNativeHostV1 *host = nullptr;
};

void *Create(const ReverieNativeHostV1 *host) {
    if (!ReverieNativeHostSupportsLogV1(host)) {
        return nullptr;
    }

    GameState *state = new (std::nothrow) GameState();
    if (state != nullptr) {
        state->host = host;
    }
    return state;
}

void Update(void *instance, const ReverieNativeInputV1 *input) {
    if (instance == nullptr || !ReverieNativeInputHasBaseV1(input)) {
        return;
    }

    // Advance simulation exactly once for this logical frame.
}

int32_t RenderEye(void *instance, const ReverieNativeEyeV1 *eye) {
    if (instance == nullptr || !ReverieNativeEyeHasMatricesV1(eye)) {
        return 0;
    }

    // Observe simulation state and render this eye without advancing it.
    return 1;
}
```

The complete callback table still needs create/destroy, GL create/release, resume/pause, update, and render-eye functions as specified by `API_V1.md`.

## What graduates here

A helper belongs in this header when:

- at least two native modules need the same correctness rule;
- it can be expressed without game-specific behavior;
- it has a stable relationship to the ABI contract;
- centralizing it reduces the chance of a cross-game regression.

Larger services such as audio, haptics, locomotion declaration, GL-state guards, generated-model recipes, or collision utilities should become separate focused SDK components rather than turning this header into a catch-all.
