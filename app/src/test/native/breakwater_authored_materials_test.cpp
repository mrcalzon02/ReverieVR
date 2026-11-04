#include "breakwater_authored_materials.h"
#include "reverie_native_asset_scope.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
using namespace reverie::breakwater;

int main(){
    static_assert(authored::kTiles==6u);
    authored::Atlas atlas;
    const auto reader=[](const char* path,void* dest,uint32_t cap,
                         uint32_t* received)->bool{
        std::ifstream f(std::string("app/src/main/assets/")+path,std::ios::binary);
        if(!f)return false;
        const std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                                        std::istreambuf_iterator<char>());
        if(!dest){*received=static_cast<uint32_t>(data.size());return true;}
        if(cap<data.size())return false;
        std::memcpy(dest,data.data(),data.size());
        *received=static_cast<uint32_t>(data.size());return true;
    };
    for(const char* p:authored::kPaths)
        assert(reverie::native::assets::MaximumBytes("breakwater-battery",p)==
               authored::kMaxPngBytes);
    assert(authored::Load(atlas,reader)==authored::kTiles);
    std::array<uint8_t,materials::kAtlasWidth*materials::kAtlasHeight*4u> pixels{};
    materials::Rasterize([&](int x,int y,materials::Rgb c) {
        const size_t pos=(static_cast<size_t>(y)*materials::kAtlasWidth+x)*4u;
        pixels[pos]=c.r;pixels[pos+1]=c.g;pixels[pos+2]=c.b;pixels[pos+3]=255;
    });
    const auto fallback=pixels;
    assert(authored::Apply(atlas,pixels)==6u);
    for(size_t i=0;i<authored::kTiles;++i) {
        const size_t at=((materials::kBaseY+(i/2u)*64u+17u)*
                         materials::kAtlasWidth+(i%2u)*64u+19u)*4u;
        const size_t src=(17u*64u+19u)*4u;
        for(size_t k=0;k<4u;++k)
            assert(pixels[at+k]==atlas.rgba[i][src+k]);
        assert(pixels[at+3u]==255u);
    }
    // Incomplete packs safely retain their old procedural counterpart.
    authored::Atlas incomplete=atlas;
    incomplete.rgba[4u].clear();
    auto recovered=fallback;
    assert(authored::Apply(incomplete,recovered)==5u);
    const size_t missing=((materials::kBaseY+2u*64u+17u)*
                          materials::kAtlasWidth+19u)*4u;
    for(size_t k=0;k<4u;++k)assert(recovered[missing+k]==fallback[missing+k]);
    authored::Atlas empty;
    assert(authored::Load(empty,[](auto...){return false;})==0u);
    assert(authored::Apply(empty,recovered)==0u);
    std::puts("PASS: six actual authored Breakwater PNGs, atlas copy and fallback");
}
