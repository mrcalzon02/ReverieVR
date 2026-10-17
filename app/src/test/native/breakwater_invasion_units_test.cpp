#include "breakwater_invasion_units.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::breakwater;
int main() {
    using invasion::SelectUnit;
    // Preserve the entire opening day and ordinary mid-game introduction.
    for(uint8_t wave=1;wave<=25;++wave)
        for(uint32_t roll=0;roll<256;++roll) {
            assert(SelectUnit(0,wave,25,roll,0)==UnitType::Rowboat);
            assert(SelectUnit(1,wave,25,roll,0)==UnitType::Rowboat);
            assert(SelectUnit(9,wave,25,roll,0)!=UnitType::ArmoredTransport);
            if(wave<=12)
                assert(SelectUnit(12,wave,25,roll,0)!=UnitType::ArmoredTransport);
        }
    // Below the heavy threshold the new selector must delegate exactly
    // to the existing escalating pressure-band mix, not reset it.
    for(unsigned grade=3u;grade<10u;++grade)
        for(uint8_t wave=1u;wave<=25u;++wave)
            for(uint32_t roll=0;roll<64u;++roll)
                assert(SelectUnit(grade,wave,25u,roll,0u)==
                       waves::SelectInvasionUnit(grade,wave,25u,roll));
    assert(SelectUnit(13,25,25,7,0)==UnitType::ArmoredTransport);
    assert(SelectUnit(13,25,25,7,1)!=UnitType::ArmoredTransport);
    assert(SelectUnit(13,13,25,4,1)==UnitType::ArmoredTransport);
    assert(SelectUnit(13,12,25,4,1)!=UnitType::ArmoredTransport);
    assert(EnemyStartingHp(UnitType::ArmoredTransport)==320);
    assert(invasion::VehicleCargo(UnitType::ArmoredTransport,true)==2);
    assert(invasion::VehicleCargo(UnitType::LandingCraft,true)==1);
    assert(invasion::VehicleCargo(UnitType::LandingCraft,false)==0);
    assert(invasion::InfantryCargo(UnitType::ArmoredTransport,77)==4);
    assert(invasion::InfantryCargo(UnitType::Rowboat,77)==1);
    assert(invasion::HullDamage(UnitType::ArmoredTransport,
                                WeaponKind::MachineGun,20)==4);
    assert(invasion::HullDamage(UnitType::ArmoredTransport,
                                WeaponKind::HeavyGun,35)==11);
    assert(invasion::HullDamage(UnitType::ArmoredTransport,
                                WeaponKind::Flak,42)==10);
    assert(invasion::HullDamage(UnitType::ArmoredTransport,
                                WeaponKind::DirectCannon,90)==90);
    assert(invasion::HullDamage(UnitType::ArmoredTransport,
                                WeaponKind::MissileBattery,125)==125);
    assert(invasion::HullDamage(UnitType::LandingCraft,
                                WeaponKind::MachineGun,20)==20);
    assert(invasion::SupportDamage(UnitType::ArmoredTransport,120)==90);
    assert(invasion::SupportDamage(UnitType::ArmoredTransport,150)==112);
    assert(invasion::SupportDamage(UnitType::Gunboat,150)==150);
    std::cout<<"Late assault selection, armored hull and 2-car manifests OK\n";
}
