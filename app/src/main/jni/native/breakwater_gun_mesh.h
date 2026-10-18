#ifndef REVERIE_BREAKWATER_GUN_MESH_H
#define REVERIE_BREAKWATER_GUN_MESH_H

#include "breakwater_weapons.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace reverie { namespace breakwater { namespace gunmesh {
// Game-private solid 3D artillery recipe; emits XYZRGB triangles using the
// existing GLES2 flat-color buffer without textures or runtime dependencies.
constexpr int kSides=12;
constexpr float kPi=3.14159265358979323846f;
inline float RecoilDistance(float shot_flash) {
    // The existing confirmed-shot flash lasts 90 ms; do not create recoil
    // on failed fire, weapon switching or idle mouse/controller movement.
    return std::isfinite(shot_flash)&&shot_flash>0.f
        ? std::min(.12f,shot_flash*1.30f) : 0.f;
}
inline float BarrelLength(WeaponKind kind) {
    return kind==WeaponKind::DirectCannon?1.47f:
           kind==WeaponKind::Flak?1.63f:
           kind==WeaponKind::MissileBattery?1.25f:1.42f;
}
struct Vec {float x,y,z;};
    // Seat/breech mount is below and just behind the viewer; the resting
    // barrel tips stay landward of the advancing infantry/armor firing line.
    // Previously z=-.88 pushed every 1.25-1.63-unit muzzle beyond the beach,
    // making landed enemies physically behind the cannon's shot origin.
    // Raise the breech above the battery parapet; launch and visible
    // barrel use the same elevated mount, not an under-cover trajectory.
    constexpr Vec kMount={0.f,-.53f,.65f};
