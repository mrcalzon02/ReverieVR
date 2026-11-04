#include "breakwater_materials.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::breakwater::materials;
int main() {
    static_assert(kAtlasWidth==128 && kAtlasHeight==448);
    std::array<int,6> counts{};
    Rasterize([&](int x,int y,Rgb rgb) {
        assert(x>=0 && x<kAtlasWidth && y>=kBaseY && y<kAtlasHeight);
        assert(rgb.r!=255 || rgb.g!=0 || rgb.b!=255);
        ++counts[((y-kBaseY)/kTile)*2+x/kTile];
    });
    for(int v:counts) assert(v==kTile*kTile);
    for(int id=0;id<6;++id) {
        auto m=static_cast<Surface>(id);
        const Rgb a=Sample(m,4,7), b=Sample(m,4,7),c=Sample(m,29,37);
        assert(a.r==b.r && a.g==b.g && a.b==b.b);
        assert(a.r!=c.r || a.g!=c.g || a.b!=c.b);
    }
    for(int id=0;id<3;++id) {
        std::vector<float> a;
        auto terrain=[](uint8_t,float x,float z){return -1.037f+.008f*std::sin(x+z);};
        AppendCoast(a,static_cast<uint8_t>(id),terrain);
        assert(a.size()==static_cast<size_t>(kSectionsX*kSectionsZ*6*5));
        for(size_t i=0;i<a.size();i+=5) {
            assert(std::isfinite(a[i]) && std::isfinite(a[i+1]) && std::isfinite(a[i+2]));
            assert(a[i]>=-3.861f && a[i]<=3.861f);
            assert(a[i+2]>=-2.201f && a[i+2]<=-.259f);
            assert(a[i+3]>=0.0f && a[i+3]<=1.0f);
            assert(a[i+4]>=0.0f && a[i+4]<=1.0f);
        }
    }
    for(int id=0;id<6;++id) {
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
    std::vector<float> invalid;
    AppendCoast(invalid,3u,[](uint8_t,float,float){return 0.f;});
    assert(invalid.empty());
    std::cout<<"Breakwater coastal material atlas and geometry OK\n";
}
