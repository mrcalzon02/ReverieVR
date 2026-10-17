#include "breakwater_armor.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::breakwater;
int main() {
    // Nothing from early-day rowboats or ordinary troop carriers can
    // create a vehicle. Only seeded later-day landing craft carry armor.
    for(unsigned progression=0;progression<4;++progression)
        for(unsigned id=0;id<12;++id)
            assert(!armor::CarriesArmor(UnitType::LandingCraft,progression,
                                       1234u,id));
    for(UnitType kind:{UnitType::Rowboat,UnitType::Aircraft,UnitType::Gunboat})
        assert(!armor::CarriesArmor(kind,10,0,0));
    assert(armor::CarriesArmor(UnitType::LandingCraft,4,0,0));
    assert(!armor::CarriesArmor(UnitType::LandingCraft,4,0,1));
    assert(armor::TroopPassengers(6,true)==3);
    assert(armor::TroopPassengers(2,true)==2);
    assert(armor::TroopPassengers(6,false)==6);
    assert(armor::kVehicleHp==95);

    ArmoredVehicle vehicle{.4f,-2.23f,armor::kVehicleHp,true,1.8f};
    for(int i=0;i<200;++i)armor::Advance(vehicle,.05f);
    assert(std::abs(vehicle.z-armor::kArmorLineZ)<.0001f);
    assert(armor::Ready(vehicle));
    vehicle.attack_timer=armor::kArmorFireInterval;
    assert(!armor::Ready(vehicle));
    armor::Advance(vehicle,1.65f);
    assert(!armor::Ready(vehicle));
    armor::Advance(vehicle,1.65f);
    assert(armor::Ready(vehicle));
    assert(armor::SecondsToFire(vehicle)==0.0f);
    const float before=vehicle.z;
    armor::Advance(vehicle,-1.0f);
    assert(vehicle.z==before);
    vehicle.active=false;
    armor::Advance(vehicle,.20f);
    assert(vehicle.z==before);
    assert(!armor::Ready(vehicle));
    assert(armor::SecondsToFire(vehicle)<0.0f);

    // The HUD countdown comes from both the physical movement to the
    // armored firing line AND the real cooldown, never a decorative timer.
    ArmoredVehicle approaching{.4f,-2.23f,armor::kVehicleHp,true,1.8f};
    const float travel=(-1.56f+2.23f)/armor::kArmorSpeed;
    assert(std::abs(armor::SecondsToFire(approaching)-travel)<.001f);
    armor::Advance(approaching,1.0f);
    assert(std::abs(armor::SecondsToFire(approaching)-(travel-1.0f))<.001f);
    armor::Advance(approaching,1.5f);
    assert(armor::Ready(approaching));
    assert(armor::SecondsToFire(approaching)==0.0f);
    approaching.attack_timer=armor::kArmorFireInterval;
    assert(std::abs(armor::SecondsToFire(approaching)-3.3f)<.001f);
    approaching.hp=0;
    assert(armor::SecondsToFire(approaching)<0.0f);
    approaching.hp=armor::kVehicleHp;
    approaching.z=NAN;
    assert(armor::SecondsToFire(approaching)<0.0f);

    assert(armor::ArmorDamage(WeaponKind::MachineGun,20)==5);
    assert(armor::ArmorDamage(WeaponKind::HeavyGun,35)==17);
    assert(armor::ArmorDamage(WeaponKind::Flak,42)==14);
    assert(armor::ArmorDamage(WeaponKind::DirectCannon,90)==90);
    assert(armor::ArmorDamage(WeaponKind::MissileBattery,125)==125);

    vehicle.active=true;
    vehicle.hp=armor::kVehicleHp;
    const float origin[]={.4f,-.72f,.1f};
    const float direction[]={0.f,0.f,-1.f};
    float t=0;
    assert(armor::Hits(vehicle,origin,direction,0.f,&t));
    assert(t>1.0f && t<3.0f);
    const float away[]={2.f,0.f,-1.f};
    assert(!armor::Hits(vehicle,origin,away,0.f,&t));
    assert(!armor::Hits(vehicle,origin,direction,-.2f,&t));
    vehicle.hp=0;
    assert(!armor::Hits(vehicle,origin,direction,0.f,&t));
    std::cout<<"Breakwater armored-craft cargo, independent vehicle attack and ballistics OK\n";
}
