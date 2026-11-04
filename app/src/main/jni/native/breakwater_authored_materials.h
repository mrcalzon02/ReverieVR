#ifndef REVERIE_BREAKWATER_AUTHORED_MATERIALS_H
#define REVERIE_BREAKWATER_AUTHORED_MATERIALS_H

// Six owner-generated 64x64 PNG material tiles. Decode exactly once on
// module creation using the already-shipped bounded PNG/zlib decoder.
// Missing or damaged art leaves the deterministic procedural tiles intact.
#include "breakwater_materials.h"
#include "omega_front_png.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace reverie { namespace breakwater { namespace authored {
constexpr size_t kTiles=6u;
constexpr size_t kMaxPngBytes=16384u;
constexpr std::array<const char*,kTiles> kPaths={{
    "native/breakwater/materials/shingle.png",
    "native/breakwater/materials/quay_concrete.png",
    "native/breakwater/materials/black_basalt.png",
    "native/breakwater/materials/oxidized_steel.png",
    "native/breakwater/materials/sandbag_burlap.png",
    "native/breakwater/materials/olive_enamel.png"
}};
struct Atlas {
    std::array<std::vector<uint8_t>,kTiles> rgba{};
    size_t count=0u;
    bool Present(size_t tile) const {
        return tile<kTiles && rgba[tile].size()==
            static_cast<size_t>(materials::kTile*materials::kTile*4);
    }
};

template<class ReadAsset>
inline size_t Load(Atlas &atlas,ReadAsset read) {
    atlas=Atlas{};
    for(size_t tile=0;tile<kTiles;++tile) {
        uint32_t length=0u;
        if(!read(kPaths[tile],nullptr,0u,&length) || length<45u ||
           length>kMaxPngBytes)continue;
        std::vector<uint8_t> bytes(length);
        uint32_t received=0u;
        if(!read(kPaths[tile],bytes.data(),length,&received) ||
           received!=length)continue;
        omega_art::Image decoded;
        if(!omega_art::DecodePng(bytes.data(),bytes.size(),decoded) ||
           decoded.width!=materials::kTile ||
           decoded.height!=materials::kTile ||
           decoded.rgba.size()!=materials::kTile*materials::kTile*4u)
            continue;
        bool opaque=true;
        for(size_t i=3;i<decoded.rgba.size();i+=4u)
            if(decoded.rgba[i]!=255u){opaque=false;break;}
        if(!opaque)continue;
        atlas.rgba[tile]=std::move(decoded.rgba);
        ++atlas.count;
    }
    return atlas.count;
}

template<size_t N>
inline size_t Apply(const Atlas &atlas,std::array<uint8_t,N>&pixels) {
    static_assert(N==static_cast<size_t>(materials::kAtlasWidth)*
                     materials::kAtlasHeight*4u,"material atlas shape drift");
    size_t copied=0u;
    for(size_t tile=0;tile<kTiles;++tile) {
        if(!atlas.Present(tile))continue;
        const size_t dx=(tile%2u)*materials::kTile;
        const size_t dy=materials::kBaseY+(tile/2u)*materials::kTile;
        for(size_t y=0;y<materials::kTile;++y)
            for(size_t x=0;x<materials::kTile;++x) {
                const size_t src=(y*materials::kTile+x)*4u;
                const size_t dst=((dy+y)*materials::kAtlasWidth+dx+x)*4u;
                for(size_t k=0;k<4u;++k)
                    pixels[dst+k]=atlas.rgba[tile][src+k];
            }
        ++copied;
    }
    return copied;
}
}}} // namespace reverie::breakwater::authored
#endif
