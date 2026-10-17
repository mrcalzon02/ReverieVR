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
    trade::Trade inventory;
    assert(products::UnitsOnHand(inventory,0)==1);
    assert(products::UnitsOnHand(inventory,3)==1);
    assert(products::UnitsOnHand(inventory,4)==1);
    assert(products::UnitsOnHand(inventory,7)==0);
    assert(products::UnitsOnHand(inventory,-1)==0);
    assert(products::UnitsOnHand(inventory,products::kCount)==0);
    for(int i=0;i<products::kCount;++i) {
        const auto shelf=products::StockShelf(i);
        assert(shelf.scale>0.f && shelf.scale<1.f);
        assert(shelf.z < -1.0f && shelf.x > -2.f && shelf.x < 1.5f);
        assert(shelf.y > -.8f && shelf.y < .5f);
        assert(products::kCatalog[i].offer>0);
        if(i>0 && i%4==0)assert(products::StockShelf(i-1).x<shelf.x);
        if(i>0 && i%4!=0) {
            assert(products::StockShelf(i-1).x==shelf.x);
            assert(products::StockShelf(i-1).y<shelf.y);
        }
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
        std::vector<uint8_t> gpu;
        assert(original_png::Reduce1024RgbaTo512(pixels,&gpu));
        assert(gpu.size()==512u*512u*4u);
        assert(!original_png::Reduce1024RgbaTo512({},&gpu));
        assert(gpu.size()==512u*512u*4u);
        auto damaged=png_data;
        damaged[29]^=0x40u;
        assert(!original_png::DecodeRgba(damaged.data(),damaged.size(),
                                         1024u,&pixels));
    }
    assert(inventory.SelectOffer(2));
    assert(products::UnitsOnHand(inventory,1)==0);
    assert(inventory.AcceptStock());
    assert(products::UnitsOnHand(inventory,1)==trade::kDrinkCatalog[2].units);
    assert(products::StockPodium().scale==1.f);
    assert(products::BarCounter().scale<1.f);
    assert(products::IndexForOffer(5)==5);
    assert(products::IndexForOffer(20)==7);
    assert(products::IndexForOffer(7)==-1);
    assert(products::IndexForOffer(trade::kOffers)==-1);
    std::cout<<"Eight original merchant meshes and textures validated\n";
}
