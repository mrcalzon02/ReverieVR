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
        // Prop placement must use exact split-triangle surface heights:
        // sampling the diagonal and the two outer edges must agree with
        // the emitted 12-strip mesh, for all three rear slope profiles.
        for(int strip=0;strip<12;++strip) {
            const float z0=-.50f+strip*1.45f;
            const float z1=z0+1.45f;
            const float t0=strip/12.0f,t1=(strip+1)/12.0f;
            const float center0=-1.12f+day.rise*t0*t0*.80f;
            const float center1=-1.12f+day.rise*t1*t1*.80f;
            const float flank0=center0+.38f*t0;
            const float flank1=center1+.38f*t1;
            assert(std::abs(RearGroundHeight(day,0,z0)-center0)<.0001f);
            assert(std::abs(RearGroundHeight(day,0,z1)-center1)<.0001f);
            assert(std::abs(RearGroundHeight(day,9.4f,z0)-flank0)<.0001f);
            assert(std::abs(RearGroundHeight(day,-9.4f,z1)-flank1)<.0001f);
            const float v=.35f,u=.68f;
            const float z=z0+v*1.45f,x=u*9.4f;
            const float lower=(1-u)*center0+(u-v)*flank0+v*flank1;
            assert(std::abs(RearGroundHeight(day,x,z)-lower)<.0001f);
             assert(std::abs(RearGroundHeight(day,v*9.4f,z0+u*1.45f)-
                            ((1-u)*center0+v*flank1+(u-v)*center1))<.0001f);
         }
        assert(std::abs(RearGroundHeight(day,3.8f,8.1f)-
                        RearGroundHeight(night,3.8f,8.1f))<.00001f);
        // Entire 360-degree circle closes without a silhouette seam.
        assert(RidgeHeight(location,0,true)==
               RidgeHeight(location,kLandscapeSectors,true));
        assert(RidgeHeight(location,0,false)==
               RidgeHeight(location,kLandscapeSectors,false));
        // Rear elevation is pronounced; frontal water stays visible.
        assert(RidgeHeight(location,0,true)>RidgeHeight(location,24,true));
        for(bool dark : {false,true}) {
            int triangles=0,boxes=0,anchoredTrees=0;
            int anchoredTrack=0,anchoredSheds=0,anchoredChimneys=0;
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
                if(location==0 && w==.10f && h==.50f &&
                   depth==.10f) {
                    assert(std::abs(y-h*.50f-
                        RearGroundHeight(day,x,z))<.0001f);
                    ++anchoredTrees;
                }
                if(location==0 && w==.80f && h==.025f &&
                   depth==1.55f) {
                    assert(std::abs(y-.015f-
                        RearGroundHeight(day,x,z))<.0001f);
                    ++anchoredTrack;
                }
                if(location==1 && w==2.3f && h==1.45f &&
                   depth==3.0f) {
                    assert(std::abs(y-h*.50f-
                        RearGroundHeight(day,x,z))<.0001f);
                    ++anchoredSheds;
                }
                if(location==1 && w==.42f && h==1.2f &&
                   depth==.42f) {
                    assert(std::abs(y-h*.50f-
                        RearGroundHeight(day,x,z))<.0001f);
                    ++anchoredChimneys;
                }
                ++boxes;
            };
            EmitSurroundingLandscape(location,dark,box,tri);
            // Bounded geometry, no unbounded tessellation per stereo eye.
            assert(triangles==(location==0?250:location==1?240:258));
            assert(boxes==(location==0?16:location==1?8:0));
            assert(anchoredTrees==(location==0?10:0));
            assert(anchoredTrack==(location==0?6:0));
            assert(anchoredSheds==(location==1?2:0));
            assert(anchoredChimneys==(location==1?2:0));
            assert(groundFarZ>kHillRadius-.20f);
        }
    }
    assert(EnvironmentFor(4,false).sea.r==0);
    assert(EnvironmentFor(0,false).earth.g!=EnvironmentFor(1,false).earth.g);
    assert(EnvironmentFor(1,false).peaks!=EnvironmentFor(2,false).peaks);
    // True sun/moon facet lighting must respond to wall orientation, not
    // merely multiply all world vertices by a uniform daytime tint.
    const auto sunFace=IlluminateFacet({.6f,.6f,.6f},false,false,
                                      1,0,0,0,0,1);
    const auto sunSide=IlluminateFacet({.6f,.6f,.6f},false,false,
                                      0,1,0,0,0,1);
    const auto moonFace=IlluminateFacet({.6f,.6f,.6f},true,false,
                                       1,0,0,0,0,1);
    const auto moonSide=IlluminateFacet({.6f,.6f,.6f},true,false,
                                       0,1,0,0,0,1);
    assert(sunFace.r!=sunSide.r);
    assert(moonFace.b!=moonSide.b);
    assert(moonFace.b>moonFace.r);
    const auto palette=EnvironmentFor(0,true).hill;
    const auto once=IlluminateFacet(palette,true,true,
                                   1,0,0,0,0,1);
    const auto twice=IlluminateFacet(palette,true,false,
                                    1,0,0,0,0,1);
    assert(once.b>twice.b);
    const auto day=IlluminateWorld({.6f,.6f,.6f},false);
    const auto night=IlluminateWorld({.6f,.6f,.6f},true);
    assert(day.r>night.r && day.g>night.g && day.b>night.b);
    assert(night.b>night.r);
    std::cout<<"Breakwater 360 landscape, locations, and day/night OK\n";
}
