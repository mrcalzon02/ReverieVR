#ifndef REVERIE_NATIVE_AVATAR_WARDROBE_H
#define REVERIE_NATIVE_AVATAR_WARDROBE_H

#include "reverie_native_avatar_geometry.h"
#include <string.h>

/*
 * Low-cost three-dimensional wardrobe fallback for native GLES2 games.
 * Authored visual proxies: NOT a replacement for inspected/rigged GLB art.
 * Twelve ellipsoid slots: garment outer shell/skirt, two fit details,
 * hairstyle 2, hat 2, accessory 2, eyes 2. A single packed vertex stream
 * is generated only when the selected identity or GL context changes.
 * Geometry is in the same floor-relative coordinates as AvatarFigureV1.
 * No gameplay collider, save data, or physics values depend on these meshes.
 */
#define REVERIE_NATIVE_WARDROBE_SLOTS_V1 12u
#define REVERIE_NATIVE_WARDROBE_VERTICES_V1 \
    (REVERIE_NATIVE_WARDROBE_SLOTS_V1*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1)
#define REVERIE_NATIVE_WARDROBE_FLOATS_V1 (REVERIE_NATIVE_WARDROBE_VERTICES_V1*3u)

#define REVERIE_NATIVE_WARDROBE_GARMENT_FIRST_V1 0u
#define REVERIE_NATIVE_WARDROBE_DETAIL_FIRST_V1 2u
#define REVERIE_NATIVE_WARDROBE_HAIR_FIRST_V1 4u
#define REVERIE_NATIVE_WARDROBE_HAT_FIRST_V1 6u
#define REVERIE_NATIVE_WARDROBE_ACCESSORY_FIRST_V1 8u
#define REVERIE_NATIVE_WARDROBE_EYES_FIRST_V1 10u

static inline void reverie_avatar_wearable_slot(
    ReverieNativeBodyEllipsoidV1 *parts,unsigned slot,
    float x,float y,float z,float rx,float ry,float rz
) {
    parts[slot].center_x=x;parts[slot].center_y=y;parts[slot].center_z=z;
    parts[slot].radius_x=rx;parts[slot].radius_y=ry;parts[slot].radius_z=rz;
}

/*
 * Body-relative shapes are intentionally very simple for S9-era VR.
 * Outfit family (0..23) changes upper/lower silhouette and detail positions.
 * Hair, hats and accessories are independent fields; 'None' means no mesh.
 * Hidden slots are zeroed; bitmask tells consumers which draw calls to skip.
 */
