#ifndef REVERIE_NATIVE_AVATAR_GEOMETRY_H
#define REVERIE_NATIVE_AVATAR_GEOMETRY_H

#include "reverie_native_sdk.h"
#include <math.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Authored in the shared SDK; no downloaded model, assets or per-frame
 * allocations. Every ellipsoid is a sphere stretched on three axes.
 * Collision remains the invariant ReverieNativeAvatarCollisionV1;
 * body shape below is appearance ONLY.
 */
#define REVERIE_NATIVE_AVATAR_RINGS_V1 8u
#define REVERIE_NATIVE_AVATAR_SLICES_V1 12u
#define REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1 (8u*12u*6u)
#define REVERIE_NATIVE_AVATAR_FIGURE_PARTS_V1 7u
#define REVERIE_NATIVE_AVATAR_FIGURE_VERTICES_V1 \
    (REVERIE_NATIVE_AVATAR_FIGURE_PARTS_V1*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1)
#define REVERIE_NATIVE_AVATAR_FIGURE_FLOATS_V1 \
    (REVERIE_NATIVE_AVATAR_FIGURE_VERTICES_V1*3u)

/** Exact 3D procedural stretched sphere, triangles as XYZ float triples. */
static inline int ReverieNativeWriteEllipsoidTrianglesV1(
    float *out, size_t floats_capacity,
    const ReverieNativeBodyEllipsoidV1 *e
) {
    if(out==NULL||e==NULL
        ||floats_capacity<REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1*3u
        ||!(e->radius_x>0.0f && e->radius_y>0.0f
            && e->radius_z>0.0f)
        ||!(fabsf(e->center_x)<=FLT_MAX)
        ||!(fabsf(e->center_y)<=FLT_MAX)
        ||!(fabsf(e->center_z)<=FLT_MAX)
        ||!(fabsf(e->radius_x)<=FLT_MAX)
        ||!(fabsf(e->radius_y)<=FLT_MAX)
        ||!(fabsf(e->radius_z)<=FLT_MAX))
        return 0;
    const float pi=3.14159265358979323846f;
    size_t at=0u;
    for(unsigned lat=0;lat<REVERIE_NATIVE_AVATAR_RINGS_V1;lat++) {
        float a=pi*(float)lat/REVERIE_NATIVE_AVATAR_RINGS_V1;
        float b=pi*(float)(lat+1u)/REVERIE_NATIVE_AVATAR_RINGS_V1;
        for(unsigned lon=0;lon<REVERIE_NATIVE_AVATAR_SLICES_V1;lon++) {
            float c=2.0f*pi*(float)lon/REVERIE_NATIVE_AVATAR_SLICES_V1;
            float d=2.0f*pi*(float)(lon+1u)/REVERIE_NATIVE_AVATAR_SLICES_V1;
            float angles[6][2]={{a,c},{b,c},{a,d},{b,c},{b,d},{a,d}};
            for(unsigned v=0;v<6u;v++) {
                float theta=angles[v][0],phi=angles[v][1];
                float radial=sinf(theta);
                out[at++]=e->center_x+e->radius_x*radial*cosf(phi);
                out[at++]=e->center_y+e->radius_y*cosf(theta);
                out[at++]=e->center_z+e->radius_z*radial*sinf(phi);
            }
        }
    }
    return at==REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1*3u;
}

/*
 * Seven named/ordered parts, in floor-relative meters:
 * torso, hips, left/right arms, left/right legs, head.
 * Games may color/material each consecutive part independently. Avatar
 * bodies never alter physics automatically: use the stable collider above.
 */
static inline int ReverieNativeWriteAvatarFigureV1(
    const ReverieNativeAvatarV1 *avatar,
    float center_x,float feet_y,float center_z,
    float *out,size_t float_capacity
) {
    if(!ReverieNativeAvatarValidV1(avatar) ||out==NULL
        ||float_capacity<REVERIE_NATIVE_AVATAR_FIGURE_FLOATS_V1
        ||!(fabsf(center_x)<10000.0f)
        ||!(fabsf(feet_y)<10000.0f)
        ||!(fabsf(center_z)<10000.0f))return 0;
    float ht=(float)avatar->fields[REVERIE_NATIVE_AVATAR_HEIGHT];
    float sh=(float)avatar->fields[REVERIE_NATIVE_AVATAR_SHOULDERS];
    float wa=(float)avatar->fields[REVERIE_NATIVE_AVATAR_WAIST];
    float hip=(float)avatar->fields[REVERIE_NATIVE_AVATAR_HIPS];
    float mu=(float)avatar->fields[REVERIE_NATIVE_AVATAR_MUSCLE];
    float legs=.58f+.045f*ht;
    float shoulder=.19f+.032f*sh+.008f*mu;
    float waist=.14f+.021f*wa;
    float hipwidth=.15f+.026f*hip;
    float armradius=.045f+.013f*mu;
    float legradius=.065f+.008f*mu+.005f*hip;
    float torsoY=feet_y+legs+.30f;
    float headY=feet_y+legs+.82f;
    ReverieNativeBodyEllipsoidV1 parts[REVERIE_NATIVE_AVATAR_FIGURE_PARTS_V1]={
        {center_x,torsoY,center_z,shoulder,.33f,waist+.06f},
        {center_x,feet_y+legs-.03f,center_z,hipwidth,.20f,hipwidth*.70f},
        {center_x-shoulder-armradius,torsoY-.10f,center_z,
            armradius,.34f,armradius},
        {center_x+shoulder+armradius,torsoY-.10f,center_z,
            armradius,.34f,armradius},
        {center_x-hipwidth*.48f,feet_y+legs*.49f,center_z,
            legradius,legs*.49f,legradius},
        {center_x+hipwidth*.48f,feet_y+legs*.49f,center_z,
            legradius,legs*.49f,legradius},
        {center_x,headY,center_z,.17f,.21f,.16f}
    };
    const size_t stride=REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1*3u;
    for(size_t i=0;i<REVERIE_NATIVE_AVATAR_FIGURE_PARTS_V1;i++)
        if(!ReverieNativeWriteEllipsoidTrianglesV1(
            out+i*stride,float_capacity-i*stride,&parts[i]))return 0;
    return 1;
}

static inline void ReverieNativeAvatarColorV1(
    uint32_t palette,float *r,float *g,float *b
) {
    static const float rgb[16][3]={
        {.25f,.64f,.84f},{.92f,.49f,.28f},{.35f,.65f,.37f},
        {.60f,.40f,.76f},{.91f,.48f,.64f},{.22f,.75f,.70f},
        {.96f,.78f,.30f},{.30f,.36f,.53f},{.64f,.17f,.24f},
        {.80f,.87f,.91f},{.75f,.56f,.24f},{.95f,.91f,.78f},
        {.16f,.17f,.22f},{.76f,.64f,.95f},{.78f,.42f,.22f},
        {.09f,.54f,.34f}
    };
    const uint32_t index=palette<16u?palette:0u;
    if(r!=NULL)*r=rgb[index][0];
    if(g!=NULL)*g=rgb[index][1];
    if(b!=NULL)*b=rgb[index][2];
}

#ifdef __cplusplus
}
#endif
#endif
