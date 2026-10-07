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
    static_assert(kAtlasBytes == 65536u, "atlas budget changed");

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

    std::cout << "procedural atlas smoke passed; bytes="
        << kAtlasBytes << " tiles=4; deterministic=true\n";
    return 0;
}
