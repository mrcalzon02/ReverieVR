#ifndef REVERIE_BREAKWATER_CANNON_TRAJECTORY_H
#define REVERIE_BREAKWATER_CANNON_TRAJECTORY_H
#include "breakwater_gun_mesh.h"
#include "breakwater_weapons.h"
#include <cmath>

namespace reverie { namespace breakwater { namespace trajectory {
struct ShotRay {
    float origin[3] = {};
    float direction[3] = {};
};
// In-range live cannon fire MUST launch from the exact world-space barrel
// mouth. The controller selects orientation, not shot origin. Out-of-traverse
// aim is a controller ghost sight only and cannot cause a regular gun shot.
// SAM remains an intentionally independent, controller-directed support.
inline bool CannonShot(WeaponKind weapon,const float* controller_direction,
                       ShotRay* out) {
    if(!out || !WeaponWithinTraverse(controller_direction,weapon))return false;
    gunmesh::Vec muzzle{};
    // A round departs the barrel at the *resting* muzzle; recoil begins
    // immediately afterward. Preview and initial shot origin therefore match
    // the visible pre-shot metal tube exactly, rather than the controller.
    if(!gunmesh::Muzzle(weapon,controller_direction,0.f,&muzzle))return false;
    const float x=controller_direction[0],y=controller_direction[1],
                z=controller_direction[2];
    const float length=std::sqrt(x*x+y*y+z*z);
    if(!std::isfinite(length) || length<.000001f)return false;
    ShotRay valid{};
    valid.origin[0]=muzzle.x;valid.origin[1]=muzzle.y;valid.origin[2]=muzzle.z;
    valid.direction[0]=x/length;
    valid.direction[1]=y/length;
    valid.direction[2]=z/length;
    *out=valid;
    return true;
}
inline bool Prediction(const ShotRay &ray,float gravity,float distance,
                       float* position) {
    return ProjectilePosition(ray.origin,ray.direction,distance,gravity,position);
}
}}}
#endif
