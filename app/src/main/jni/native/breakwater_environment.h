#ifndef REVERIE_BREAKWATER_ENVIRONMENT_H
#define REVERIE_BREAKWATER_ENVIRONMENT_H

#include "breakwater_sky.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

struct EnvironmentColor { float r,g,b; };
struct EnvironmentProfile {
    EnvironmentColor earth, hill, mountain, distant, sea;
    float rise, peaks;
};

// The three locations have different geology, beach approaches and rear
// silhouettes. Day/night changes actual vertex lighting as well as the sky.
inline EnvironmentColor Moonlight(EnvironmentColor c) {
    return {c.r*.34f,c.g*.49f,c.b*.72f};
}
inline EnvironmentProfile EnvironmentFor(uint8_t location,bool night) {
    const EnvironmentProfile day[Campaign::kLocations]={
        // Shingle Bay: bright grass, soft rolling coastal uplands.
        {{.40f,.59f,.34f},{.31f,.53f,.36f},{.43f,.57f,.54f},
         {.53f,.66f,.69f},{.09f,.42f,.63f},2.6f,4.8f},
        // Iron Quay: ochre embankments, industrial slate ridges.
        {{.48f,.45f,.37f},{.40f,.41f,.40f},{.40f,.46f,.49f},
         {.59f,.61f,.60f},{.11f,.33f,.48f},2.0f,5.6f},
        // Blackcap Island: charcoal basalt, steep volcanic mountains.
        {{.30f,.31f,.34f},{.27f,.30f,.38f},{.35f,.35f,.45f},
         {.49f,.48f,.56f},{.10f,.28f,.47f},3.1f,7.3f}
    };
    if(location>=Campaign::kLocations)return {};
    EnvironmentProfile p=day[location];
    if(night) {
        p.earth=Moonlight(p.earth);
        p.hill=Moonlight(p.hill);
        p.mountain=Moonlight(p.mountain);
        p.distant=Moonlight(p.distant);
        p.sea=Moonlight(p.sea);
    }
    return p;
}

// Input colors remain authorial albedos; this one cheap transform is applied
// to the combat-world vertex batch, never to UI, text or fire effects.
inline EnvironmentColor IlluminateWorld(EnvironmentColor c,bool night) {
    if(night) return {c.r*.42f,c.g*.55f,c.b*.84f};
    return {std::min(1.0f,c.r*1.10f),
            std::min(1.0f,c.g*1.10f),
            std::min(1.0f,c.b*1.05f)};
}

constexpr int kLandscapeSectors=48;
constexpr float kHillRadius=17.0f;
constexpr float kMountainRadius=22.0f;

inline float RidgeHeight(uint8_t location,int sector,bool farRing) {
    // Periodic index makes the closing seam identical to sector zero.
    const int n=(sector%kLandscapeSectors+kLandscapeSectors)%kLandscapeSectors;
    const float angle=-kSkyPi+2.0f*kSkyPi*n/kLandscapeSectors;
    const float rear=.5f-.5f*std::cos(angle); // clear sea in front
    const float ripple=static_cast<float>((n*7+location*11)%9)*.075f;
    const EnvironmentProfile profile=EnvironmentFor(location,false);
    const float height=farRing?profile.peaks:profile.rise;
    const float serration=farRing?(location==2?1.16f:.66f):.25f;
    return -1.18f+rear*(height+ripple+serration*std::abs(std::sin(angle*8.0f)));
}

template<typename TriangleFn>
void EmitLandscapeRing(uint8_t location,bool night,bool farRing,
                       TriangleFn triangle) {
    if(location>=Campaign::kLocations)return;
    const EnvironmentProfile p=EnvironmentFor(location,night);
    const EnvironmentColor c=farRing?p.distant:p.hill;
    const float radius=farRing?kMountainRadius:kHillRadius;
    for(int n=0;n<kLandscapeSectors;++n) {
        const float a=-kSkyPi+2.0f*kSkyPi*n/kLandscapeSectors;
        const float b=-kSkyPi+2.0f*kSkyPi*(n+1)/kLandscapeSectors;
        const SkyPoint lowA=SkyAt(a,radius,0,-3.0f);
        const SkyPoint lowB=SkyAt(b,radius,0,-3.0f);
        const SkyPoint topA=SkyAt(a,radius,0,RidgeHeight(location,n,farRing));
        const SkyPoint topB=SkyAt(b,radius,0,RidgeHeight(location,n+1,farRing));
        const float light=n%3==0?1.04f:n%3==1?.94f:1.0f;
        triangle(lowA.x,lowA.y,lowA.z,lowB.x,lowB.y,lowB.z,
                 topB.x,topB.y,topB.z,c.r*light,c.g*light,c.b*light);
        triangle(lowA.x,lowA.y,lowA.z,topB.x,topB.y,topB.z,
                 topA.x,topA.y,topA.z,c.r*light,c.g*light,c.b*light);
    }
}

