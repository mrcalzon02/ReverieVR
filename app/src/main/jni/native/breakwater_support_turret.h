#ifndef REVERIE_BREAKWATER_SUPPORT_TURRET_H
#define REVERIE_BREAKWATER_SUPPORT_TURRET_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>

namespace reverie { namespace breakwater { namespace support {
constexpr float kTurretX=2.15f;
constexpr float kTurretZ=-1.40f;
constexpr float kTurretMuzzleY=-.38f;
// An allied battery must not silently snipe first-spawn boats on the horizon.
// The player sees its gun, traversing and tracer once threats close on shore.
constexpr float kMinimumTargetZ=-4.60f;
constexpr float kMaximumTargetZ=-1.34f;
inline bool InRange(const Enemy& enemy) {
    return enemy.active && enemy.hp>0 && enemy.type!=UnitType::Aircraft &&
           std::isfinite(enemy.z) &&
           enemy.z>=kMinimumTargetZ && enemy.z<=kMaximumTargetZ;
}
inline bool InRange(const ArmoredVehicle& vehicle) {
    return vehicle.active && vehicle.hp>0 && std::isfinite(vehicle.z) &&
           vehicle.z>=kMinimumTargetZ && vehicle.z<=kMaximumTargetZ;
}
// Build the stationary, recognizably friendly 3D emplacement. This is
// scenery linked to the actual purchased turret; there is no dummy actor.
// The gun points toward the latest authoritative target or seaward at rest.
// Box and triangle emitters write the same GLES2 triangle batch for both eyes.
template<class BoxFn,class TriangleFn>
inline void EmitTurret(float targetX,float targetZ,float flash,
                       BoxFn box,TriangleFn triangle) {
    if(!std::isfinite(targetX)||!std::isfinite(targetZ))return;
    const float x=kTurretX,z=kTurretZ;
    // Concrete footings, four physical bolts and an olive armor skirt.
    box(x,-1.15f,z,1.08f,.23f,1.02f,.35f,.36f,.33f);
    box(x,-.94f,z,.78f,.25f,.76f,.33f,.40f,.32f);
    for(int side : {-1,1})for(int fore : {-1,1})
        box(x+side*.42f,-1.03f,z+fore*.39f,.095f,.10f,.10f,
            .70f,.65f,.46f);
    // Raised gun pedestal, traversing ring and four-sided gun shield.
    box(x,-.67f,z,.41f,.42f,.41f,.24f,.29f,.25f);
    box(x,-.48f,z,.72f,.12f,.72f,.48f,.51f,.39f);
    box(x,-.33f,z,.58f,.24f,.54f,.33f,.39f,.31f);
    box(x,-.24f,z+.18f,.63f,.27f,.11f,.29f,.34f,.30f);
    // Readable ammo box, orange bolt panel and aerial warning beacon.
    box(x+.39f,-.40f,z+.19f,.28f,.31f,.38f,.40f,.43f,.29f);
    box(x+.39f,-.23f,z+.39f,.23f,.09f,.05f,.79f,.59f,.24f);
    box(x-.30f,-.01f,z+.22f,.035f,.38f,.038f,.36f,.37f,.34f);
    box(x-.30f,.19f,z+.22f,.11f,.09f,.11f,.85f,.62f,.31f);
    // Aim the mounted barrel itself, not just a disconnected muzzle spark.
    float dx=targetX-x,dz=targetZ-z;
    const float distance=std::sqrt(dx*dx+dz*dz);
    if(distance<.001f) {dx=0.f;dz=-1.f;}
    else {dx/=distance;dz/=distance;}
    const float rx=-dz*.095f,rz=dx*.095f;
    const float startX=x,startZ=z;
    const float endX=x+dx*.94f,endZ=z+dz*.94f;
    const float barrelY=kTurretMuzzleY;
    triangle(startX+rx,barrelY+.065f,startZ+rz,
             startX-rx,barrelY+.065f,startZ-rz,
             endX-rx*.64f,barrelY+.065f,endZ-rz*.64f,
             .19f,.23f,.22f);
    triangle(startX+rx,barrelY+.065f,startZ+rz,
             endX-rx*.64f,barrelY+.065f,endZ-rz*.64f,
             endX+rx*.64f,barrelY+.065f,endZ+rz*.64f,
             .19f,.23f,.22f);
    triangle(startX+rx,barrelY-.065f,startZ+rz,
             endX+rx*.64f,barrelY-.065f,endZ+rz*.64f,
             endX-rx*.64f,barrelY-.065f,endZ-rz*.64f,
             .12f,.15f,.16f);
    triangle(startX+rx,barrelY+.065f,startZ+rz,
             endX+rx*.64f,barrelY+.065f,endZ+rz*.64f,
             endX+rx*.64f,barrelY-.065f,endZ+rz*.64f,
             .24f,.28f,.24f);
    // Actual shot-only muzzle flash. It originates at the 3D barrel tip.
    if(std::isfinite(flash)&&flash>0.f) {
        const float tipX=endX+dx*.12f,tipZ=endZ+dz*.12f;
        triangle(endX-rx*.8f,barrelY-.08f,endZ-rz*.8f,
                 endX+rx*.8f,barrelY+.08f,endZ+rz*.8f,
                 tipX,barrelY,tipZ,1.f,.84f,.38f);
    }
}
inline void MuzzleToward(float targetX,float targetZ,float* x,float* y,float* z) {
    if(!x||!y||!z)return;
    const float dx=targetX-kTurretX,dz=targetZ-kTurretZ;
    const float length=std::sqrt(dx*dx+dz*dz);
    const float ux=length>.001f?dx/length:0.f;
    const float uz=length>.001f?dz/length:-1.f;
    *x=kTurretX+ux*.94f;
    *y=kTurretMuzzleY;
    *z=kTurretZ+uz*.94f;
}
}}}
#endif
