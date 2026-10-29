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
// Fully enclosed 3D mechanical housings, oriented with the gun elevation.
// Unlike decorative single triangles these expose six shaded faces.
template<typename Emit>
inline void Housing(Emit &emit,Vec center,Vec right,Vec up,Vec forward,
                    float halfW,float halfH,float halfD,
                    float r,float g,float b) {
    const auto corner=[&](float x,float y,float z) {
        return Add(center,Add(Scale(right,x*halfW),
                   Add(Scale(up,y*halfH),Scale(forward,z*halfD))));
    };
    const Vec a=corner(-1,-1,-1),c=corner(1,-1,-1);
    const Vec d=corner(-1,1,-1),e=corner(1,1,-1);
    const Vec f=corner(-1,-1,1),h=corner(1,-1,1);
    const Vec i=corner(-1,1,1),j=corner(1,1,1);
    Tri(emit,a,c,e,r*.74f,g*.76f,b*.76f);Tri(emit,a,e,d,r*.74f,g*.76f,b*.76f);
    Tri(emit,f,j,h,r*.93f,g*.94f,b*.95f);Tri(emit,f,i,j,r*.93f,g*.94f,b*.95f);
    Tri(emit,d,e,j,r*1.12f,g*1.11f,b*1.08f);
    Tri(emit,d,j,i,r*1.12f,g*1.11f,b*1.08f);
    Tri(emit,a,h,c,r*.51f,g*.55f,b*.57f);Tri(emit,a,f,h,r*.51f,g*.55f,b*.57f);
    Tri(emit,a,d,i,r*.63f,g*.64f,b*.69f);Tri(emit,a,i,f,r*.63f,g*.64f,b*.69f);
    Tri(emit,c,h,j,r*.83f,g*.85f,b*.86f);Tri(emit,c,j,e,r*.83f,g*.85f,b*.86f);
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
    // Changing weapons must visibly exchange the entire barrel assembly.
    // Every silhouette here is tied to the active WeaponKind and therefore
    // appears identically on the mounted cannon AND its miniature rack copy.
    // Keep the central tube's moving tip aligned with Muzzle()/ballistics.
    switch(kind) {
        case WeaponKind::MachineGun: {
            // Cooling jacket, hard receiver and actual ammunition feed
            // replace the former spindly four-pipe-only appearance.
            Housing(emit,Add(rear,Scale(aim,.50f)),right,up,aim,
                    .22f,.19f,.25f,.28f,.33f,.31f);
            Housing(emit,Add(rear,Add(Scale(aim,.17f),Scale(right,.29f))),
                    right,up,aim,.18f,.19f,.18f,.31f,.30f,.23f);
            for(int band=0;band<4;++band) {
                const Vec pos=Add(rear,Scale(aim,.88f+band*.13f));
                Tube(emit,pos,Add(pos,Scale(aim,.028f)),right,up,
                     .133f,.133f,.36f,.41f,.37f,false);
            }
            for(int link=0;link<4;++link) {
                const Vec pos=Add(rear,Add(Scale(aim,.19f+link*.065f),
                                 Scale(right,.28f-link*.035f)));
                Housing(emit,pos,right,up,aim,.062f,.075f,.043f,
                        .57f,.49f,.30f);
            }
            Housing(emit,Add(rear,Add(Scale(aim,.38f),Scale(up,-.23f))),
                    right,up,aim,.11f,.09f,.18f,.20f,.24f,.23f);
            // Rotary machine gun: five exposed slender barrels and binding
            // collars surrounding the primary, correctly aimed muzzle.
            for(int side : {-1,1})for(int tier : {-1,1}) {
                const Vec offset=Add(Scale(right,.115f*side),
                                     Scale(up,.10f*tier));
                Tube(emit,Add(rear,Add(Scale(aim,.30f),offset)),
                     Add(tip,offset),right,up,
                     .032f,.027f,.20f,.24f,.24f,false);
            }
            Tube(emit,Add(rear,Scale(aim,.64f)),
                 Add(rear,Scale(aim,.78f)),right,up,
                 .185f,.185f,.46f,.48f,.42f,false);
            // A broad feed block on the starboard side is an unambiguous
            // machine-gun feature, even from above the seated breech.
            const Vec feed=Add(rear,Add(Scale(aim,.26f),Scale(right,.26f)));
            Tri(emit,Add(feed,Scale(up,.13f)),
                     Add(feed,Scale(up,-.13f)),
                     Add(feed,Scale(right,.20f)),.42f,.36f,.25f);
            break;
        }
        case WeaponKind::HeavyGun: {
            // Chunky breech with slab receiver, dorsal recoil guide
            // and seated feed tray read as a heavy automatic weapon.
            Housing(emit,Add(rear,Scale(aim,.30f)),right,up,aim,
                    .30f,.245f,.32f,.30f,.34f,.30f);
            Housing(emit,Add(rear,Add(Scale(aim,.57f),Scale(up,.23f))),
                    right,up,aim,.19f,.095f,.33f,.40f,.43f,.36f);
            Housing(emit,Add(rear,Add(Scale(aim,.17f),Scale(right,.38f))),
                    right,up,aim,.18f,.17f,.23f,.34f,.31f,.24f);
            for(int band=0;band<3;++band) {
                const Vec pos=Add(rear,Scale(aim,.60f+band*.15f));
                Tube(emit,pos,Add(pos,Scale(aim,.040f)),right,up,
                    .229f,.229f,.43f,.44f,.36f,false);
            }
            // A single massive jacketed barrel, two external recoil
            // cylinders and a thick rear sleeve; no antiair pipe array.
            Tube(emit,Add(rear,Scale(aim,.43f)),
                 Add(rear,Scale(aim,1.02f)),right,up,
                 .215f,.195f,.28f,.34f,.32f,false);
            Tube(emit,Add(rear,Scale(aim,.92f)),
                 Add(rear,Scale(aim,1.07f)),right,up,
                 .205f,.205f,.51f,.50f,.43f,false);
            for(int side : {-1,1}) {
                const Vec offset=Scale(right,.29f*side);
                Tube(emit,Add(rear,offset),
                     Add(rear,Add(Scale(aim,.75f),offset)),
                     right,up,.072f,.062f,.31f,.34f,.29f,false);
            }
            break;
        }
        case WeaponKind::Flak: {
            // A central traverse gearbox and twin outboard recoil
            // blocks put actual mass behind the long AA barrels.
            Housing(emit,Add(rear,Scale(aim,.28f)),right,up,aim,
                    .25f,.23f,.31f,.32f,.38f,.33f);
            for(int side : {-1,1}) {
                const Vec offset=Scale(right,.34f*side);
                Housing(emit,Add(rear,Add(Scale(aim,.31f),offset)),
                        right,up,aim,.155f,.16f,.26f,.32f,.38f,.33f);
                Tube(emit,Add(rear,Add(Scale(aim,.64f),offset)),
                     Add(rear,Add(Scale(aim,.86f),offset)),
                     right,up,.117f,.117f,.44f,.49f,.41f,false);
            }
            Housing(emit,Add(rear,Add(Scale(aim,.16f),Scale(up,.27f))),
                    right,up,aim,.12f,.10f,.17f,.49f,.47f,.34f);
            // Twin extra-long outboard AA barrels and separate flash
            // suppressors form a recognizable triple-barrel flak mounting.
            for(int side : {-1,1}) {
                const Vec offset=Scale(right,.34f*side);
                Tube(emit,Add(rear,Add(Scale(aim,.22f),offset)),
                     Add(tip,offset),right,up,
                     .105f,.093f,.28f,.38f,.33f);
                Tube(emit,Add(tip,Add(Scale(aim,-.16f),offset)),
                     Add(tip,offset),right,up,
                     .14f,.13f,.43f,.48f,.39f,false);
            }
            break;
        }
        case WeaponKind::MissileBattery: {
            // Weather-sealed launcher cradle, guide rails and
            // constricting bands make these recognizable rocket pods.
            Housing(emit,Add(rear,Scale(aim,.43f)),right,up,aim,
                    .42f,.33f,.18f,.27f,.34f,.29f);
            for(int row : {-1,1})for(int col : {-1,1}) {
                const Vec offset=Add(Scale(right,.255f*row),
                                     Scale(up,.205f*col));
                Tube(emit,Add(rear,Add(Scale(aim,.82f),offset)),
                     Add(rear,Add(Scale(aim,.88f),offset)),right,up,
                     .130f,.130f,.51f,.50f,.38f,false);
            }
            Housing(emit,Add(rear,Add(Scale(aim,.16f),Scale(up,-.38f))),
                    right,up,aim,.25f,.13f,.30f,.34f,.36f,.32f);
            // Four unmistakable rocket canisters in a 2x2 array. Their
            // actual open, dark muzzle bores face the aircraft. The small
            // central aiming tube still aligns with the launch ray.
            for(int side : {-1,1})for(int tier : {-1,1}) {
                const Vec offset=Add(Scale(right,.255f*side),
                                     Scale(up,.205f*tier));
                Tube(emit,Add(rear,Add(Scale(aim,.28f),offset)),
                     Add(rear,Add(Scale(aim,length-.045f),offset)),
                     right,up,.108f,.102f,.31f,.36f,.29f);
            }
            // Launcher crossbraces visibly join the four canisters, rather
            // than pretending the missile mount is another gun barrel.
            for(int station=0;station<2;++station) {
                const Vec center=Add(rear,Scale(aim,station?.89f:.39f));
                Tube(emit,Add(center,Scale(right,-.32f)),
                     Add(center,Scale(right,.32f)),aim,up,
                     .048f,.048f,.36f,.39f,.31f,false);
            }
            break;
        }
        case WeaponKind::DirectCannon: {
            // Giant rectangular breech ring and curved ammunition
            // slide make the direct cannon a gun, not a thick pipe.
            Housing(emit,Add(rear,Scale(aim,.28f)),right,up,aim,
                    .34f,.31f,.36f,.29f,.34f,.33f);
            Housing(emit,Add(rear,Add(Scale(aim,.52f),Scale(up,-.26f))),
                    right,up,aim,.29f,.075f,.31f,.43f,.42f,.34f);
            Housing(emit,Add(rear,Add(Scale(aim,.05f),Scale(up,.26f))),
                    right,up,aim,.21f,.07f,.15f,.48f,.46f,.36f);
            Tube(emit,Add(rear,Scale(aim,.54f)),
                 Add(rear,Scale(aim,.59f)),right,up,.23f,.23f,
                 .55f,.52f,.40f,false);
            // Enormous bore, paired hydropneumatic recoil tubes, and two
            // distinct enlarged muzzle-brake chambers; not a missile pod.
            for(int station=0;station<2;++station) {
                const float offset=station?.23f:.43f;
                Tube(emit,Add(tip,Scale(aim,-offset-.095f)),
                     Add(tip,Scale(aim,-offset)),right,up,
                     .235f,.225f,.48f,.47f,.39f);
            }
            for(int side : {-1,1}) {
                const Vec offset=Scale(right,.31f*side);
                Tube(emit,Add(rear,offset),
                     Add(rear,Add(Scale(aim,.82f),offset)),right,up,
                     .062f,.051f,.44f,.45f,.38f,false);
            }
            break;
        }
    }
    // Independent trunnion blocks, lower turntable and rear recoil bed.
    Housing(emit,{0.f,-.93f,mount.z+.08f},{1.f,0.f,0.f},
            {0.f,1.f,0.f},{0.f,0.f,-1.f},
            .38f,.17f,.45f,.26f,.30f,.29f);
    Housing(emit,{0.f,-.67f,mount.z-.08f},{1.f,0.f,0.f},
            {0.f,1.f,0.f},{0.f,0.f,-1.f},
            .56f,.10f,.22f,.36f,.40f,.34f);
    for(int side : {-1,1}) {
        const Vec offset={.42f*side,-.76f,mount.z-.14f};
        Housing(emit,offset,{1.f,0.f,0.f},{0.f,1.f,0.f},
                {0.f,0.f,-1.f},.10f,.27f,.37f,.31f,.35f,.31f);
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
// Selectable real miniature artillery models mounted in a cabinet left of
// the seated player. This does not change the combat gun, ammunition or saves.
// Player's LEFT bunker wall, behind the firing slit and inland of beach.
constexpr float kRackX=-1.85f,kRackZ=.48f,kRackTop=.55f;
constexpr float kRackStep=.35f,kRackHalfX=.49f,kRackHalfY=.151f,
                kRackHalfZ=.32f;
struct RackSlot {float x,y,z;};
inline RackSlot RackLocation(uint8_t id) {
    return {kRackX,kRackTop-static_cast<float>(id)*kRackStep,kRackZ};
}
// The selectable box encloses the same world-space mesh that is drawn.
inline int HitRack(const float *origin,const float *direction) {
    if(!origin || !direction)return -1;
    for(int axis=0;axis<3;++axis)
        if(!std::isfinite(origin[axis]) || !std::isfinite(direction[axis]))
            return -1;
    int found=-1;
    float nearest=40.f;
    for(uint8_t id=0;id<kWeaponCount;++id) {
        const RackSlot slot=RackLocation(id);
        const float center[]={slot.x,slot.y,slot.z};
        const float half[]={kRackHalfX,kRackHalfY,kRackHalfZ};
        float enter=0.f,exit=40.f;
        bool valid=true;
        for(int axis=0;axis<3;++axis) {
            const float lo=center[axis]-half[axis];
            const float hi=center[axis]+half[axis];
            const float d=direction[axis],o=origin[axis];
            if(std::abs(d)<.000001f) {
                if(o<lo || o>hi){valid=false;break;}
                continue;
            }
            float a=(lo-o)/d,b=(hi-o)/d;
            if(a>b)std::swap(a,b);
            enter=std::max(enter,a);
            exit=std::min(exit,b);
            if(enter>exit){valid=false;break;}
        }
        if(valid && exit>=enter && enter<nearest) {
            nearest=enter;found=id;
        }
    }
    return found;
}
template<typename Emit>
inline void RackModel(Emit emit,WeaponKind kind) {
    const uint8_t id=static_cast<uint8_t>(kind);
    if(!WeaponValid(id))return;
    const RackSlot slot=RackLocation(id);
    const auto transform=[&](float &x,float &y,float &z) {
        const float originalX=x,originalZ=z;
        // Rotate previews toward the seated operator (+X), not out to sea.
        x=slot.x+(-originalZ-.06f)*.21f;
        y=slot.y+(y+.84f)*.21f;
        z=slot.z+originalX*.21f;
    };
    constexpr float forward[]={0.f,0.f,-1.f};
    // Same five physical gunmeshes as the player's cannon, scaled only.
    Build([&](float x0,float y0,float z0,float x1,float y1,float z1,
              float x2,float y2,float z2,float r,float g,float b) {
        transform(x0,y0,z0);
        transform(x1,y1,z1);
        transform(x2,y2,z2);
        emit(x0,y0,z0,x1,y1,z1,x2,y2,z2,r,g,b);
    },kind,forward);
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
