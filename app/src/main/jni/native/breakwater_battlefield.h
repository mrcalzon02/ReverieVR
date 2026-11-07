#ifndef REVERIE_BREAKWATER_BATTLEFIELD_H
#define REVERIE_BREAKWATER_BATTLEFIELD_H

#include "breakwater_cover.h"
#include "breakwater_gun_mesh.h"
#include "breakwater_defense_points.h"
#include <algorithm>
#include <cmath>

namespace reverie { namespace breakwater { namespace battlefield {
// Campaign/save coordinates remain deterministic. This bridge owns the actual
// metre-scale stage, BOTH model placement and inverse collision transforms.
constexpr float kLateral=18.f, kApproach=55.f, kOffsetZ=7.f;
constexpr float kSeaY=-20.f, kRange=2400.f, kFarClip=6000.f;
constexpr float X(float x) { return x*kLateral; }
constexpr float Z(float z) { return z*kApproach+kOffsetZ; }
constexpr float kShoreZ=Z(-2.2f);
constexpr float LandZ(float z) { return Z(z)+6.f; }
inline float ShoreZ(uint8_t location,float x) {
    // Each theater has a continuous, different waterline. The central landing
    // channel remains at the campaign's real beaching line.
    const float side=std::clamp((std::abs(x)-48.f)/100.f,0.f,1.f);
    return kShoreZ+side*side*(location==1?14.f:location==2?-35.f:
                             location==3?-18.f:25.f);
}
inline float GroundRecipe(uint8_t location,float x,float z) {
    const float shore=ShoreZ(location,x);
    const float t=std::clamp((z-shore)/(-6.f-shore),0.f,1.f);
    const float slope=t*t*(3.f-2.f*t);
    float h=kSeaY+18.96f*slope;
    // Flooded apron slopes below the water; never a vertical shoreline sheet.
    if(z<shore)h=kSeaY-std::min(9.f,(shore-z)*.12f);
    const float flank=std::clamp((std::abs(x)-65.f)/90.f,0.f,1.f);
    h+=flank*flank*(location==2?32.f:location==3?19.f:12.f)*t;
    if(z>12.f)h+=std::min(90.f,(z-12.f)*.18f)*
        (.8f+.2f*std::sin(x*.014f+location));
    return h;
}
constexpr float kGroundX[]={-512,-256,-160,-128,-96,-64,-48,-32,-24,-16,
    -12,-8,-6,-4,-2,0,2,4,6,8,12,16,24,32,48,64,96,128,160,256,512};
constexpr float kGroundZ[]={-192,-160,-136,-114,-100,-84,-68,-52,-36,-24,
    -16,-12,-8,-6,-4,-2,0,2,4,6,8,12,16,24,36,60,96,160,256,512};
inline float Ground(uint8_t location,float x,float z) {
    if(location>=Campaign::kLocations || !std::isfinite(x)||!std::isfinite(z))return kSeaY;
    const auto cell=[](const auto &grid,float v) {
        return std::clamp<int>(int(std::upper_bound(std::begin(grid),std::end(grid),v)-
            std::begin(grid))-1,0,int(std::size(grid))-2);
    };
    if(x<kGroundX[0] || x>kGroundX[std::size(kGroundX)-1] ||
       z<kGroundZ[0] || z>kGroundZ[std::size(kGroundZ)-1])return GroundRecipe(location,x,z);
    const int ix=cell(kGroundX,x),iz=cell(kGroundZ,z);
    const float x0=kGroundX[ix],x1=kGroundX[ix+1],z0=kGroundZ[iz],z1=kGroundZ[iz+1];
    const float u=(x-x0)/(x1-x0),v=(z-z0)/(z1-z0);
    // Bake the immutable height samples once; targeting/grounding never
    // recomputes terrain sine recipes in the frame loop.
    static const auto heights=[] {
        std::array<std::array<float,std::size(kGroundX)*std::size(kGroundZ)>,Campaign::kLocations> table{};
        for(uint8_t map=0;map<Campaign::kLocations;++map)
            for(size_t row=0;row<std::size(kGroundZ);++row)
                for(size_t col=0;col<std::size(kGroundX);++col)
                    table[map][row*std::size(kGroundX)+col]=GroundRecipe(map,kGroundX[col],kGroundZ[row]);
        return table;
    }();
    const auto sample=[&](int col,int row){return heights[location][row*std::size(kGroundX)+col];};
    const float a=sample(ix,iz),b=sample(ix+1,iz),c=sample(ix+1,iz+1),d=sample(ix,iz+1);
    return v<=u?a*(1.f-u)+b*(u-v)+c*v:a*(1.f-v)+c*u+d*(v-u);
}
inline float Gravity(float campaignGravity) {
    // Drop coefficient is per squared metre, not per old tabletop unit.
    return campaignGravity/(kApproach*kApproach);
}
struct Placement {
    float sx=0,sy=0,sz=0,wx=0,wy=0,wz=0,scale=1;
    void Point(float &x,float &y,float &z) const {
        x=wx+(x-sx)*scale;y=wy+(y-sy)*scale;z=wz+(z-sz)*scale;
    }
    void InverseRay(const float *origin,const float *direction,
                    float *localOrigin,float *localDirection) const {
        const float source[]={sx,sy,sz},world[]={wx,wy,wz};
        for(int i=0;i<3;++i) {
            localOrigin[i]=source[i]+(origin[i]-world[i])/scale;
            localDirection[i]=direction[i]/scale;
        }
    }
};
inline Placement Vessel(const Enemy &e) {
    const bool air=e.type==UnitType::Aircraft;
    const float scale=air?4.f:e.type==UnitType::Rowboat?3.f:4.f;
    // Keep the heavy bow/ramp at the same waterline as smaller landers;
    // its old 1.2-unit center offset must not become a 66m cargo gap.
    const float landingOffset=e.type==UnitType::ArmoredTransport?60.2f:
        e.type==UnitType::LandingCraft?-3.2f:e.type==UnitType::Rowboat?-1.5f:0.f;
    return {e.x,air?1.6f:-1.105f,e.z,X(e.x),air?65.f:kSeaY,Z(e.z)+landingOffset,scale};
}
inline Placement Land(uint8_t location,float x,float z,float scale=2.4f) {
    return {x,-1.04f,z,X(x),Ground(location,X(x),LandZ(z)),LandZ(z),scale};
}
inline bool Sphere(const Placement &p,const float *origin,const float *direction,
                   float gravity,const float *center,float radius,float *depth) {
    float localOrigin[3],localDirection[3];
    p.InverseRay(origin,direction,localOrigin,localDirection);
    return ProjectileHitsSphere(localOrigin,localDirection,gravity/p.scale,
                                center,radius,depth,kRange);
}
inline bool Cover(uint8_t location,const ShoreCover &cover,const float *origin,
                  const float *direction,float gravity,float *depth) {
    const Placement p=Land(location,cover.x,cover.z,3.f);
    float o[3],d[3];p.InverseRay(origin,direction,o,d);
    return ProjectileHitsCover(o,d,gravity/p.scale,cover,depth,kRange);
}
inline bool Position(const float *o,const float *d,float t,float g,float *out) {
    return ProjectilePosition(o,d,t,g,out,kRange);
}
// Bounded ground/water crossing, shared by aiming occlusion and misses.
// Coarse intervals are refined; no allocation or per-eye simulation.
inline bool Terrain(uint8_t location,const float *o,const float *d,
                    float gravity,float maximum,float *depth) {
    if(!o || !d || !depth || !std::isfinite(maximum) || maximum<=0.f)return false;
    maximum=std::min(maximum,kRange);
    const auto above=[&](float t) {
        float p[3];if(!Position(o,d,t,gravity,p))return -1.e6f;
        return p[1]-std::max(kSeaY,Ground(location,p[0],p[2]));
    };
    float prior=.01f;
    if(above(prior)<0.f){*depth=prior;return true;}
    for(int step=1;step<=192;++step) {
        const float next=maximum*step/192.f;
        if(next<=prior)continue;
        if(above(next)<=0.f) {
            float lo=prior,hi=next;
            for(int i=0;i<20;++i){const float mid=(lo+hi)*.5f;
                if(above(mid)>0.f)lo=mid;else hi=mid;}
            *depth=hi;return true;
        }
        prior=next;
    }
    return false;
}
inline bool BarrelAim(WeaponKind kind,const float *sight,float *barrel,
                      defense::Position station,uint8_t location=0) {
    if(!barrel || !WeaponWithinTraverse(sight,kind))return false;
    float len=std::sqrt(sight[0]*sight[0]+sight[1]*sight[1]+sight[2]*sight[2]);
    if(!std::isfinite(len)||len<.00001f)return false;
    for(int i=0;i<3;++i)barrel[i]=sight[i]/len;
    // Convergence uses the expanded shore, not the old -0.66m toy beach.
    const float eye[]={station.x,station.y,station.z};
    float distance=0.f;
    if(sight[1]<-.04f && Terrain(location,eye,barrel,0.f,kRange,&distance) && distance>8.f) {
        float target[3];Position(eye,barrel,distance,0.f,target);
        const float mount[]={gunmesh::kMount.x+station.x,
            gunmesh::kMount.y+station.y,gunmesh::kMount.z+station.z};
        len=0.f;for(int i=0;i<3;++i){barrel[i]=target[i]-mount[i];len+=barrel[i]*barrel[i];}
        len=std::sqrt(len);for(int i=0;i<3;++i)barrel[i]/=len;
    }
    return true;
}
}}}
#endif
