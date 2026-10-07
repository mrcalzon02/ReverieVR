#include "procedural_material_atlas.h"

#include <algorithm>
#include <cstdint>

namespace reverie {
namespace procedural {
namespace {

uint32_t PixelHash(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = seed ^ (x * 0x9e3779b9u) ^ (y * 0x85ebca6bu);
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    return h ^ (h >> 16);
}

uint8_t Shade(int value) {
    return static_cast<uint8_t>(std::max(100, std::min(255, value)));
}

uint8_t PixelValue(
    Material material,
    int x,
    int y,
    uint32_t hash
) {
    const int fleck = static_cast<int>(hash % 29u) - 14;
    switch (material) {
        case Material::Stone: {
            const int brick_row = y / 12;
            const int shifted_x = x + ((brick_row & 1) * 13);
            const bool mortar =
                (y % 12) == 0 || (shifted_x % 26) == 0;
            return Shade(211 + fleck - (mortar ? 47 : 0));
        }
        case Material::Wood: {
            const int warped = x + (y / 11) + static_cast<int>((hash >> 7) & 1u);
            const int grain = warped % 13;
            const int knot_x = x - 43;
            const int knot_y = y - 27;
            const bool knot = knot_x * knot_x + knot_y * knot_y < 28;
            return Shade(215 + fleck / 2
                - (grain <= 1 ? 31 : 0) - (knot ? 43 : 0));
        }
        case Material::Metal: {
            const bool seam = (y % 17) == 0;
            const bool rivet =
                (x % 29) <= 2 && (y % 29) <= 2;
            return Shade(225 + fleck / 3
                - (seam ? 20 : 0) - (rivet ? 57 : 0));
        }
        case Material::Paper: {
            const bool rule = y % 13 == 0 && x > 6 && x < 58;
            const bool margin = x == 7;
            return Shade(244 + fleck / 4
                - (rule ? 29 : 0) - (margin ? 16 : 0));
        }
    }
    return 255u;
}

template <typename PixelWriter>
void GeneratePixels(uint32_t seed, PixelWriter write) {
    for (int tile = 0; tile < 4; ++tile) {
        const Material material = static_cast<Material>(tile);
        const int tile_x = (tile & 1) * kTileSize;
        const int tile_y = (tile >> 1) * kTileSize;
        for (int y = 0; y < kTileSize; ++y) {
            for (int x = 0; x < kTileSize; ++x) {
                const uint32_t hash = PixelHash(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(y),
                    seed ^ (static_cast<uint32_t>(tile) * 0x9e3779b9u)
                );
                const uint8_t value = PixelValue(material, x, y, hash);
                const size_t pixel = static_cast<size_t>(tile_y + y)
                    * kAtlasWidth + static_cast<size_t>(tile_x + x);
                write(pixel, value);
            }
        }
    }
}

}  // namespace

bool GenerateMaterialAtlas(uint32_t seed, uint8_t *rgba, size_t capacity) {
    if (rgba == nullptr || capacity < kAtlasBytes) return false;
    GeneratePixels(seed, [rgba](size_t pixel, uint8_t value) {
        const size_t offset = pixel * 4u;
        rgba[offset] = value;
        rgba[offset + 1u] = value;
        rgba[offset + 2u] = value;
        rgba[offset + 3u] = 255u;
    });
    return true;
}

bool GenerateMaterialAtlasRgb565(
    uint32_t seed,
    uint16_t *pixels,
    size_t capacity_bytes
) {
    if (pixels == nullptr || capacity_bytes < kAtlasRgb565Bytes) {
        return false;
    }
    GeneratePixels(seed, [pixels](size_t pixel, uint8_t value) {
        // GLES2 native RGB565: quantize a grayscale procedural pixel
        // directly without allocating an intermediate RGBA atlas.
        const uint16_t r = static_cast<uint16_t>(value >> 3);
        const uint16_t g = static_cast<uint16_t>(value >> 2);
        pixels[pixel] = static_cast<uint16_t>((r << 11) | (g << 5) | r);
    });
    return true;
}

}  // namespace procedural
}  // namespace reverie
