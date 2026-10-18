#ifndef REVERIE_BREAKWATER_TERRAIN_H
#define REVERIE_BREAKWATER_TERRAIN_H

#include "breakwater_campaign.h"
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace reverie { namespace breakwater {

// Each campaign location has its own deterministic, low-budget coast.
// This is game-private render geometry: enemies, shell collisions and
// targeting remain entirely governed by Campaign. No texture or RNG.
struct CoastPalette {
    float sand_r, sand_g, sand_b;
    float detail_r, detail_g, detail_b;
};

inline CoastPalette CoastColors(uint8_t location, bool night) {
    const CoastPalette daytime[] = {
        {.83f, .75f, .54f, .91f, .85f, .63f}, // Shingle Bay
        {.51f, .54f, .54f, .65f, .63f, .56f}, // Iron Quay
        {.35f, .34f, .40f, .50f, .45f, .43f}  // Blackcap Island
    };
    if (location >= Campaign::kLocations) return {0,0,0,0,0,0};
    CoastPalette c = daytime[location];
    if (night) {
        c.sand_r *= .32f; c.sand_g *= .43f; c.sand_b *= .69f;
        c.detail_r *= .38f; c.detail_g *= .48f; c.detail_b *= .74f;
    }
    return c;
}

template <typename BoxFn, typename TriangleFn>
void EmitCoastalTerrain(uint8_t location, bool night,
                        BoxFn box, TriangleFn triangle) {
    if (location >= Campaign::kLocations) return;
    const CoastPalette c = CoastColors(location, night);
    constexpr float y = -1.037f; // Above sea, below soldiers' feet.
    // Six shallow beach strips, with stable slight tonal variation.
    // Water ends at z=-2.20, the authoritative transport landing line.
    for (int strip=0; strip<6; ++strip) {
        const float nearZ = -2.20f + strip*.20f;
        const float farZ = nearZ + .20f;
        const float shade = strip%2==0 ? .97f : 1.03f;
        triangle(-3.86f,y,nearZ, 3.86f,y,nearZ,
                 3.86f,y,farZ,
                 c.sand_r*shade,c.sand_g*shade,c.sand_b*shade);
        triangle(-3.86f,y,nearZ, 3.86f,y,farZ,
                 -3.86f,y,farZ,
                 c.sand_r*shade,c.sand_g*shade,c.sand_b*shade);
    }
    // A segmented foamy surf edge visually identifies where boats unload.
    // The foam remains static so no separate eye can animate it differently.
    for (int n=0; n<8; ++n) {
        const float x = -3.80f + n*.95f;
        triangle(x,-1.018f,-2.213f, x+.80f,-1.018f,-2.213f,
                 x+.51f,-1.018f,-2.167f,
                 night?.30f:.72f,night?.41f:.87f,night?.54f:.91f);
    }

    // Each beach has a different profile beyond its playable 7.72-wide
    // center lane: rolling sandy dune, square quay, or fractured scree.
    // These shoulders are scenery only; no new collision or troop blockers.
    for(int side : {-1,1}) {
        const float sign=static_cast<float>(side);
        const float shoulder=location==0?4.65f:location==1?4.15f:5.10f;
        const float crest=location==0?.32f:location==1?.14f:.54f;
        const float color=location==0?1.05f:location==1?.77f:.63f;
        triangle(sign*3.85f,y,-2.15f,
                 sign*shoulder,y+crest,-1.48f,
                 sign*3.85f,y,-.95f,
                 c.sand_r*color,c.sand_g*color,c.sand_b*color);
        triangle(sign*shoulder,y+crest,-1.48f,
                 sign*(shoulder+.70f),y+crest*.76f,-.72f,
                 sign*3.85f,y,-.95f,
                 c.detail_r*.91f,c.detail_g*.91f,c.detail_b*.91f);
    }
    // Continue the shoreline beyond the playable lanes all the way toward
    // the circular backdrop. A short center-only beach left two conspicuous
    // gaps when looking left and right in the headset.
    for(int side : {-1,1}) {
        const float sign=static_cast<float>(side);
        if(location==1) {
            // Iron Quay: substantial harbor piers, edge walls, bollards and
            // a distant gantry. These remain decorative, never hitboxes.
            box(sign*12.0f,-1.11f,-.46f,16.0f,.32f,3.65f,
                c.sand_r*.73f,c.sand_g*.75f,c.sand_b*.78f);
            box(sign*12.0f,-.79f,-2.26f,16.0f,.65f,.32f,
                c.detail_r*.72f,c.detail_g*.73f,c.detail_b*.74f);
            for(int index=0;index<7;index++) {
                const float x=sign*(5.05f+index*2.3f);
                box(x,-.38f,-2.22f,.32f,.35f,.36f,
                    .30f,.30f,.29f);
                box(x,-.16f,-2.22f,.46f,.14f,.48f,
                    .22f,.24f,.24f);
            }
            const float towerX=sign*13.6f;
            box(towerX,.68f,.22f,.24f,3.35f,.26f,
                c.detail_r*.75f,c.detail_g*.75f,c.detail_b*.77f);
            box(towerX-sign*1.3f,2.30f,.22f,2.70f,.21f,.28f,
                c.detail_r*.83f,c.detail_g*.84f,c.detail_b*.85f);
            box(towerX-sign*2.50f,1.76f,.22f,.055f,1.03f,.055f,
                .28f,.27f,.25f);
        } else {
            // Shingle Bay and Blackcap: wide natural side slopes instead of
            // abrupt squared-off terrain beside the battery position.
            const float shade=location==0?.84f:.65f;
            triangle(sign*3.80f,y,-2.15f,
                     sign*20.0f,y+.34f,-2.30f,
                     sign*20.0f,y+.18f,1.45f,
                     c.sand_r*shade,c.sand_g*shade,c.sand_b*shade);
            triangle(sign*3.80f,y,-2.15f,
                     sign*20.0f,y+.18f,1.45f,
                     sign*3.80f,y,1.45f,
                     c.detail_r*shade,c.detail_g*shade,c.detail_b*shade);
            for(int n=0;n<9;n++) {
                const float x=sign*(5.2f+n*1.60f);
                const float height=location==0?.24f+.07f*(n%3):
                    .60f+.12f*(n%4);
                box(x,y+height*.42f,.42f+(n%3)*.37f,
                    .72f,height,.75f,
                    c.detail_r*.76f,c.detail_g*.78f,c.detail_b*.80f);
            }
        }
    }
    if (location==0) {
        // Shingle Bay: small deterministic stone chips and sparse driftwood
        // against an unobstructed troop path. Nothing requires collision.
        for (int n=0; n<18; ++n) {
            const float x = -3.35f + static_cast<float>(n%9)*.79f;
            const float z = -2.06f + static_cast<float>(n/9)*.64f
                           + static_cast<float>(n%3)*.07f;
            triangle(x-.075f,y+.007f,z-.035f,
                     x+.085f,y+.007f,z-.010f,
                     x+.006f,y+.012f,z+.060f,
                     c.detail_r,c.detail_g,c.detail_b);
        }
        for (int side : {-1,1}) {
            box(static_cast<float>(side)*3.48f,-.975f,-1.52f,
                .13f,.12f,.62f,.37f,.29f,.19f);
        }
    } else if (location==1) {
        // Iron Quay: concrete sea wall with rusty mooring posts.
        // Posts are confined to the outside edges of the infantry lane.
        for (int side : {-1,1}) {
            const float x = static_cast<float>(side)*3.53f;
            box(x,-.96f,-1.58f,.35f,.18f,1.14f,
                c.detail_r*.78f,c.detail_g*.81f,c.detail_b*.84f);
            for(int n=0;n<3;n++) {
                box(x,-.80f,-2.02f+n*.39f,.12f,.35f,.13f,
                    .39f,.26f,.18f);
            }
        }
        // Separate small masonry seams retain a PS1-style hard edge.
        for(int n=0;n<5;n++) {
            const float z=-2.02f+n*.22f;
            triangle(-3.84f,y+.008f,z,3.84f,y+.008f,z,
                     3.84f,y+.008f,z+.018f,
                     .19f,.22f,.23f);
        }
    } else {
        // Blackcap Island: basalt outcrops frame the beach. Keep the
        // center open for individually targetable moving infantry.
        for(int side : {-1,1}) {
            const float sign=static_cast<float>(side);
            for(int n=0;n<4;n++) {
                const float x=sign*(3.18f+.17f*(n%2));
                const float z=-2.01f+n*.28f;
                const float h=.23f+.09f*(n%3);
                box(x,-1.02f+h*.5f,z,.30f,h,.27f,
                    c.detail_r*.74f,c.detail_g*.76f,c.detail_b*.82f);
                triangle(x-sign*.21f,-.99f,z-.14f,
                         x,-.98f+h,z,
                         x+sign*.22f,-.99f,z+.16f,
                         c.detail_r,c.detail_g,c.detail_b);
            }
        }
    }
}

}} // namespace reverie::breakwater
#endif
