#include "procedural_material_atlas.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace reverie::procedural;

int main() {
    static_assert(kAtlasWidth == 128, "atlas width changed");
    static_assert(kAtlasHeight == 128, "atlas height changed");
    static_assert(kAtlasBytes == 65536u, "RGBA reference budget changed");
    static_assert(kAtlasRgb565Bytes == 32768u, "RGB565 budget changed");

    std::vector<uint8_t> first(kAtlasBytes + 8u, 0xa5u);
    std::vector<uint8_t> second(kAtlasBytes, 0u);
    std::vector<uint8_t> alternate(kAtlasBytes, 0u);
    assert(!GenerateMaterialAtlas(0u, nullptr, kAtlasBytes));
    assert(!GenerateMaterialAtlas(0u, first.data(), kAtlasBytes - 1u));
    for (uint8_t value : first) assert(value == 0xa5u);

    assert(GenerateMaterialAtlas(kRedLedgerMaterialSeed,
        first.data(), kAtlasBytes));
    assert(GenerateMaterialAtlas(kRedLedgerMaterialSeed,
        second.data(), second.size()));
    assert(GenerateMaterialAtlas(kRedLedgerMaterialSeed + 1u,
        alternate.data(), alternate.size()));
    for (size_t i = 0; i < kAtlasBytes; ++i) {
        assert(first[i] == second[i]);
        if ((i & 3u) == 3u) assert(first[i] == 255u);
    }
    for (size_t i = kAtlasBytes; i < first.size(); ++i) {
        assert(first[i] == 0xa5u);
    }

    bool seed_changes_output = false;
    for (size_t i = 0; i < kAtlasBytes; ++i) {
        if (first[i] != alternate[i]) seed_changes_output = true;
    }
    assert(seed_changes_output);

    std::array<uint64_t, 4> checksums{};
    for (int tile = 0; tile < 4; ++tile) {
        uint64_t checksum = 1469598103934665603ull;
        int min_value = 255;
        int max_value = 0;
        for (int y = 0; y < kTileSize; ++y) {
            for (int x = 0; x < kTileSize; ++x) {
                const size_t index = (
                    static_cast<size_t>((tile / 2) * kTileSize + y)
                    * kAtlasWidth + (tile % 2) * kTileSize + x
                ) * 4u;
                const uint8_t value = first[index];
                assert(value == first[index + 1u]);
                assert(value == first[index + 2u]);
                if (value < min_value) min_value = value;
                if (value > max_value) max_value = value;
                checksum = (checksum ^ value) * 1099511628211ull;
            }
        }
        assert(max_value > min_value + 20);
        checksums[static_cast<size_t>(tile)] = checksum;
    }
    for (size_t a = 0; a < checksums.size(); ++a) {
        for (size_t b = a + 1u; b < checksums.size(); ++b) {
            assert(checksums[a] != checksums[b]);
        }
    }

    // RGB565 is generated directly, is seed-deterministic, respects byte
    // capacity/guard bytes and matches the RGBA recipe after quantization.
    std::vector<uint16_t> compact(kAtlasPixelCount + 4u, 0xa55au);
    std::vector<uint16_t> compact_copy(kAtlasPixelCount, 0u);
    std::vector<uint16_t> compact_other(kAtlasPixelCount, 0u);
    assert(!GenerateMaterialAtlasRgb565(0u, nullptr, kAtlasRgb565Bytes));
    assert(!GenerateMaterialAtlasRgb565(kRedLedgerMaterialSeed,
        compact.data(), kAtlasRgb565Bytes - 1u));
    for (uint16_t value : compact) assert(value == 0xa55au);
    assert(GenerateMaterialAtlasRgb565(kRedLedgerMaterialSeed,
        compact.data(), kAtlasRgb565Bytes));
    assert(GenerateMaterialAtlasRgb565(kRedLedgerMaterialSeed,
        compact_copy.data(), kAtlasRgb565Bytes));
    assert(GenerateMaterialAtlasRgb565(kRedLedgerMaterialSeed + 1u,
        compact_other.data(), kAtlasRgb565Bytes));
    bool compact_seed_changes = false;
    for (size_t pixel = 0; pixel < kAtlasPixelCount; ++pixel) {
        assert(compact[pixel] == compact_copy[pixel]);
        if (compact[pixel] != compact_other[pixel]) {
            compact_seed_changes = true;
        }
        const uint8_t value = first[pixel * 4u];
        const uint16_t r = static_cast<uint16_t>(value >> 3);
        const uint16_t g = static_cast<uint16_t>(value >> 2);
        const uint16_t expected = static_cast<uint16_t>(
            (r << 11) | (g << 5) | r
        );
        assert(compact[pixel] == expected);
    }
    assert(compact_seed_changes);
    for (size_t pixel = kAtlasPixelCount; pixel < compact.size(); ++pixel) {
        assert(compact[pixel] == 0xa55au);
    }

    std::cout << "procedural atlas smoke passed; RGB565 bytes="
        << kAtlasRgb565Bytes << " RGBA reference bytes="

        << kAtlasBytes << " tiles=4; deterministic=true\n";
    return 0;
}