static inline int ReverieNativeWriteAvatarWardrobeV1(
    const ReverieNativeAvatarV1 *avatar,
    float center_x,float feet_y,float center_z,
    float *out,size_t capacity,uint32_t *out_visible
) {
    if(out_visible!=NULL)*out_visible=0u;
    if(!ReverieNativeAvatarValidV1(avatar)||out==NULL||out_visible==NULL
        ||capacity<REVERIE_NATIVE_WARDROBE_FLOATS_V1
        ||!(fabsf(center_x)<10000.0f)
        ||!(fabsf(feet_y)<10000.0f)
        ||!(fabsf(center_z)<10000.0f))
        return 0;
    ReverieNativeBodyEllipsoidV1 parts[REVERIE_NATIVE_WARDROBE_SLOTS_V1]={0};
    const uint32_t outfit=avatar->fields[REVERIE_NATIVE_AVATAR_OUTFIT];
    const uint32_t hairstyle=avatar->fields[REVERIE_NATIVE_AVATAR_HAIR];
    const uint32_t hat=avatar->fields[REVERIE_NATIVE_AVATAR_HAT];
    const uint32_t extra=avatar->fields[REVERIE_NATIVE_AVATAR_ACCESSORY];
    const uint32_t eyes=avatar->fields[REVERIE_NATIVE_AVATAR_EYES];
    const float height=(float)avatar->fields[REVERIE_NATIVE_AVATAR_HEIGHT];
    const float sh=(float)avatar->fields[REVERIE_NATIVE_AVATAR_SHOULDERS];
    const float wa=(float)avatar->fields[REVERIE_NATIVE_AVATAR_WAIST];
    const float hip=(float)avatar->fields[REVERIE_NATIVE_AVATAR_HIPS];
    const float muscle=(float)avatar->fields[REVERIE_NATIVE_AVATAR_MUSCLE];
    const float leg_height=.58f+.045f*height;
    const float shoulder=.19f+.032f*sh+.008f*muscle;
    const float waist=.14f+.021f*wa;
    const float hips=.15f+.026f*hip;
    const float torso=feet_y+leg_height+.30f;
    const float head=feet_y+leg_height+.82f;
    const float forward=center_z+.10f;
    uint32_t visible=0u;
    // Eight actual garment cuts, repeated with three trim constructions.
    switch(outfit%8u) {
        case 0u: // close-fitting knit/casual
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*1.02f,.305f,waist+.070f);
            reverie_avatar_wearable_slot(parts,1u,center_x,feet_y+leg_height-.08f,
                center_z,hips*1.02f,.115f,hips*.78f);break;
        case 1u: // fitted jacket and coat tails
            reverie_avatar_wearable_slot(parts,0u,center_x,torso+.01f,center_z,
                shoulder*1.14f,.34f,waist+.080f);
            reverie_avatar_wearable_slot(parts,1u,center_x,torso-.45f,
                center_z,hips*1.05f,.29f,hips*.80f);break;
        case 2u: // long robe with hanging hem
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*1.12f,.355f,waist+.08f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height*.52f,center_z,hips*1.25f,leg_height*.50f,
                hips*.92f);break;
        case 3u: // flared gown or kilt, usable by any presentation
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*.96f,.33f,waist+.075f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height*.46f,center_z,hips*1.55f,leg_height*.45f,
                hips*1.12f);break;
        case 4u: // layered plate or power armor
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*1.18f,.36f,waist+.13f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height-.06f,center_z,hips*1.22f,.18f,hips*.87f);break;
        case 5u: // workshop coveralls
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*1.03f,.34f,waist+.09f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height-.13f,center_z,hips*1.07f,.18f,hips*.78f);break;
        case 6u: // formal/courtly jacket
            reverie_avatar_wearable_slot(parts,0u,center_x,torso+.04f,center_z,
                shoulder*1.09f,.31f,waist+.067f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height-.22f,center_z,hips*1.08f,.27f,
                hips*.81f);break;
        default: // long storm coat with dramatic lower silhouette
            reverie_avatar_wearable_slot(parts,0u,center_x,torso,center_z,
                shoulder*1.13f,.34f,waist+.10f);
            reverie_avatar_wearable_slot(parts,1u,center_x,
                feet_y+leg_height*.54f,center_z,hips*1.30f,
                leg_height*.46f,hips*1.01f);break;
    }
    visible|=3u;
    // Three bands of independent trim language, not identical color swaps.
    switch(outfit/8u) {
        case 0u: // chest insignia and buckle
            reverie_avatar_wearable_slot(parts,2u,center_x,torso+.13f,
                forward+.09f,.075f,.045f,.036f);
            reverie_avatar_wearable_slot(parts,3u,center_x,torso-.23f,
                forward+.08f,.11f,.045f,.04f);break;
        case 1u: // broad epaulettes / shoulder gear
            reverie_avatar_wearable_slot(parts,2u,center_x-shoulder,
                torso+.27f,center_z,.12f,.085f,.14f);
            reverie_avatar_wearable_slot(parts,3u,center_x+shoulder,
                torso+.27f,center_z,.12f,.085f,.14f);break;
        default: // diagonal cross-body sash and brooch
            reverie_avatar_wearable_slot(parts,2u,center_x-shoulder*.43f,
                torso+.12f,forward+.06f,.075f,.24f,.040f);
            reverie_avatar_wearable_slot(parts,3u,center_x+waist*.25f,
                torso-.23f,forward+.06f,.09f,.055f,.04f);break;
    }
    visible|=(3u<<2u);
    // Hair builds from an independent twelve-look family.
    if(hairstyle!=0u) {
        float mass=.16f+.016f*(float)(hairstyle%4u);
        float length=.035f+.10f*(float)((hairstyle-1u)/4u);
        reverie_avatar_wearable_slot(parts,4u,center_x,head+.16f,
            center_z,mass,.07f+.035f*(float)(hairstyle%3u),mass);
        reverie_avatar_wearable_slot(parts,5u,
            center_x+((hairstyle%2u)?-.14f:.14f),
            head-.05f-length*.34f,center_z,
            .065f+.01f*(float)(hairstyle%3u),
            length+.025f,.080f);
        visible|=(3u<<4u);
    }
    // Hat families: brim/visor, tall, ornamental and helmets.
    if(hat!=0u) {
        const float crown_height=.045f+.035f*(float)(hat%5u);
        const float brim=.19f+.025f*(float)(hat%4u);
        reverie_avatar_wearable_slot(parts,6u,center_x,
            head+.23f+crown_height*.38f,center_z,
            .14f+.012f*(float)(hat%3u),crown_height,.15f);
        reverie_avatar_wearable_slot(parts,7u,center_x,head+.20f,
            center_z,brim,.025f+.01f*(float)(hat%3u),brim*.9f);
        visible|=(3u<<6u);
    }
    // Sixteen accessories including None. Shape family changes position.
    if(extra!=0u) {
        switch(extra%4u) {
            case 0u: // wing/backpack shoulder bulge
                reverie_avatar_wearable_slot(parts,8u,center_x-shoulder*1.5f,
                    torso-.03f,center_z-.18f,.14f,.30f,.10f);
                reverie_avatar_wearable_slot(parts,9u,center_x+shoulder*1.5f,
                    torso-.03f,center_z-.18f,.14f,.30f,.10f);break;
            case 1u: // belt + hanging pouch
                reverie_avatar_wearable_slot(parts,8u,center_x,
                    torso-.25f,center_z,waist+.09f,.055f,waist+.09f);
                reverie_avatar_wearable_slot(parts,9u,center_x+waist,
                    torso-.38f,center_z+.10f,.11f,.16f,.10f);break;
            case 2u: // collar + ornament
                reverie_avatar_wearable_slot(parts,8u,center_x,torso+.26f,
                    center_z,.13f,.050f,.13f);
                reverie_avatar_wearable_slot(parts,9u,center_x,torso+.14f,
                    forward+.10f,.075f+.01f*(float)(extra/4u),
                    .11f,.07f);break;
            default: // cape/back panel
                reverie_avatar_wearable_slot(parts,8u,center_x-shoulder*.68f,
                    torso-.26f,center_z-.17f,.13f,.46f,.080f);
                reverie_avatar_wearable_slot(parts,9u,center_x+shoulder*.68f,
                    torso-.26f,center_z-.17f,.13f,.46f,.080f);break;
        }
        visible|=(3u<<8u);
    }
    // Eyes as actual 3D details, forward of the head's surface.
    const float eyeY=head+.045f+(eyes==1u?-.025f:0.f);
    const float eyeSize=eyes==1u?.012f:eyes==3u?.035f:.024f;
    reverie_avatar_wearable_slot(parts,10u,center_x-.075f,eyeY,
        center_z+.154f,eyeSize,eyeSize*.73f,.014f);
    reverie_avatar_wearable_slot(parts,11u,center_x+.075f,eyeY,
        center_z+.154f,eyeSize,eyeSize*.73f,.014f);
    visible|=(3u<<10u);
    const size_t stride=REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1*3u;
    for(unsigned i=0u;i<REVERIE_NATIVE_WARDROBE_SLOTS_V1;i++) {
        float *section=out+(size_t)i*stride;
        if((visible&(1u<<i))==0u) {
            memset(section,0,stride*sizeof(float));
        } else if(!ReverieNativeWriteEllipsoidTrianglesV1(
            section,capacity-(size_t)i*stride,&parts[i])) {
            return 0;
        }
    }
    *out_visible=visible;
    return 1;
}

