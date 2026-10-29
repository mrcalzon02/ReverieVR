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
    // Headset sight has a larger downward arc than the low-mounted barrel:
    // breakwater_cannon_trajectory.h converges sight-on-ground onto the
    // physical gun pivot without driving the muzzle through the platform.
    // This is a *controller* limit, not permission for a barrel below deck.
    const float depression=kind==WeaponKind::MachineGun?.78f:
                           kind==WeaponKind::HeavyGun?.72f:
                           kind==WeaponKind::Flak?.58f:
                           kind==WeaponKind::DirectCannon?.48f:.65f;
    return std::abs(d[0])<=horizontal*2.5f &&
           d[1]>=-depression*horizontal &&
           d[1]<=horizontal*WeaponTraverseLimit(kind);
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
    if(direction[2]>=-.0001f)return false;
    const double atZ=(static_cast<double>(center[2])-origin[2])/direction[2];
    const double halfWindow=static_cast<double>(radius)/-direction[2]+.18;
    const double from=std::max(.001,atZ-halfWindow);
    const double to=std::min(40.0,atZ+halfWindow);
    if(to<=from)return false;
    const double x=static_cast<double>(origin[0])-center[0];
    const double y=static_cast<double>(origin[1])-center[1];
    const double z=static_cast<double>(origin[2])-center[2];
    const double vx=direction[0],vy=direction[1],vz=direction[2],g=gravity;
    const auto squared=[&](double t) {
        const double dx=x+vx*t,dy=y+vy*t-g*t*t,dz=z+vz*t;
        return dx*dx+dy*dy+dz*dz;
    };
    const double radiusSquared=static_cast<double>(radius)*radius;
    if(squared(from)<=radiusSquared) {
        *distance=static_cast<float>(from);
        return true;
    }
    // Distance squared along a parabola is quartic, not unimodal.
    // A whole-window golden-section search can miss a close target or
    // the second arc crossing. Isolate all cubic-derivative stationary
    // points and scan them in shot-travel order for the first contact.
    const double a0=x*vx+y*vy+z*vz;
    const double a1=vx*vx+vy*vy+vz*vz-2.0*g*y;
    const double a2=-3.0*g*vy;
    const double a3=2.0*g*g;
    const auto derivative=[&](double t) {
        return ((a3*t+a2)*t+a1)*t+a0;
    };
    double knots[4]={from,to};
    size_t count=2;
    if(a3>0.0) {
        const double discriminant=a2*a2-3.0*a3*a1;
        if(discriminant>0.0) {
            const double root=std::sqrt(discriminant);
            const double first=(-a2-root)/(3.0*a3);
            const double second=(-a2+root)/(3.0*a3);
            if(first>from && first<to)knots[count++]=first;
            if(second>from && second<to)knots[count++]=second;
        }
    }
    std::sort(knots,knots+count);
    double firstInside=-1.0;
    for(size_t i=1;i<count;++i) {
        const double left=knots[i-1],right=knots[i];
        const double dl=derivative(left),dr=derivative(right);
        if((dl<0.0 && dr>0.0) || (dl>0.0 && dr<0.0)) {
            double lo=left,hi=right;
            for(int n=0;n<44;++n) {
                const double mid=(lo+hi)*.5;
                if((derivative(mid)>0.0)==(dl>0.0))lo=mid;
                else hi=mid;
            }
            const double stationary=(lo+hi)*.5;
            if(squared(stationary)<=radiusSquared) {
                firstInside=stationary;
                break;
            }
        }
        if(squared(right)<=radiusSquared) {
            firstInside=right;
            break;
        }
    }
    if(firstInside<0.0)return false;
    double lo=from,hi=firstInside;
    for(int n=0;n<32;++n) {
        const double mid=(lo+hi)*.5;
        if(squared(mid)<=radiusSquared)hi=mid;
        else lo=mid;
    }
    *distance=static_cast<float>(hi);
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
