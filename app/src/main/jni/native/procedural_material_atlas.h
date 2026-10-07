#ifndef REVERIE_PROCEDURAL_MATERIAL_ATLAS_H
#define REVERIE_PROCEDURAL_MATERIAL_ATLAS_H

#include <cstddef>
#include <cstdint>

namespace reverie {
namespace procedural {

// Four grayscale material recipes share a single tiny GLES2 texture.
// The seed and source are the asset; no bitmap is shipped in the APK.
enum class Material : uint8_t {
    Stone = 0,
    Wood = 1,
    Metal = 2,
    Paper = 3
};

constexpr int kTileSize = 64;
constexpr int kAtlasWidth = kTileSize * 2;
constexpr int kAtlasHeight = kTileSize * 2;
constexpr size_t kAtlasBytes =
    static_cast<size_t>(kAtlasWidth) * kAtlasHeight * 4u;
constexpr uint32_t kRedLedgerMaterialSeed = 0x52ED1ED6u;

// RGBA8, deterministic, no GL calls or dynamic allocation. Reject undersized
// buffers before writing. Intended to run once per GL context, not per frame.
bool GenerateMaterialAtlas(
    uint32_t seed,
    uint8_t *rgba,
    size_t capacity
);

}  // namespace procedural
}  // namespace reverie

#endif
