#ifndef REVERIE_BREAKWATER_SKY_H
#define REVERIE_BREAKWATER_SKY_H

#include "breakwater_campaign.h"
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

// A complete, low-budget world-space atmospheric panorama. No imported
// imagery, texture, RNG, heap state or animation phase. Sky and cutout layers
// are emitted once per authoritative BuildScene, then reused for both eyes.
// Distances exceed the farthest gameplay sea strip (z=-24.08).
constexpr int kSkySegments = 16;
constexpr int kSkyBands = 7;
constexpr float kSkyRadius = 25.6f;
constexpr float kSkyPi = 3.14159265358979323846f;
constexpr float SkyCelestialAngle(bool night) {
    return night?-.77f:.88f;
}
constexpr float SkyCelestialHeight(bool night) {
    return night?9.1f:9.7f;
}
struct SkyPoint { float x,y,z; };
struct SkyColor { float r,g,b; };

inline SkyPoint SkyAt(float angle,float radius,float across,float y) {
    // Angle zero faces the battle (-Z); positive angles wrap toward +X.
    const float sine=std::sin(angle),cosine=std::cos(angle);
    return {radius*sine+across*cosine,y,
            -radius*cosine+across*sine};
}

template<typename TriangleFn>
void EmitSkyPanorama(uint8_t location,bool night,TriangleFn triangle) {
    if(location>=Campaign::kLocations)return;
    const auto emit=[&](SkyPoint a,SkyPoint b,SkyPoint c,SkyColor color) {
        triangle(a.x,a.y,a.z,b.x,b.y,b.z,c.x,c.y,c.z,
                 color.r,color.g,color.b);
    };
    const float heights[kSkyBands+1]={
        -4.0f,-1.1f,.2f,1.8f,3.8f,7.0f,11.0f,17.0f};
    const SkyColor day[kSkyBands]={
        {.59f,.75f,.79f},{.65f,.82f,.90f},{.49f,.75f,.94f},
        {.37f,.64f,.93f},{.23f,.49f,.83f},{.14f,.34f,.71f},
        {.09f,.23f,.56f}};
    const SkyColor dusk[kSkyBands]={
        {.047f,.093f,.18f},{.037f,.077f,.16f},{.029f,.063f,.15f},
        {.022f,.046f,.13f},{.015f,.032f,.105f},
        {.010f,.022f,.088f},{.006f,.012f,.068f}};
    // Campaign locations have subtly distinct atmospheric casts.
    const float red=location==1?.94f:location==2?.85f:1.0f;
    const float green=location==1?.91f:location==2?.94f:1.0f;
    const float blue=location==1?.87f:location==2?1.09f:1.0f;
    for(int sector=0;sector<kSkySegments;++sector) {
        const float angle0=-kSkyPi+2.0f*kSkyPi*sector/kSkySegments;
        const float angle1=-kSkyPi+
            2.0f*kSkyPi*(sector+1)/kSkySegments;
        for(int band=0;band<kSkyBands;++band) {
            const SkyColor base=night?dusk[band]:day[band];
            const SkyColor color={base.r*red,base.g*green,base.b*blue};
            const SkyPoint a=SkyAt(angle0,kSkyRadius,0,heights[band]);
            const SkyPoint b=SkyAt(angle1,kSkyRadius,0,heights[band]);
            const SkyPoint c=SkyAt(angle1,kSkyRadius,0,heights[band+1]);
            const SkyPoint d=SkyAt(angle0,kSkyRadius,0,heights[band+1]);
            emit(a,b,c,color);emit(a,c,d,color);
        }
    }

    // Seal the zenith: a horizontal-only panorama leaves an empty cone
    // directly overhead when the player looks up in Daydream VR. Extend
    // the uppermost ring to one shared, world-space apex. Its ring color
    // matches the final sky band, so the join has no daytime/night seam.
    // One extra triangle per existing sky sector; no new shader/draw pass.
    const SkyColor top=night?dusk[kSkyBands-1]:day[kSkyBands-1];
    const SkyColor zenith={top.r*red,top.g*green,top.b*blue};
    const SkyPoint apex={0.0f,40.0f,0.0f};
    for(int sector=0;sector<kSkySegments;++sector) {
        const float a=-kSkyPi+2.0f*kSkyPi*sector/kSkySegments;
        const float b=-kSkyPi+2.0f*kSkyPi*(sector+1)/kSkySegments;
        emit(SkyAt(a,kSkyRadius,0,heights[kSkyBands]),
             SkyAt(b,kSkyRadius,0,heights[kSkyBands]),
             apex,zenith);
    }

    // Far, flat cloud silhouettes. Sparse cards curve around the panorama
    // and are not camera-facing per eye: VR parallax is world-consistent.
    const int clouds=night?8:12;
    for(int i=0;i<clouds;++i) {
        const float angle=-kSkyPi+2.0f*kSkyPi*
            (static_cast<float>(i)+.31f*(i%3))/clouds;
        const float y=1.9f+static_cast<float>((i*7)%9)*.48f;
        const float width=2.35f+.32f*static_cast<float>(i%4);
        const float height=.24f+.095f*static_cast<float>(i%3);
        const SkyColor color=night?
            SkyColor{.065f,.078f,.115f}:
            SkyColor{.60f-.026f*(i%3),.66f-.023f*(i%3),
                     .67f-.018f*(i%3)};
        const SkyColor shade=night?
            SkyColor{.046f,.056f,.090f}:
            SkyColor{.47f,.56f,.59f};
        constexpr float radius=25.20f;
        emit(SkyAt(angle,radius,-width*.50f,y),
             SkyAt(angle,radius,-width*.24f,y+height),
             SkyAt(angle,radius,width*.12f,y+height*.69f),color);
        emit(SkyAt(angle,radius,-width*.50f,y),
             SkyAt(angle,radius,width*.12f,y+height*.69f),
             SkyAt(angle,radius,width*.50f,y),color);
        emit(SkyAt(angle,radius,-width*.36f,y-.14f),
             SkyAt(angle,radius,width*.50f,y),
             SkyAt(angle,radius,width*.34f,y-.14f),shade);
    }

    // Flat celestial disc viewed against the upper sky, behind clouds.
    // No per-eye rotation and no extra billboard shader path.
    const float bodyAngle=SkyCelestialAngle(night);
    const float bodyHeight=SkyCelestialHeight(night);
    const float bodySize=night?.57f:.82f;
    const SkyColor bodyColor=night?
        SkyColor{.68f,.75f,.81f}:SkyColor{.96f,.78f,.47f};
    const SkyPoint middle=SkyAt(bodyAngle,25.12f,0,bodyHeight);
    for(int n=0;n<10;++n) {
        const float a=2*kSkyPi*n/10.0f;
        const float b=2*kSkyPi*(n+1)/10.0f;
        emit(middle,
             SkyAt(bodyAngle,25.12f,bodySize*std::cos(a),
                   bodyHeight+bodySize*std::sin(a)),
             SkyAt(bodyAngle,25.12f,bodySize*std::cos(b),
                   bodyHeight+bodySize*std::sin(b)),bodyColor);
    }

    if(night) {
        // Fixed micro-diamond stars. Tiny enough to remain background,
        // with stable placement across eyes, pauses and saved campaigns.
        for(int i=0;i<24;++i) {
            const float angle=-kSkyPi+2.0f*kSkyPi*((i*37)%97)/97.0f;
            const float y=3.5f+static_cast<float>((i*23)%47)*.20f;
            const float w=(i%5==0)?.080f:.047f;
            const SkyColor star={(i%3==0)?.55f:.40f,
                                 (i%3==0)?.63f:.46f,.66f};
            emit(SkyAt(angle,25.04f,-w,y),
                 SkyAt(angle,25.04f,0,y+w*1.7f),
                 SkyAt(angle,25.04f,w,y),star);
            emit(SkyAt(angle,25.04f,-w,y),
                 SkyAt(angle,25.04f,w,y),
                 SkyAt(angle,25.04f,0,y-w*1.7f),star);
        }
    }

    // Industrial Iron Quay has three distant plumes; Blackcap has one.
    // These are scenery, never targets or obstacle collision geometry.
    const int plumes=location==1?3:location==2?1:0;
    for(int i=0;i<plumes;++i) {
        const float angle=-.28f+i*.23f;
        const float y=-.35f;
        const float height=1.35f+(i%2)*.52f;
        const float radius=25.0f;
        const SkyColor smoke=night?
            SkyColor{.039f,.051f,.070f}:
            SkyColor{.25f,.29f,.29f};
        emit(SkyAt(angle,radius,-.20f,y),
             SkyAt(angle,radius,.23f,y),
             SkyAt(angle,radius,.17f,y+height),smoke);
        emit(SkyAt(angle,radius,-.20f,y),
             SkyAt(angle,radius,.17f,y+height),
             SkyAt(angle,radius,-.11f,y+height*.72f),smoke);
    }
}

}} // namespace reverie::breakwater
#endif