inline Vec Add(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec Scale(Vec a,float v){return {a.x*v,a.y*v,a.z*v};}
inline float Dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec Cross(Vec a,Vec b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
inline Vec Normalize(Vec v) {
    const float l=std::sqrt(Dot(v,v));
    return l>0.0001f && std::isfinite(l)?Scale(v,1.f/l):Vec{0.f,0.f,-1.f};
}
// Shared with combat targeting. This exactly matches the gun mesh's
// moving barrel tip, not a controller offset or approximated camera ray.
inline bool Muzzle(WeaponKind kind,const float* direction,float flash,Vec* out) {
    if(!out || !direction ||
       !std::isfinite(direction[0]) ||
       !std::isfinite(direction[1]) ||
       !std::isfinite(direction[2]))return false;
    const float length2=Dot({direction[0],direction[1],direction[2]},
                            {direction[0],direction[1],direction[2]});
    if(!std::isfinite(length2) || length2<.000001f)return false;
    const Vec aim=Normalize({direction[0],direction[1],direction[2]});
    const Vec mount=kMount;
    *out=Add(mount,Scale(aim,BarrelLength(kind)-RecoilDistance(flash)));
    return true;
}
inline Vec RingPoint(Vec center,Vec right,Vec up,float radius,int i) {
    const float theta=2.f*kPi*static_cast<float>(i)/static_cast<float>(kSides);
    return Add(center,Add(Scale(right,radius*std::cos(theta)),
                          Scale(up,radius*std::sin(theta))));
}
template<typename Emit>
inline void Tri(Emit &emit,Vec a,Vec b,Vec c,float r,float g,float bl) {
    emit(a.x,a.y,a.z,b.x,b.y,b.z,c.x,c.y,c.z,r,g,bl);
}
template<typename Emit>
inline void Tube(Emit &emit,Vec rear,Vec forward,Vec right,Vec up,
                 float rear_radius,float tip_radius,
                 float r,float g,float b,bool open_bore=true) {
    for(int i=0;i<kSides;++i) {
        const int j=(i+1)%kSides;
        const Vec a=RingPoint(rear,right,up,rear_radius,i);
        const Vec d=RingPoint(rear,right,up,rear_radius,j);
        const Vec c=RingPoint(forward,right,up,tip_radius,i);
        const Vec e=RingPoint(forward,right,up,tip_radius,j);
        const float light=(i%3==0)?1.10f:(i%3==1)?0.90f:1.0f;
        Tri(emit,a,d,c,r*light,g*light,b*light);
        Tri(emit,d,e,c,r*light,g*light,b*light);
        if(open_bore) {
            const Vec innerA=RingPoint(forward,right,up,tip_radius*.57f,i);
            const Vec innerB=RingPoint(forward,right,up,tip_radius*.57f,j);
            Tri(emit,c,e,innerB,r*.60f,g*.61f,b*.63f);
            Tri(emit,c,innerB,innerA,r*.60f,g*.61f,b*.63f);
            const Vec recess=Add(forward,Scale(Normalize(Add(forward,Scale(rear,-1.f))),-.065f));
            const Vec innerBackA=RingPoint(recess,right,up,tip_radius*.57f,i);
            const Vec innerBackB=RingPoint(recess,right,up,tip_radius*.57f,j);
            Tri(emit,innerA,innerB,innerBackA,.065f,.075f,.074f);
            Tri(emit,innerB,innerBackB,innerBackA,.065f,.075f,.074f);
        }
    }
}
template<typename Emit>
inline void Wheel(Emit &emit,float x,float y,float z,float radius,
                  float r,float g,float b) {
    const Vec right={0.f,1.f,0.f},up={0.f,0.f,1.f};
    const Vec hub={x,y,z};
    // Flat faceted wheel has real near/far surfaces and a thick rim,
    // unlike a crossed billboard or a single disc.
    const float side=(x<0.f)?-.085f:.085f;
    for(int i=0;i<kSides;++i) {
        const int j=(i+1)%kSides;
        const Vec outerA=RingPoint(hub,right,up,radius,i);
        const Vec outerB=RingPoint(hub,right,up,radius,j);
        const Vec farA={outerA.x+side,outerA.y,outerA.z};
        const Vec farB={outerB.x+side,outerB.y,outerB.z};
        Tri(emit,hub,outerA,outerB,r*.78f,g*.72f,b*.58f);
        Tri(emit,outerA,farA,outerB,r*.45f,g*.43f,b*.38f);
        Tri(emit,farA,farB,outerB,r*.45f,g*.43f,b*.38f);
        const Vec innerA=RingPoint(hub,right,up,radius*.24f,i);
        const Vec innerB=RingPoint(hub,right,up,radius*.24f,j);
        Tri(emit,hub,innerA,innerB,.56f,.54f,.45f);
    }
}
template<typename Emit>
inline void Build(Emit emit,WeaponKind kind,const float *direction,
                  float shot_flash=0.f) {
    if(!direction || !std::isfinite(direction[0]) ||
       !std::isfinite(direction[1]) || !std::isfinite(direction[2]))return;
    const Vec aim=Normalize({direction[0],direction[1],direction[2]});
    // Basis follows the player's physical gun direction, including elevation.
    const Vec right=Normalize({-aim.z,0.f,aim.x});
    const Vec up=Normalize(Cross(right,aim));
    const Vec mount=kMount;
    const Vec rear=Add(mount,Scale(aim,-RecoilDistance(shot_flash)));
    const float radius=kind==WeaponKind::DirectCannon?.175f:
        kind==WeaponKind::HeavyGun?.14f:
        kind==WeaponKind::Flak?.125f:
        kind==WeaponKind::MissileBattery?.12f:.09f;
    const float length=BarrelLength(kind);
    const float r=kind==WeaponKind::DirectCannon?.36f:.32f;
    const float g=kind==WeaponKind::Flak?.41f:.36f;
    const float b=kind==WeaponKind::MissileBattery?.36f:.31f;
    // Rear breech, fitted metal collar, tapered barrel and hollow muzzle.
    const Vec breech=Add(rear,Scale(aim,.28f));
    Tube(emit,rear,breech,right,up,radius*1.25f,radius*1.18f,
         .25f,.29f,.28f,false);
    const Vec collar=Add(rear,Scale(aim,.40f));
    Tube(emit,breech,collar,right,up,radius*1.28f,radius*1.13f,
         .49f,.49f,.42f,false);
    const Vec tip=Add(rear,Scale(aim,length));
    Tube(emit,collar,tip,right,up,radius*.96f,radius*.80f,r,g,b);
    // Muzzle reinforcement has a second observable ring.
    Tube(emit,Add(tip,Scale(aim,-.10f)),tip,right,up,
         radius*.97f,radius*.92f,.42f,.44f,.40f);
    if(kind==WeaponKind::Flak || kind==WeaponKind::MissileBattery) {
        // A visible offset second launcher barrel for heavy antiair mounts.
        const Vec offset=Scale(right,.26f);
        Tube(emit,Add(rear,offset),Add(tip,offset),right,up,
             radius*.69f,radius*.62f,.33f,.38f,.34f);
    }
    // Every fixed carriage piece follows the relocated gun breech as a
    // solid assembly; only barrel/breech tube retracts on confirmed fire.
    Wheel(emit,-.44f,-1.04f,mount.z-.06f,.26f,.25f,.24f,.21f);
    Wheel(emit,.44f,-1.04f,mount.z-.06f,.26f,.25f,.24f,.21f);
    // Sloped armor plates provide a stable recognizable gun shield in 3D.
    const Vec p0={-.52f,-.95f,mount.z-.20f};
    const Vec p1={.52f,-.95f,mount.z-.20f};
    const Vec p2={-.44f,-.38f,mount.z-.29f};
    const Vec p3={.44f,-.38f,mount.z-.29f};
    Tri(emit,p0,p1,p2,.30f,.37f,.32f);
    Tri(emit,p1,p3,p2,.30f,.37f,.32f);
    // Faint upper rim edge, two forward braces and a solid rear breech face.
    Tri(emit,{- .44f,-.38f,mount.z-.29f},
        {.44f,-.38f,mount.z-.29f},
        {.44f,-.42f,mount.z-.36f},.52f,.54f,.47f);
    Tri(emit,{- .44f,-.38f,mount.z-.29f},
        {.44f,-.42f,mount.z-.36f},
        {-.44f,-.42f,mount.z-.36f},.52f,.54f,.47f);
}
// A real muzzle-flash volume built from triangles around the tube's moving
// mouth. Called only for a confirmed 90-ms shot, not when Select is rejected.
template<typename Emit>
inline void Flash(Emit emit,WeaponKind kind,const float *direction,
                  float shot_flash) {
    if(!direction || !std::isfinite(direction[0]) ||
       !std::isfinite(direction[1]) || !std::isfinite(direction[2]) ||
       !std::isfinite(shot_flash) || shot_flash<=0.f)return;
    const float intensity=std::clamp(shot_flash/.09f,0.f,1.f);
    const Vec aim=Normalize({direction[0],direction[1],direction[2]});
    const Vec right=Normalize({-aim.z,0.f,aim.x});
    const Vec up=Normalize(Cross(right,aim));
    const Vec mount=kMount;
    const Vec muzzle=Add(mount,Scale(aim,
        BarrelLength(kind)-RecoilDistance(shot_flash)));
    const float radius=.10f+.12f*intensity;
    const Vec apex=Add(muzzle,Scale(aim,.11f+.29f*intensity));
    for(int i=0;i<kSides;++i){
        const Vec a=RingPoint(muzzle,right,up,radius,i);
        const Vec b=RingPoint(muzzle,right,up,radius,(i+1)%kSides);
        if(i%3==0)
            Tri(emit,a,b,apex,1.0f,.78f,.29f);
        else
            Tri(emit,a,b,apex,.96f,.50f,.16f);
    }
}
}}}
#endif