/* Each material pair maps onto two consecutive, fixed-size meshes.
 * Color is host-readable and intentionally cosmetic, never game physics. */
static inline void ReverieNativeAvatarWardrobeTintV1(
    const ReverieNativeAvatarV1 *avatar,unsigned group,
    float *red,float *green,float *blue
) {
    if(!ReverieNativeAvatarValidV1(avatar)) {
        ReverieNativeAvatarColorV1(0u,red,green,blue);
        return;
    }
    const uint32_t c=avatar->fields[REVERIE_NATIVE_AVATAR_FABRIC];
    const uint32_t accent=avatar->fields[REVERIE_NATIVE_AVATAR_ACCENT_COLOR];
    if(group==0u)ReverieNativeAvatarColorV1(c,red,green,blue);
    else if(group==1u||group==4u)
        ReverieNativeAvatarColorV1(accent,red,green,blue);
    else if(group==2u) {
        float r=0.f,g=0.f,b=0.f;
        ReverieNativeAvatarColorV1(accent,&r,&g,&b);
        if(red)*red=r*.42f;if(green)*green=g*.42f;if(blue)*blue=b*.42f;
    } else if(group==3u) {
        if(red)*red=.89f;if(green)*green=.72f;if(blue)*blue=.31f;
    } else { // eyes: cyan cyber style, otherwise darker irises
        const int cyber=avatar->fields[REVERIE_NATIVE_AVATAR_EYES]==2u;
        if(red)*red=cyber?.04f:.17f;
        if(green)*green=cyber?.88f:.20f;
        if(blue)*blue=cyber?1.0f:.26f;
    }
}

#endif
