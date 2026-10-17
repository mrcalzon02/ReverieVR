#include "breakwater_environment.h"
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    static_assert(kLandscapeSectors==48,"seam coverage");
    static_assert(kHillRadius<kMountainRadius,"real depth between ridges");
    static_assert(kMountainRadius<kSkyRadius,"panorama behind mountains");
    for(unsigned location=0;location<Campaign::kLocations;++location) {
        const auto day=EnvironmentFor(location,false);
        const auto night=EnvironmentFor(location,true);
        assert(day.earth.r>night.earth.r);
        assert(day.sea.b>night.sea.b);
        // Entire 360-degree circle closes without a silhouette seam.
        assert(RidgeHeight(location,0,true)==
               RidgeHeight(location,kLandscapeSectors,true));
        assert(RidgeHeight(location,0,false)==
               RidgeHeight(location,kLandscapeSectors,false));
        // Rear elevation is pronounced; frontal water stays visible.
        assert(RidgeHeight(location,0,true)>RidgeHeight(location,24,true));
        for(bool dark : {false,true}) {
            int triangles=0,boxes=0;
            float groundFarZ=0;
            auto tri=[&](float x,float y,float z,float a,float b,float c,
                         float d,float e,float f,float r,float g,float blue){
                for(float value:{x,y,z,a,b,c,d,e,f,r,g,blue})
                    assert(std::isfinite(value));
                // Ground strip triangles have a center x=0 and flank x=9.4.
                if((x==0 || a==0 || d==0) &&
                   (std::abs(x)>9.0f || std::abs(a)>9.0f ||
                    std::abs(d)>9.0f))
                    groundFarZ=std::max(groundFarZ,
                        std::max(z,std::max(c,f)));
                ++triangles;
            };
            auto box=[&](float x,float y,float z,float w,float h,float depth,
                         float r,float g,float b){
                for(float value:{x,y,z,w,h,depth,r,g,b})
                    assert(std::isfinite(value));
                ++boxes;
            };
            EmitSurroundingLandscape(location,dark,box,tri);
            // Bounded geometry, no unbounded tessellation per stereo eye.
            assert(triangles>=240 && triangles<420);
            assert(boxes<50);
            assert(groundFarZ>kHillRadius-.20f);
        }
    }
    assert(EnvironmentFor(4,false).sea.r==0);
    assert(EnvironmentFor(0,false).earth.g!=EnvironmentFor(1,false).earth.g);
    assert(EnvironmentFor(1,false).peaks!=EnvironmentFor(2,false).peaks);
    const auto day=IlluminateWorld({.6f,.6f,.6f},false);
    const auto night=IlluminateWorld({.6f,.6f,.6f},true);
    assert(day.r>night.r && day.g>night.g && day.b>night.b);
    assert(night.b>night.r);
    std::cout<<"Breakwater 360 landscape, locations, and day/night OK\n";
}
