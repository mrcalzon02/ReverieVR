#include "breakwater_materials.h"
#include "breakwater_terrain.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::breakwater::materials;
int main() {
    static_assert(kAtlasWidth==128 && kAtlasHeight==768);
    std::array<int,kMaterialCount> counts{};
    Rasterize([&](int x,int y,Rgb rgb) {
        assert(x>=0 && x<kAtlasWidth && y>=kBaseY && y<kAtlasHeight);
        assert(rgb.r!=255 || rgb.g!=0 || rgb.b!=255);
        ++counts[((y-kBaseY)/kTile)*2+x/kTile];
    });
    for(int v:counts) assert(v==kTile*kTile);
    for(int id=0;id<kMaterialCount;++id) {
        auto m=static_cast<Surface>(id);
        const Rgb a=Sample(m,4,7), b=Sample(m,4,7),c=Sample(m,29,37);
        assert(a.r==b.r && a.g==b.g && a.b==b.b);
        assert(a.r!=c.r || a.g!=c.g || a.b!=c.b);
    }
    assert(CoastSurfaceAt(0,0,0)==Surface::PebbleGround);
    assert(CoastSurfaceAt(0,6,18)==Surface::TrenchSoil);
    assert(CoastSurfaceAt(1,1,12)==Surface::DockPlanks);
    assert(CoastSurfaceAt(2,9,2)==Surface::Basalt);
    for(int id=0;id<4;++id) {
        std::vector<float> a;
        auto terrain=[](uint8_t,float x,float z){return -1.037f+.008f*std::sin(x+z);};
        AppendCoast(a,static_cast<uint8_t>(id),terrain);
        constexpr size_t bankTriangles=2u*9u*2u;
        assert(a.size()==static_cast<size_t>(
            (kSectionsX*kSectionsZ*2u+bankTriangles)*3u*5u));
        // Adjacent cells sample contiguous UVs within the same tile.
        assert(std::abs(a[8]-a[33])<.000001f);
        assert(std::abs(a[29]-a[kSectionsX*30+4])<.000001f);
        bool leftBank=false,rightBank=false;
        for(size_t j=kSectionsX*kSectionsZ*30u;j<a.size();j+=5) {
            leftBank|=a[j]<-3.87f;
            rightBank|=a[j]>3.87f;
        }
        assert(leftBank&&rightBank);
        for(size_t i=0;i<a.size();i+=5) {
            assert(std::isfinite(a[i]) && std::isfinite(a[i+1]) && std::isfinite(a[i+2]));
            assert(a[i]>=-4.581f && a[i]<=4.581f);
            assert(a[i+2]>=-2.231f && a[i+2]<=-.199f);
            assert(a[i+3]>=0.0f && a[i+3]<=1.0f);
            assert(a[i+4]>=0.0f && a[i+4]<=1.0f);
        }
    }
    for(unsigned location=0;location<4;++location) {
        const auto land=EnvironmentFor(static_cast<uint8_t>(location),false);
        for(float x:{-3.86f,-2.7f,0.f,2.7f,3.86f}) {
            const float joined=BeachHeight(location,x,-.26f);
            const float rear=RearGroundHeight(land,x,-.26f);
            assert(std::abs(joined-rear)<.00001f);
            assert(std::isfinite(BeachHeight(location,x,-2.20f)));
        }
    }
    for(int id=0;id<kMaterialCount;++id) {
        std::vector<float> shell;
        assert(AppendBoxCladding(shell,static_cast<Surface>(id),
                                 3.f,-.5f,-1.4f,.7f,.4f,.9f));
        assert(shell.size()==12u*3u*5u);
        for(size_t i=0;i<shell.size();i+=5) {
            for(size_t j=0;j<5;++j)assert(std::isfinite(shell[i+j]));
            assert(shell[i+3]>=0 && shell[i+3]<=1);
            assert(shell[i+4]>=0 && shell[i+4]<=1);
        }
        const size_t before=shell.size();
        assert(!AppendBoxCladding(shell,static_cast<Surface>(id),
                                  0.f,0.f,0.f,0.f,1.f,1.f));
        assert(shell.size()==before);
    }
    assert(CoastSurfaceAt(3,0,1)==Surface::MossySeawall);
    assert(CoastSurfaceAt(3,8,1)==Surface::PebbleGround);
    assert(CoastSurfaceAt(3,8,2)==Surface::Quay);
    assert(CoastSurfaceAt(3,8,19)==Surface::ReinforcedConcrete);
    std::vector<float> invalid;
    AppendCoast(invalid,4u,[](uint8_t,float,float){return 0.f;});
    assert(invalid.empty());
    std::cout<<"Breakwater coastal material atlas and geometry OK\n";
}
