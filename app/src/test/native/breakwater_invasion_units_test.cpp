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
    assert(SelectUnit(13,25,25,7,1)==UnitType::Gunboat);
    assert(SelectUnit(13,25,25,7,2)!=UnitType::Gunboat);
    assert(SelectUnit(9,25,25,7,1)==
           waves::SelectInvasionUnit(9,25,25,7));
    assert(invasion::FlagshipEscortLane(2.60f)>0.99f);
    assert(invasion::FlagshipEscortLane(2.60f)<1.01f);
    assert(invasion::FlagshipEscortLane(-2.60f)<-0.99f);
    assert(invasion::FlagshipEscortLane(-2.60f)>-1.01f);
    assert(SelectUnit(13,25,25,(1u<<16u)|7u,1)!=UnitType::ArmoredTransport);
    assert(SelectUnit(13,13,25,6,1)==UnitType::ArmoredTransport);
    assert(SelectUnit(13,12,25,6,1)!=UnitType::ArmoredTransport);
    // Regression: late gunboats and aircraft are never displaced by
    // routine heavy transports, including the highest pressure band.
    assert(SelectUnit(13,13,25,4,1)==UnitType::Gunboat);
    assert(SelectUnit(13,13,25,5,1)==UnitType::Aircraft);
    bool saw_heavy=false,saw_regular=false;
    for(uint8_t wave=13;wave<25;++wave)
        for(uint32_t high=0;high<16u;++high)
            for(uint32_t low=0;low<256u;++low) {
                const uint32_t roll=(high<<16u)|low;
                const UnitType base=waves::SelectInvasionUnit(
                    13u,wave,25u,roll);
                const UnitType chosen=SelectUnit(13,wave,25,roll,1);
                if(base==UnitType::Gunboat ||
                   base==UnitType::Aircraft)assert(chosen==base);
                if(chosen==UnitType::ArmoredTransport)saw_heavy=true;
                if(chosen==UnitType::LandingCraft)saw_regular=true;
            }
    assert(saw_heavy && saw_regular);
    assert(invasion::UnloadingSeconds(UnitType::ArmoredTransport)==1.25f);
    assert(invasion::UnloadingSeconds(UnitType::LandingCraft)==0.0f);
    static_assert(Campaign::kMaxArmor>=20u,"ten heavy ships carry 20 cars");
    static_assert(Campaign::kMaxInfantry>=60u,"ten craft carry 60 soldiers");
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
