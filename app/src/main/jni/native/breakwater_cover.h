#ifndef REVERIE_BREAKWATER_COVER_H
#define REVERIE_BREAKWATER_COVER_H

#include "breakwater_campaign.h"
#include "breakwater_weapons.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {

// Battle-only, destructible shoreline obstructions. The same dimensions
// govern projectile interception and 3D rendering, without independent
// per-eye geometry or arbitrary infantry damage reduction.
constexpr float kCoverBaseY=-1.035f;
constexpr float kCoverDepth=.24f;
constexpr float kCoverWidth=.84f;
constexpr int kCoverTargetBase=static_cast<int>(
    Campaign::kMaxEnemies+Campaign::kMaxInfantry);
constexpr int kCoverTargetEnd=kCoverTargetBase+
    static_cast<int>(Campaign::kMaxCover);

inline float CoverHeight(const ShoreCover &cover) {
    if(cover.hp<=0 || cover.max_hp<=0)return 0.0f;
    const float proportion=std::max(0.0f,std::min(1.0f,
        static_cast<float>(cover.hp)/cover.max_hp));
    // Degraded barricades progressively reveal a larger aiming window.
    return .20f+.24f*proportion;
}
inline float CoverWidth(const ShoreCover &) {
    return kCoverWidth;
}

inline std::array<ShoreCover,Campaign::kMaxCover> MakeShoreCover(
    uint8_t location
) {
    std::array<ShoreCover,Campaign::kMaxCover> walls{};
    // Shingle Bay intentionally has no opening-wave cover.
    if(location==1u) {
        walls[0]={-2.35f,-1.17f,105,105};
        walls[1]={0.0f,-1.17f,105,105};
        walls[2]={2.35f,-1.17f,105,105};
    } else if(location==2u) {
        walls[0]={-2.35f,-1.19f,170,170};
        walls[1]={-.78f,-1.19f,170,170};
        walls[2]={.78f,-1.19f,170,170};
        walls[3]={2.35f,-1.19f,170,170};
    }
    return walls;
}

// The projectile's forward Z component is monotonic, so one crossing of
// the near face determines whether it entered a cover block. Evaluate the
// true drop at that crossing, not the controller's straight sight ray.
inline bool ProjectileHitsCover(
    const float *origin,const float *direction,float gravity,
    const ShoreCover &cover,float *distance
) {
    if(!origin || !direction || !distance || cover.hp<=0 ||
       cover.max_hp<=0 || !std::isfinite(cover.x) ||
       !std::isfinite(cover.z) || !std::isfinite(gravity) ||
       gravity<0.0f)return false;
    for(int i=0;i<3;++i)
        if(!std::isfinite(origin[i]) || !std::isfinite(direction[i]))
            return false;
    if(direction[2]>=-.0001f)return false;
    // Z travel is monotonic. Clip the projectile time interval to the
    // front/back faces and then to the X slab, before solving the
    // parabolic Y intersections. This also catches shots dropping
    // *into the top* of a wall after clearing its front edge.
    const double front=static_cast<double>(cover.z)+kCoverDepth*.5;
    const double rear=static_cast<double>(cover.z)-kCoverDepth*.5;
    double enter=std::max(.000001,
        (front-origin[2])/direction[2]);
    double leave=std::min(40.0,
        (rear-origin[2])/direction[2]);
    if(leave<enter || !std::isfinite(enter) ||
       !std::isfinite(leave))return false;
    const double left=static_cast<double>(cover.x)-CoverWidth(cover)*.5;
    const double right=static_cast<double>(cover.x)+CoverWidth(cover)*.5;
    if(std::abs(direction[0])<1.0e-8) {
        if(origin[0]<left || origin[0]>right)return false;
    } else {
        const double a=(left-origin[0])/direction[0];
        const double b=(right-origin[0])/direction[0];
        enter=std::max(enter,std::min(a,b));
        leave=std::min(leave,std::max(a,b));
        if(leave<enter)return false;
    }
    const double low=kCoverBaseY;
    const double high=kCoverBaseY+CoverHeight(cover);
    const auto yAt=[&](double t) {
        return origin[1]+direction[1]*t-static_cast<double>(gravity)*t*t;
    };
    const auto within=[&](double t) {
        return t>=enter-1e-8 && t<=leave+1e-8 &&
            yAt(t)>=low-1e-7 && yAt(t)<=high+1e-7;
    };
    double first=1.0e30;
    if(within(enter))first=enter;
    const auto crossing=[&](double bound) {
        if(gravity==0.0f) {
            if(std::abs(direction[1])<1.0e-9)return;
            const double t=(bound-origin[1])/direction[1];
            if(within(t))first=std::min(first,t);
            return;
        }
        const double dy=direction[1],g=gravity;
        const double discriminant=dy*dy+4.0*g*(origin[1]-bound);
        if(discriminant<0.0 || !std::isfinite(discriminant))return;
        const double root=std::sqrt(discriminant);
        const double earlier=(dy-root)/(2.0*g);
        const double later=(dy+root)/(2.0*g);
        if(within(earlier))first=std::min(first,earlier);
        if(within(later))first=std::min(first,later);
    };
    crossing(low);
    crossing(high);
    if(first>40.0 || !std::isfinite(first))return false;
    *distance=static_cast<float>(first);
    return true;
}

inline int CoverTargetFromArray(
    const std::array<ShoreCover,Campaign::kMaxCover> &cover,
    const float *origin,const float *direction,
    float gravity,float *distance
) {
    if(!distance)return -1;
    float nearest=1.0e30f;
    int selected=-1;
    for(size_t i=0;i<Campaign::kMaxCover;++i) {
        float hit=0;
        if(ProjectileHitsCover(origin,direction,gravity,
                               cover[i],&hit) &&
           hit<nearest) {
            nearest=hit;
            selected=static_cast<int>(i);
        }
    }
    if(selected>=0)*distance=nearest;
    return selected;
}

// Public terrain source of truth: the actual active campaign barriers.
inline int CoverTarget(const Campaign &game,
                       const float *origin,const float *direction,
                       float gravity,float *distance) {
    return CoverTargetFromArray(game.shore_cover(),origin,direction,
                                gravity,distance);
}

// Render an inexpensive sandbag/concrete/basalt wall using the same
// physical barrier height and x/z coordinates. A broken wall is absent.
// Schematic blocks sit within the projectile-tested AABB.
template<class BoxFn>
void EmitShoreCover(uint8_t location,const ShoreCover &cover,
                    bool night,BoxFn box) {
    const float height=CoverHeight(cover);
    if(height<=0.0f)return;
    float r=location==1u?.31f:.27f;
    float g=location==1u?.33f:.29f;
    float b=location==1u?.31f:.30f;
    if(night){r*=.48f;g*=.58f;b*=.67f;}
    const float y=kCoverBaseY+height*.5f;
    box(cover.x,y,cover.z,CoverWidth(cover),height,kCoverDepth,r,g,b);
    // Small stacked edge stones/sandbags; no protrusion beyond collision box.
    for(int n=0;n<3;++n) {
        const float x=cover.x+(static_cast<float>(n)-1.0f)*.265f;
        box(x,kCoverBaseY+height*.86f,cover.z,
            .245f,height*.25f,kCoverDepth*.97f,
            r*1.12f,g*1.09f,b*1.06f);
    }
}

}} // namespace reverie::breakwater
#endif
