#ifndef REVERIE_BREAKWATER_TRAJECTORY_GUIDE_H
#define REVERIE_BREAKWATER_TRAJECTORY_GUIDE_H
#include "breakwater_battlefield.h"
#include "breakwater_cannon_trajectory.h"
namespace reverie { namespace breakwater { namespace guide {
constexpr int kDashes=64;
inline float HalfWidth(float distance) {
    // Approximately constant angular visibility, rather than a 9mm strip
    // disappearing below one pixel as the battle moves hundreds of metres.
    return std::clamp(distance*.0018f,.025f,3.2f);
}
template<class Triangle>
int Emit(const trajectory::ShotRay &ray,float gravity,float distance,
         bool contact,defense::Position viewer,Triangle triangle) {
    if(!std::isfinite(distance) || distance<.3f || distance>battlefield::kRange)return 0;
    using gunmesh::Vec;using gunmesh::Add;using gunmesh::Scale;
    const Vec aim=gunmesh::Normalize({ray.direction[0],ray.direction[1],ray.direction[2]});
    const Vec right=gunmesh::Normalize({-aim.z,0.f,aim.x});
    const Vec up=gunmesh::Normalize(gunmesh::Cross(right,aim));
    int triangles=0;
    const auto ribbon=[&](Vec a,Vec b,Vec side,float wa,float wb,float r,float g,float blue) {
        const Vec p=Add(a,Scale(side,-wa)),q=Add(a,Scale(side,wa));
        const Vec u=Add(b,Scale(side,-wb)),v=Add(b,Scale(side,wb));
        triangle(p.x,p.y,p.z,q.x,q.y,q.z,v.x,v.y,v.z,r,g,blue);
        triangle(p.x,p.y,p.z,v.x,v.y,v.z,u.x,u.y,u.z,r,g,blue);triangles+=2;
    };
    const auto width=[&](const Vec &p){return HalfWidth(std::sqrt(
        (p.x-viewer.x)*(p.x-viewer.x)+(p.y-viewer.y)*(p.y-viewer.y)+
        (p.z-viewer.z)*(p.z-viewer.z)));};
    for(int i=0;i<kDashes;++i) {
        const float from=float(i)/kDashes,to=(float(i)+.72f)/kDashes;
        const float t0=.3f+(distance-.3f)*from*from;
        const float t1=.3f+(distance-.3f)*to*to;
        float a[3],b[3];
        if(!battlefield::Position(ray.origin,ray.direction,t0,gravity,a) ||
           !battlefield::Position(ray.origin,ray.direction,t1,gravity,b))return triangles;
        const Vec first{a[0],a[1],a[2]},next{b[0],b[1],b[2]};
        const float wa=width(first),wb=width(next);
        // Crossed world-space ribbons stay visible from side defense posts
        // and render identically for both eyes. Dark edge, luminous centre.
        for(const Vec side:{right,up}) {
            // Only the border bands are dark. Do not cover the bright
            // centre with a coplanar full quad (which loses the depth test).
            for(int sign:{-1,1})ribbon(Add(first,Scale(side,sign*wa*1.225f)),
                Add(next,Scale(side,sign*wb*1.225f)),side,
                wa*.225f,wb*.225f,.025f,.075f,.09f);
            ribbon(first,next,side,wa,wb,.12f,.96f,1.f);
        }
    }
    if(contact) {
        float p[3];if(!battlefield::Position(ray.origin,ray.direction,distance,gravity,p))return triangles;
        const Vec center{p[0],p[1]+.08f,p[2]};
        const float radius=std::max(.15f,width(center)*2.8f);
        const Vec corners[]={Add(center,Scale(right,radius)),Add(center,Scale(up,radius)),
            Add(center,Scale(right,-radius)),Add(center,Scale(up,-radius))};
        for(int i=0;i<4;++i)ribbon(corners[i],corners[(i+1)%4],
            gunmesh::Normalize(Add(corners[i],Scale(center,-1.f))),
            radius*.12f,radius*.12f,1.f,.80f,.16f);
    }
    return triangles;
}
}}}
#endif
