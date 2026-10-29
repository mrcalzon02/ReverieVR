#ifndef REVERIE_BREAKWATER_EMPLACEMENT_H
#define REVERIE_BREAKWATER_EMPLACEMENT_H

#include "breakwater_campaign.h"
#include <cmath>

namespace reverie { namespace breakwater {
// Low-poly, actually volumetric shore defense for each combat coast.
// All features are outside the central gun corridor / infantry contact lane;
// the geometry is decorative until separately authored into the combat
// collision and targeting model. No fictitious shot-intercepting barriers.
template<typename BoxFn,typename TriangleFn>
inline void EmitBatteryEmplacement(uint8_t location,bool night,
                                   BoxFn box,TriangleFn triangle) {
    if(location>=Campaign::kLocations)return;
    const float tint=night?.43f:1.f;
    const float concrete=location==1?.36f:location==2?.30f:.48f;
    const float stone=concrete*tint;
    const float metal=(location==2?.29f:.36f)*tint;
    // Solid bunker apron, high enough to join the actual beach rear
    // slope, behind the simulated seaward soldier contact line.
    box(0.f,-1.155f,.83f,8.20f,.37f,3.03f,
        stone,stone*1.035f,stone*.94f);
    // Shoreward tapered ramp from height-meshed beach to gun deck.
    triangle(-1.90f,-.94f,-.28f,1.90f,-.94f,-.28f,
              1.83f,-.97f,.24f,stone*.91f,stone*.92f,stone*.88f);
    triangle(-1.90f,-.94f,-.28f,1.83f,-.97f,.24f,
              -1.83f,-.97f,.24f,stone*.88f,stone*.90f,stone*.84f);
    // Recessed firing embrasure with protected flanks. No wall is placed
    // across x=-1.6..1.6; barrel and shallow shots remain unobstructed.
    for(int side : {-1,1}) {
        const float sign=static_cast<float>(side);
        const float x=sign*2.70f;
        box(x,-.69f,-.35f,2.20f,.63f,.64f,
            stone*.98f,stone,stone*.97f);
        // Slanted cheek armor rather than a pair of bare rectangular posts.
        triangle(sign*1.58f,-.92f,-.76f,
                 sign*1.58f,-.14f,-.34f,
                 sign*2.05f,-.91f,-.74f,
                 stone*.81f,stone*.82f,stone*.80f);
        triangle(sign*1.58f,-.14f,-.34f,
                 sign*2.06f,-.20f,-.35f,
                 sign*2.05f,-.91f,-.74f,
                 stone*.88f,stone*.91f,stone*.87f);
        // Sidewall return extends behind the viewer, creating an actual
        // three-dimensional bunker rather than a free-floating play card.
        box(sign*3.72f,-.39f,1.18f,.58f,1.29f,3.1f,
            stone*.91f,stone*.91f,stone*.84f);
        box(sign*3.72f,.305f,1.18f,.71f,.13f,3.28f,
            stone*1.11f,stone*1.10f,stone*1.03f);
        box(sign*3.58f,-.95f,.12f,.24f,.16f,1.2f,
            metal*.80f,metal*.83f,metal*.81f);
        // Layered irregular sandbags around slit, but never in it.
        for(int layer=0;layer<2;++layer)for(int n=0;n<3;++n) {
            const float bx=sign*(1.90f+.46f*n);
            box(bx,-.28f+layer*.17f,-.42f,
                .41f,.16f,.42f,
                .37f*tint,.34f*tint,.27f*tint);
        }
        // Side firing alcoves and locally believable utility storage.
        box(sign*3.04f,-.85f,.76f,.70f,.40f,.83f,
            metal*.75f,metal*.78f,metal*.72f);
        box(sign*3.04f,-.62f,.76f,.55f,.10f,.69f,
            metal*1.08f,metal*1.04f,metal*.98f);
        // Pillboxes are placed outside the 7.72-wide playable beach.
        const float pillX=sign*(5.35f+location*.34f);
        box(pillX,-.65f,1.04f,1.72f,1.0f,2.12f,
            stone*.74f,stone*.77f,stone*.73f);
        box(pillX,-.08f,1.04f,1.93f,.16f,2.34f,
            stone*.95f,stone*.96f,stone*.89f);
        box(pillX,-.48f,-.035f,1.12f,.13f,.13f,
            .11f*tint,.15f*tint,.14f*tint);
        for(int post=0;post<3;++post) {
            const float z=-2.01f+post*.51f;
            // Beach stakes are on the *far* shoulders.
            box(sign*4.90f,-.58f,z,.15f,.77f,.15f,
                metal*.66f,metal*.69f,metal*.62f);
        }
    }
    // Solid rear bunker bulkhead with real door recess and overhead beam.
    for(int side : {-1,1}) {
        box(side*2.37f,-.23f,2.46f,3.6f,1.72f,.38f,
            stone*.84f,stone*.89f,stone*.85f);
    }
    box(0.f,.63f,2.46f,1.28f,.18f,.47f,
        stone*.98f,stone*1.03f,stone*.95f);
    box(0.f,-.29f,2.26f,.98f,1.60f,.08f,
        metal*.60f,metal*.66f,metal*.65f);
    box(.33f,-.34f,2.18f,.07f,.07f,.07f,
        .60f*tint,.59f*tint,.49f*tint);
    // Overhead sight rail and service equipment; no overhead roof crossing
    // the camera, the traversing gun, or the world-space HUD.
    for(int side : {-1,1}) {
        box(side*2.76f,.42f,1.87f,.12f,.12f,.90f,
            metal,metal*1.07f,metal*.89f);
        box(side*2.76f,.25f,1.38f,.14f,.43f,.14f,
            metal*.75f,metal*.79f,metal*.74f);
    }
}
}} // namespace reverie::breakwater
#endif
