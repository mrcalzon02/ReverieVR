// Source-only native terrain proof. Not registered as a playable game until
// licensed Sitka geography, shell content scope and real activities exist.
#include "sitka_terrain_gl.h"
#include <cstdint>
#include <new>
#include <string>
#include <vector>

namespace {
struct State {
    const ReverieNativeHostV1* host = nullptr;
    reverie::sitka::TerrainMesh terrain;
    reverie::sitka::TerrainGl gl;
};
void Log(const ReverieNativeHostV1* host, const char* message) {
    ReverieNativeLogV1(host, REVERIE_NATIVE_LOG_ERROR, "SitkaTerrain", message);
}
void* Create(const ReverieNativeHostV1* host) {
    if (!ReverieNativeHostSupportsLogV1(host) ||
        host->struct_size < REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE ||
        host->read_asset == nullptr) return nullptr;
    // The host's game-private read_asset service enforces path/size scope.
    // No synthetic geographic fallback is ever used in the installed game.
    const char* path = "sitka/coast/terrain.sitkam";
    uint32_t size = 0u;
    const int32_t probe = host->read_asset(path, nullptr, 0u, &size);
    const size_t max_bytes = reverie::sitka::kTerrainHeaderBytes +
        size_t(reverie::sitka::kMaxTerrainVertices)*reverie::sitka::kTerrainVertexBytes +
        size_t(reverie::sitka::kMaxTerrainTriangles)*12u;
    if ((probe != REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
         probe != REVERIE_NATIVE_SAVE_OK) || size < 32u || size > max_bytes) {
        Log(host, "No bounded, approved Sitka terrain asset is available.");
        return nullptr;
    }
    std::vector<uint8_t> bytes(size);
    uint32_t actual = 0u;
    if (host->read_asset(path, bytes.data(), size, &actual) != REVERIE_NATIVE_SAVE_OK ||
        actual != size) {
        Log(host, "Sitka terrain asset read was incomplete.");
        return nullptr;
    }
    try {
        State* state = new State;
        state->host = host;
        try { state->terrain = reverie::sitka::ParseTerrainMesh(bytes.data(), bytes.size()); }
        catch (...) { delete state; throw; }
        return state;
    } catch (const std::exception& e) {
        Log(host, e.what());
        return nullptr;
    }
}
void Destroy(void* instance) { delete static_cast<State*>(instance); }
int32_t OnGlCreated(void* instance) {
    State* state = static_cast<State*>(instance);
    if (!state) return 0;
    std::string error;
    if (state->gl.Create(state->terrain, &error)) return 1;
    Log(state->host, error.c_str());
    return 0;
}
void ReleaseGl(void* instance) {
    if (instance) static_cast<State*>(instance)->gl.Release();
}
void Resume(void*) {}
void Pause(void*) {}
void Update(void*, const ReverieNativeInputV1*) {}
int32_t RenderEye(void* instance, const ReverieNativeEyeV1* eye) {
    return instance && static_cast<State*>(instance)->gl.RenderEye(eye) ? 1 : 0;
}
const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1), REVERIE_NATIVE_MODULE_ABI_VERSION,
     "sitka", "Sitka [TERRAIN PROOF]", 2u, 0u},
    Create, Destroy, OnGlCreated, ReleaseGl, Resume, Pause, Update, RenderEye, nullptr
};
} // namespace
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1* reverie_native_module_entry_v1(void) { return &kApi; }
