#include "breakwater_ocean.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    static_assert(kOceanCrossSections<=24 && kOceanDepthSections<=32,
                  "Reference Daydream geometry budget");
    for(uint8_t location=0;location<Campaign::kLocations;++location) {
        for(bool night:{false,true}) {
            int triangles=0,nonflat=0;
            float minZ=0,maxRadius=0,minY=100,maxY=-100;
            const auto capture=[&](float x,float y,float z,
                                   float a,float b,float c,
                                   float d,float e,float f,
                                   float red,float green,float blue) {
                for(float v:{x,y,z,a,b,c,d,e,f,red,green,blue})
                    assert(std::isfinite(v));
                assert(red>=0&&green>=0&&blue>=0);
                minZ=std::min(minZ,std::min(z,std::min(c,f)));
                maxRadius=std::max(maxRadius,std::max(
                    std::hypot(x,z),std::max(std::hypot(a,c),std::hypot(d,f))));
                minY=std::min(minY,std::min(y,std::min(b,e)));
                maxY=std::max(maxY,std::max(y,std::max(b,e)));
                if(std::abs(y-b)>.000001f||std::abs(y-e)>.000001f)
                    ++nonflat;
                ++triangles;
            };
            EmitOpenSea(location,night,0.0f,capture);
            assert(triangles==kOceanCrossSections*kOceanDepthSections*2);
            assert(nonflat>triangles/3);
            assert(std::abs(minZ-kOceanFarZ)<.001f);
            assert(maxRadius<kSkyRadius);
            assert(maxY-minY>.04f);
        }
    }
    assert(OceanSurfaceY(0,-2,0)!=OceanSurfaceY(0,-42,0));
    assert(OceanSurfaceY(3,-18,1)!=OceanSurfaceY(3,-18,0));
    int invalid=0;
    EmitOpenSea(Campaign::kLocations,false,0.0f,[&](auto...){++invalid;});
    EmitOpenSea(0,false,NAN,[&](auto...){++invalid;});
    assert(invalid==0);
    std::cout<<"Breakwater bounded 3D ocean and deterministic waves OK\n";
}
