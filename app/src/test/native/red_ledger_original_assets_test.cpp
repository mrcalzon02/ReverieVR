#include "red_ledger_product_assets.h"
#include "red_ledger_original_fbx.h"
#include "red_ledger_original_png.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
using namespace reverie::redledger;
static std::vector<uint8_t> Read(const std::string &path) {
    std::ifstream in(path,std::ios::binary);
    assert(in.good());
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>());
}
int main(int argc,char **argv) {
    assert(argc==2);
    const std::string base=std::string(argv[1])+"/app/src/main/assets/";
    for(int i=0;i<products::kCount;++i) {
        const products::Entry &product=products::kCatalog[i];
        assert(products::IndexForOffer(product.offer)==i);
        for(int j=0;j<i;++j)assert(product.offer!=products::kCatalog[j].offer);
        const auto fbx_data=Read(base+product.mesh_path);
        const auto png_data=Read(base+product.texture_path);
        std::vector<float> triangles;
        assert(fbx::BuildWorldTriangles(fbx_data.data(),fbx_data.size(),
                                        &triangles,product.model_name));
        assert(!triangles.empty()&&triangles.size()%10u==0u);
        assert(triangles.size()/10u<=90000u);
        for(float value:triangles)assert(std::isfinite(value));
        std::vector<float> reject;
        assert(!fbx::BuildWorldTriangles(fbx_data.data(),fbx_data.size(),
                                         &reject,"UCX_101beer"));
        std::vector<uint8_t> pixels;
        assert(original_png::DecodeRgba(png_data.data(),png_data.size(),
                                        1024u,&pixels));
        assert(pixels.size()==1024u*1024u*4u);
        auto damaged=png_data;
        damaged[29]^=0x40u;
        assert(!original_png::DecodeRgba(damaged.data(),damaged.size(),
                                         1024u,&pixels));
    }
    assert(products::IndexForOffer(5)==-1);
    assert(products::IndexForOffer(trade::kOffers)==-1);
    std::cout<<"Four original merchant meshes and textures validated\n";
}
