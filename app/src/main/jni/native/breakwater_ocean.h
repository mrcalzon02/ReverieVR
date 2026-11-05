#ifndef REVERIE_BREAKWATER_OCEAN_H
#define REVERIE_BREAKWATER_OCEAN_H
#include "breakwater_environment.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {
// One low-poly ocean extending from the shore to the far atmosphere.
// Gameplay remains Campaign-owned; geometry is shared across stereo eyes.
constexpr int kOceanCrossSections=16;
constexpr int kOceanDepthSections=24;
constexpr float kOceanFarZ=-110.0f;
inline float OceanSurfaceY(float x,float z,float phase) {
    if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(phase))
        return -1.105f;
    const float offshore=std::clamp((-z-2.0f)/12.0f,0.0f,1.0f);
    const float amplitude=.006f+.055f*offshore;
    return -1.105f+amplitude*(
        .65f*std::sin(x*.27f+z*.19f+phase*.42f)+
        .35f*std::sin(x*.45f-z*.12f+phase*.26f));
}
template<class TriangleFn>
void EmitOpenSea(uint8_t location,bool night,float phase,TriangleFn triangle) {
    if(location>=Campaign::kLocations||!std::isfinite(phase))return;
    const EnvironmentColor c=EnvironmentFor(location,night).sea;
    for(int iz=0;iz<kOceanDepthSections;++iz) {
        const float t0=static_cast<float>(iz)/kOceanDepthSections;
        const float t1=static_cast<float>(iz+1)/kOceanDepthSections;
        const float z0=-2.0f+(kOceanFarZ+2.0f)*t0*t0;
        const float z1=-2.0f+(kOceanFarZ+2.0f)*t1*t1;
        const float half0=20.0f+45.0f*t0;
        const float half1=20.0f+45.0f*t1;
        for(int ix=0;ix<kOceanCrossSections;++ix) {
            const float u0=2.0f*ix/kOceanCrossSections-1.0f;
            const float u1=2.0f*(ix+1)/kOceanCrossSections-1.0f;
            const float x00=u0*half0,x10=u1*half0;
            const float x01=u0*half1,x11=u1*half1;
            const float y00=OceanSurfaceY(x00,z0,phase);
            const float y10=OceanSurfaceY(x10,z0,phase);
            const float y01=OceanSurfaceY(x01,z1,phase);
            const float y11=OceanSurfaceY(x11,z1,phase);
            const float fog=.88f+.12f*t0;
            const float facet=((iz+ix*3)%5==0?.91f:1.0f)*fog;
            triangle(x00,y00,z0,x10,y10,z0,x11,y11,z1,
                c.r*facet,c.g*facet,c.b*facet);
            triangle(x00,y00,z0,x11,y11,z1,x01,y01,z1,
                c.r*facet*.95f,c.g*facet*.98f,c.b*facet);
        }
    }
}
}} // namespace reverie::breakwater
#endif
