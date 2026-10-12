#ifndef REVERIE_BREAKWATER_WEAPONS_H
#define REVERIE_BREAKWATER_WEAPONS_H
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class WeaponKind : uint8_t {
    MachineGun, HeavyGun, Flak, MissileBattery, DirectCannon
};
constexpr uint8_t kWeaponCount = 5;
struct WeaponSpec {
    const char *name;
    uint16_t fire_delay_ms, damage, magazine;
    float gravity, elevation_ratio;
    int32_t purchase_cost;
    bool air_capable, uses_ghost, anti_infantry;
};
// Named compact equipment: trajectories are world-unit approximations,
// deliberately exaggerated for readable low-poly coastal engagements.
inline WeaponSpec WeaponFor(WeaponKind kind) {
    switch(kind) {
        case WeaponKind::HeavyGun:
            return {"HEAVY",430,35,8,.008f,.95f,900,false,false,true};
        case WeaponKind::Flak:
            return {"FLAK",560,42,6,.001f,3.30f,1350,true,false,false};
        case WeaponKind::MissileBattery:
            return {"MISSILE",1450,125,2,0.0f,10.0f,2200,true,true,false};
        case WeaponKind::DirectCannon:
            return {"CANNON",950,90,4,.026f,.40f,1600,false,false,false};
        default:
            return {"MACHINE",300,20,12,.014f,1.60f,0,false,false,true};
    }
}
inline bool WeaponValid(uint8_t id) { return id<kWeaponCount; }
inline float WeaponTraverseLimit(WeaponKind kind) {
    return WeaponFor(kind).elevation_ratio;
}
inline bool WeaponWithinTraverse(const float *d, WeaponKind kind) {
    if(!d || !std::isfinite(d[0]) || !std::isfinite(d[1]) ||
       !std::isfinite(d[2]) || d[2]>=-.18f) return false;
    const float horizontal=-d[2];
    return std::abs(d[0])<=horizontal*2.5f &&
           std::abs(d[1])<=horizontal*WeaponTraverseLimit(kind);
}
inline bool ProjectilePosition(const float *origin,const float *direction,
                               float t,float gravity,float *out) {
    if(!origin || !direction || !out || !std::isfinite(t) ||
       !std::isfinite(gravity) || gravity<0 || t<0 || t>40) return false;
    for(int i=0;i<3;i++)
        if(!std::isfinite(origin[i]) || !std::isfinite(direction[i]))
            return false;
    for(int i=0;i<3;i++)out[i]=origin[i]+direction[i]*t;
    out[1]-=gravity*t*t;
    return std::isfinite(out[0])&&std::isfinite(out[1])&&std::isfinite(out[2]);
}
// Computes a collision against the true dropping trajectory. A straight
// reticle ray aimed at a far boat no longer gives free damage from gunfire.
inline bool ProjectileHitsSphere(const float *origin,const float *direction,
                                 float gravity,const float *center,float radius,
                                 float *distance) {
    if(!origin || !direction || !center || !distance ||
       !std::isfinite(radius) || radius<=0.0f ||
       !std::isfinite(gravity) || gravity<0.0f) return false;
    for(int i=0;i<3;i++)
        if(!std::isfinite(center[i]) || !std::isfinite(origin[i]) ||
           !std::isfinite(direction[i]))return false;
    // The projectile must be travelling into the scene. Each enemy sphere
    // occupies a bounded depth interval, which bounds the minimizer.
    if(direction[2]>=-.0001f)return false;
    const float atZ=(center[2]-origin[2])/direction[2];
    const float halfWindow=radius / -direction[2]+.18f;
    const float from=std::max(.001f,atZ-halfWindow);
    const float to=std::min(40.0f,atZ+halfWindow);
    if(to<=from)return false;
    // Golden-section minimization over the target's depth slice; no heap
    // allocation, frame-independent result and independent of stereo eye.
    auto distanceSquared=[&](float t) {
        float point[3]={};
        if(!ProjectilePosition(origin,direction,t,gravity,point))
            return 1.0e30f;
        const float dx=point[0]-center[0],dy=point[1]-center[1],
                    dz=point[2]-center[2];
        return dx*dx+dy*dy+dz*dz;
    };
    float lo=from,hi=to;
    constexpr float invphi=.61803398875f;
    float a=hi-(hi-lo)*invphi,b=lo+(hi-lo)*invphi;
    float da=distanceSquared(a),db=distanceSquared(b);
    for(int n=0;n<22;n++) {
        if(da>db) {lo=a;a=b;da=db;b=lo+(hi-lo)*invphi;db=distanceSquared(b);}
        else {hi=b;b=a;db=da;a=hi-(hi-lo)*invphi;da=distanceSquared(a);}
    }
    const float nearest=(lo+hi)*.5f;
    if(distanceSquared(nearest)>radius*radius)return false;
    // Find the first positive point inside the sphere, preserving depth
    // ordering among multiple candidate targets.
    float low=from,high=nearest;
    for(int n=0;n<18;n++) {
        const float mid=(low+high)*.5f;
        if(distanceSquared(mid)<=radius*radius)high=mid;
        else low=mid;
    }
    *distance=high;
    return true;
}
// Independent ghost sight represents the actual controller pointer, even
// when the physical gun cannot elevate to follow it.
inline bool GhostAimPoint(const float *origin,const float *direction,float *out) {
    if(!origin || !direction || !out)return false;
    float length=0;
    for(int i=0;i<3;i++) {
        if(!std::isfinite(direction[i]))return false;
        length+=direction[i]*direction[i];
    }
    if(length<.000001f)return false;
    const float distance=5.0f/std::sqrt(length);
    return ProjectilePosition(origin,direction,distance,0.0f,out);
}
}} // namespace reverie::breakwater
#endif
