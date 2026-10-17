#include "breakwater_environment.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using namespace reverie::breakwater;

struct SkyMeshSummary {
    int triangles=0;
    int zenith_triangles=0;
    int invalid_coordinates=0;
    float apex_r=0.0f,apex_g=0.0f,apex_b=0.0f;
};

static SkyMeshSummary Summarize(uint8_t location,bool night) {
    SkyMeshSummary data;
    auto triangle=[&](float ax,float ay,float az,
                      float bx,float by,float bz,
                      float cx,float cy,float cz,
                      float r,float g,float b) {
        for(float v : {ax,ay,az,bx,by,bz,cx,cy,cz,r,g,b}) {
            if(!std::isfinite(v))++data.invalid_coordinates;
        }
        // The apex is shared in world coordinates across every sky
        // sector and is not a head-facing per-eye billboard.
        if(cx==0.0f && cy==40.0f && cz==0.0f) {
            ++data.zenith_triangles;
            assert(std::abs(std::hypot(ax,az)-kSkyRadius)<.001f);
            assert(std::abs(std::hypot(bx,bz)-kSkyRadius)<.001f);
            assert(std::abs(ay-17.0f)<.0001f);
            assert(std::abs(by-17.0f)<.0001f);
            if(data.zenith_triangles==1) {
                data.apex_r=r;data.apex_g=g;data.apex_b=b;
            } else {
                assert(r==data.apex_r && g==data.apex_g &&
                       b==data.apex_b);
            }
        }
        ++data.triangles;
    };
    EmitSkyPanorama(location,night,triangle);
    return data;
}

int main() {
    static_assert(kSkySegments>=12,"VR sky too coarsely segmented");
    static_assert(kSkyBands>=5,"Day/night zenith needs stacked gradients");
    assert(kSkyRadius>kMountainRadius); // environment skyline remains in front
    for(uint8_t location=0;location<Campaign::kLocations;++location) {
        const SkyMeshSummary day=Summarize(location,false);
        const SkyMeshSummary night=Summarize(location,true);
        const int plumeTriangles=(location==1?6:location==2?2:0);
        assert(day.triangles==kSkySegments*kSkyBands*2+
               kSkySegments+12*3+10+plumeTriangles);
        assert(night.triangles==kSkySegments*kSkyBands*2+
               kSkySegments+8*3+10+24*2+plumeTriangles);
        assert(day.zenith_triangles==kSkySegments);
        assert(night.zenith_triangles==kSkySegments);
        assert(day.invalid_coordinates==0 && night.invalid_coordinates==0);
        assert(day.apex_r>night.apex_r &&
               day.apex_g>night.apex_g &&
               day.apex_b>night.apex_b);
        assert(night.apex_b>night.apex_r);
    }
    assert(Summarize(Campaign::kLocations,false).triangles==0);
    assert(Summarize(Campaign::kLocations,true).triangles==0);
    std::cout<<"Breakwater closed 360 sky zenith and palettes OK\n";
}