// Exact height of the faceted rear ground mesh at an x/z point. The
// two-triangle strip diagonal changes interpolation at v==u, so merely
// fitting a polynomial to z makes props visibly float or sink.
inline float RearGroundHeight(const EnvironmentProfile& p,float x,float z) {
    constexpr float nearZ=-.50f, step=1.45f, width=9.4f;
    const float along=std::max(0.0f,std::min(12.0f,(z-nearZ)/step));
    const int strip=std::min(11,static_cast<int>(along));
    const float v=along-static_cast<float>(strip);
    const float u=std::min(1.0f,std::abs(x)/width);
    const float t0=strip/12.0f,t1=(strip+1)/12.0f;
    const float y0=-1.12f+p.rise*t0*t0*.80f;
    const float y1=-1.12f+p.rise*t1*t1*.80f;
    const float flank0=y0+.38f*t0,flank1=y1+.38f*t1;
    if(v<=u) return (1.0f-u)*y0+(u-v)*flank0+v*flank1;
    return (1.0f-v)*y0+u*flank1+(v-u)*y1;
}

template<typename BoxFn,typename TriangleFn>
void EmitSurroundingLandscape(uint8_t location,bool night,
                              BoxFn box,TriangleFn triangle) {
    if(location>=Campaign::kLocations)return;
    const EnvironmentProfile p=EnvironmentFor(location,night);
    // Fully joined rings provide cliffs and mountain silhouettes at every
    // yaw angle. Low forward ridges avoid hiding incoming ships and aircraft.
    EmitLandscapeRing(location,night,true,triangle);
    EmitLandscapeRing(location,night,false,triangle);
    // A solid walk-back ground surface, behind the player's battery. Its
    // nearest edge stays BELOW the existing gun cradle and menu panels.
    for(int strip=0;strip<12;++strip) {
        const float nearZ=-.50f+strip*1.45f,farZ=nearZ+1.45f;
        const float t0=strip/12.0f,t1=(strip+1)/12.0f;
        const float y0=-1.12f+p.rise*t0*t0*.80f;
        const float y1=-1.12f+p.rise*t1*t1*.80f;
        const float flank0=y0+.38f*t0,flank1=y1+.38f*t1;
        const EnvironmentColor c=strip%2?p.earth:p.hill;
        for(int side=-1;side<=1;side+=2) {
            const float outer=static_cast<float>(side)*9.4f;
            triangle(0,y0,nearZ,outer,flank0,nearZ,
                     outer,flank1,farZ,c.r,c.g,c.b);
            triangle(0,y0,nearZ,outer,flank1,farZ,
                     0,y1,farZ,c.r,c.g,c.b);
        }
    }
    if(location==0) {
        // Soft green Shingle Bay: sparse pines and a rear service track.
        for(int n=0;n<10;++n) {
            const float x=(n%2==0?-1.0f:1.0f)*(3.4f+(n%3)*1.4f);
            const float z=4.4f+(n/2)*1.65f;
            const float h=.85f+.24f*(n%3);
            const float ground=RearGroundHeight(p,x,z);
            constexpr float trunk=.50f;
            triangle(x-.40f,ground+trunk,z,
                     x+.40f,ground+trunk,z,
                     x,ground+trunk+h,z,
                     p.hill.r*.68f,p.hill.g*.82f,p.hill.b*.69f);
            box(x,ground+trunk*.50f,z,.10f,trunk,.10f,
                night?.10f:.29f,night?.14f:.25f,night?.18f:.17f);
        }
        for(int n=0;n<6;++n) {
            const float z=1.4f+n*1.75f;
            box(0,RearGroundHeight(p,0,z)+.015f,z,
                .80f,.025f,1.55f,
                night?.18f:.52f,night?.23f:.47f,night?.27f:.33f);
        }
    } else if(location==1) {
        // Iron Quay: recognizable fortified concrete and supply sheds.
        for(int side=-1;side<=1;side+=2) {
            const float x=static_cast<float>(side)*5.3f;
            const float base=RearGroundHeight(p,x,7.2f);
            box(x,base+.725f,7.2f,2.3f,1.45f,3.0f,
                p.mountain.r,p.mountain.g,p.mountain.b);
            box(x,base+1.50f,5.65f,2.65f,.19f,.32f,
                p.distant.r,p.distant.g,p.distant.b);
            box(x,base+1.02f,6.0f,.85f,.33f,.10f,
                night?.07f:.15f,night?.12f:.20f,night?.21f:.25f);
            box(x,RearGroundHeight(p,x,8.2f)+.60f,8.2f,
                .42f,1.2f,.42f,
                night?.12f:.37f,night?.16f:.32f,night?.21f:.27f);
        }
    } else {
        // Blackcap Island: tall, asymmetric basalt fingers behind the fort.
        for(int n=0;n<9;++n) {
            const float x=(n-4)*1.65f;
            const float z=7.0f+(n%3)*2.0f;
            const float h=1.9f+(n*5%7)*.51f;
            const float y=RearGroundHeight(p,x,z)+.02f;
            triangle(x-.58f,y,z-.40f,x+.48f,y,z-.30f,
                     x+.10f,y+h,z+.12f,p.mountain.r,p.mountain.g,p.mountain.b);
            triangle(x+.48f,y,z-.30f,x+.37f,y,z+.53f,
                     x+.10f,y+h,z+.12f,p.hill.r,p.hill.g,p.hill.b);
        }
    }
}

}} // namespace reverie::breakwater
#endif
