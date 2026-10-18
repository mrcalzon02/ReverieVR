#include "breakwater_cannon_trajectory.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace reverie::breakwater;
int main(){
    const float straight[]={0.f,0.f,-1.f};
    const float above[]={0.f,1.5f,-1.f};
    const float elevated[]={0.f,.56f,-1.f};
    const float nav[]={0.f,.20f,-1.f};
    const float bad[]={NAN,0.f,-1.f};
    trajectory::ShotRay ray{};
    assert(trajectory::CannonShot(WeaponKind::MachineGun,straight,&ray));
    assert(std::abs(ray.origin[0])<1.e-5f);
    assert(std::abs(ray.origin[1]+.53f)<1.e-5f);
    // The lowered aim used to clip the player's parapet; no mount may
    // point into the emplacement beneath its minimum safe declination.
    const float intoParapet[]={0.f,-.40f,-1.f};
    assert(!trajectory::CannonShot(WeaponKind::MachineGun,intoParapet,&ray));
    assert(ray.origin[2]> -1.34f && ray.origin[2]< -.4f);
    // The gun must not overhang the shore-firing line: infantry and
    // unloaded armor are shootable *in front of* the physical muzzle.
    gunmesh::Vec exact{};
    assert(gunmesh::Muzzle(WeaponKind::MachineGun,straight,0.f,&exact));
    assert(ray.origin[2]==exact.z);
    assert(std::abs(ray.direction[2]+1.f)<1.e-6f);
    for(int weapon=0;weapon<kWeaponCount;++weapon){
        assert(trajectory::CannonShot(static_cast<WeaponKind>(weapon),
                                      straight,&ray));
        gunmesh::Vec muzzle{};
        assert(gunmesh::Muzzle(static_cast<WeaponKind>(weapon),straight,
                               0.f,&muzzle));
        assert(std::abs(ray.origin[0]-muzzle.x)<1.e-6f &&
               std::abs(ray.origin[1]-muzzle.y)<1.e-6f &&
               std::abs(ray.origin[2]-muzzle.z)<1.e-6f);
    }
    for(int weapon=0;weapon<kWeaponCount;++weapon) {
        const auto kind=static_cast<WeaponKind>(weapon);
        assert(trajectory::CannonShot(kind,straight,&ray));
        // Historic muzzle at z=-2.3 placed both of these shore enemies
        // behind the shot; all current muzzle origins must reach them.
        const float landedInfantry[]={0.f,-.66f,-1.34f};
        const float landedArmor[]={0.f,-.72f,-1.56f};
        float nearHit=0.f;
        assert(ProjectileHitsSphere(ray.origin,ray.direction,
                                    WeaponFor(kind).gravity,
                                    landedInfantry,.30f,&nearHit));
        assert(nearHit>0.0f);
        assert(ProjectileHitsSphere(ray.origin,ray.direction,
                                    WeaponFor(kind).gravity,
                                    landedArmor,.43f,&nearHit));
        assert(nearHit>0.0f);
    }
    trajectory::ShotRay untouched=ray;
    assert(!trajectory::CannonShot(WeaponKind::DirectCannon,above,&ray));
    assert(ray.origin[2]==untouched.origin[2]); // no ghost firing path
    assert(!trajectory::CannonShot(WeaponKind::MachineGun,bad,&ray));
    assert(!trajectory::CannonShot(WeaponKind::MachineGun,straight,nullptr));
    assert(!gunmesh::Muzzle(WeaponKind::MachineGun,bad,0.f,&exact));
    assert(trajectory::CannonShot(WeaponKind::DirectCannon,nav,&ray));
    assert(!trajectory::CannonShot(WeaponKind::DirectCannon,elevated,&ray));
    assert(trajectory::CannonShot(WeaponKind::Flak,elevated,&ray));
    float at[3]={};
    assert(trajectory::Prediction(ray,WeaponFor(WeaponKind::Flak).gravity,
                                  7.f,at));
    assert(at[2]<-6.5f && at[2]>-8.0f); // post-reposition flak arc
    // The same cannon ray feeds hit checks and visible parabolic guide.
    const float hull[]={0.f,-.76f,-8.f};
    assert(trajectory::CannonShot(WeaponKind::MachineGun,straight,&ray));
    float depth=0.f;
    assert(!ProjectileHitsSphere(ray.origin,ray.direction,
                                 WeaponFor(WeaponKind::MachineGun).gravity,
                                 hull,.38f,&depth));
    float raised[]={0.f,.08f,-1.f};
    assert(trajectory::CannonShot(WeaponKind::MachineGun,raised,&ray));
    assert(ProjectileHitsSphere(ray.origin,ray.direction,
                                WeaponFor(WeaponKind::MachineGun).gravity,
                                hull,.75f,&depth));
    float contact[3]={};
    assert(trajectory::Prediction(ray,WeaponFor(WeaponKind::MachineGun).gravity,
                                  depth,contact));
    const float delta=(contact[0]-hull[0])*(contact[0]-hull[0])+
        (contact[1]-hull[1])*(contact[1]-hull[1])+
        (contact[2]-hull[2])*(contact[2]-hull[2]);
    assert(delta<=.75f*.75f+.0001f);
    // The controller origin is nowhere in this equation.
    std::cout<<"Breakwater muzzle geometry, in-arc targeting and impact agreement passed\n";
}
