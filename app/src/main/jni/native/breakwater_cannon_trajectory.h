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
// The head-mounted sight rotates around the viewer, but the gun rotates
// around its *lower, rearward* physical pivot. Close-range infantry at the
// foot of the bunker cannot be hit by treating those two rays as parallel.
// Converge a downward gaze on the actual infantry-height beach plane,
// then orient the barrel from kMount to that target. The visible mesh and
// projectile must use this SAME physical direction. Far/level shots retain
// their old trajectory, preserving established anti-ship lead and ballistics.
inline bool BarrelAim(WeaponKind kind,const float *sight,float *barrel) {
    if(!barrel || !WeaponWithinTraverse(sight,kind))return false;
    const float len=std::sqrt(sight[0]*sight[0]+
                              sight[1]*sight[1]+sight[2]*sight[2]);
    if(!std::isfinite(len) || len<.00001f)return false;
    float x=sight[0]/len,y=sight[1]/len,z=sight[2]/len;
    if(y<-.12f*(-z)) {
        constexpr float infantryY=-.66f;
        const float t=infantryY/y;
        const float targetX=x*t,targetZ=z*t;
        // The muzzle must stay behind the intended shoreline contact:
        // otherwise a steep sight ray would hit the player's own deck.
        const float dx=targetX-gunmesh::kMount.x;
        const float dy=infantryY-gunmesh::kMount.y;
        const float dz=targetZ-gunmesh::kMount.z;
        const float reach=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(!std::isfinite(reach) ||
            reach<gunmesh::BarrelLength(kind)+.08f)return false;
        x=dx/reach;y=dy/reach;z=dz/reach;
    }
    barrel[0]=x;barrel[1]=y;barrel[2]=z;
    return true;
}
// In-range live cannon fire MUST launch from the exact world-space barrel
// mouth. The controller selects orientation, not shot origin. Out-of-traverse
// aim is a controller ghost sight only and cannot cause a regular gun shot.
// SAM remains an intentionally independent, controller-directed support.
inline bool CannonShot(WeaponKind weapon,const float* controller_direction,
                       ShotRay* out) {
    if(!out)return false;
    float physical[3]={};
    if(!BarrelAim(weapon,controller_direction,physical))return false;
    gunmesh::Vec muzzle{};
    // A round departs the barrel at the *resting* muzzle; recoil begins
    // immediately afterward. Preview and initial shot origin therefore match
    // the visible pre-shot metal tube exactly, rather than the controller.
    if(!gunmesh::Muzzle(weapon,physical,0.f,&muzzle))return false;
    const float x=physical[0],y=physical[1],z=physical[2];
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
