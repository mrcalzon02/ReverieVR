#include "omega_front_faction_encounters.h"
#include "omega_front_checkpoint.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Reseal(Checkpoint::Bytes &data){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<data.size()-8;++i){h^=data[i];h*=UINT64_C(1099511628211);}
    for(size_t i=data.size()-8;i<data.size();++i){data[i]=static_cast<uint8_t>(h);h>>=8;}
}
int main(){
    // Four source-built, genuinely combat-capable encounter shapes; hostile
    // faction identities are the existing F01/F02/F04/F06, not new factions.
    for(const auto faction:{Doctrine::Hearthward,Doctrine::Meridian,
                            Doctrine::Scrapline,Doctrine::LostNinth}){
        Battle b(31),same(31);
        assert(FactionEncounters::Build(faction,900,&b));
        assert(FactionEncounters::Build(faction,900,&same));
        assert(b.doctrine()==faction && b.unit_count()>=7);
        assert(b.Checksum()==same.Checksum());
        assert(!b.ConfigureDoctrine(static_cast<Doctrine>(19)));
        for(int t=0;t<25;++t){b.Step();same.Step();assert(b.Checksum()==same.Checksum());}
    }
    Battle ignored;
    const uint64_t untouched=ignored.Checksum();
    assert(!FactionEncounters::Build(Doctrine::Unaligned,12,&ignored));
    assert(!FactionEncounters::Build(static_cast<Doctrine>(19),12,&ignored));
    assert(!FactionEncounters::Build(Doctrine::Hearthward,12,nullptr));
    assert(untouched==ignored.Checksum());

    // F01 watch holds a position near the orchard rather than racing the pump.
    Battle watch(91);watch.ClearUnits();
    assert(watch.ConfigureDoctrine(Doctrine::Hearthward));
    assert(watch.ConfigureMission(Mission::CaptureOrchard));
    assert(watch.AddUnit(Side::Hearthwell,Class::Engineer,2,13)==0);
    assert(watch.AddUnit(Side::Raiders,Class::Rifle,18,12)==1);
    watch.Step();assert(watch.unit(1)->x==18 && watch.unit(1)->y==12);

    // F02 targets an exposed field engineer over a closer ordinary rifleman,
    // but only when both are actually visible. Unaligned uses closest.
    Battle trained(10),looters(10);
    for(Battle *b:{&trained,&looters}){
        b->ClearUnits();
        assert(b->AddUnit(Side::Hearthwell,Class::Rifle,11,12)==0);
        assert(b->AddUnit(Side::Hearthwell,Class::Engineer,12,9)==1);
        assert(b->AddUnit(Side::Raiders,Class::Sentinel,12,12)==2);
    }
    assert(trained.ConfigureDoctrine(Doctrine::Meridian));
    trained.Step();looters.Step();
    assert(trained.last_event().target==1);
    assert(looters.last_event().target==0);
    assert(trained.unit(2)->ammo==5);

    // F04 steals finite, existing ammunition: depot falls by exactly one
    // whenever a salvager at the crate acquires exactly one round.
    Battle theft(88);theft.ClearUnits();
    assert(theft.ConfigureDoctrine(Doctrine::Scrapline));
    theft.AddUnit(Side::Hearthwell,Class::Rifle,0,0);
    theft.AddUnit(Side::Raiders,Class::Salvager,6,12);
    assert(theft.unit(1)->ammo==2);
    const int stock=theft.ammo_stock();
    for(int i=1;i<=4;++i){
        theft.Step();
        assert(theft.ammo_stock()==stock-i);
        assert(theft.unit(1)->ammo==2+i);
        assert(theft.stolen_rounds()==i);
    }
    // A different faction on the identical map cannot extract the depot.
    Battle noTheft(88);noTheft.ClearUnits();
    noTheft.AddUnit(Side::Hearthwell,Class::Rifle,0,0);
    noTheft.AddUnit(Side::Raiders,Class::Salvager,6,12);
    noTheft.Step();assert(noTheft.ammo_stock()==stock);
    assert(noTheft.stolen_rounds()==0);

    // F06 carries no firearm ammunition, only shielded melee infantry.
    Battle legion(17);legion.ClearUnits();
    assert(legion.ConfigureDoctrine(Doctrine::LostNinth));
    legion.AddUnit(Side::Hearthwell,Class::Rifle,5,12);
    legion.AddUnit(Side::Raiders,Class::Legionary,6,12);
    assert(legion.unit(1)->ammo==0);
    legion.Step();assert(legion.unit(0)->hp<9);
    assert(legion.unit(1)->ammo==0);
    // Adjacent legionary formation members provide +2 protection against
    // the same shot in an otherwise identical seeded battlefield.
    Battle solo(6),shield(6);
    for(Battle *b:{&solo,&shield}){
        b->ClearUnits();
        b->AddUnit(Side::Hearthwell,Class::Rifle,5,12);
        b->AddUnit(Side::Raiders,Class::Legionary,6,12);
    }
    assert(shield.AddUnit(Side::Raiders,Class::Legionary,6,13)==2);
    assert(solo.QueueAttack(0,1) && shield.QueueAttack(0,1));
    solo.Step();shield.Step();
    assert(shield.unit(1)->hp==solo.unit(1)->hp+2);

    // An allied legionary can issue a legitimate melee order with 0 bullets,
    // but cannot request depot resupply for a non-ranged weapon.
    Battle auxiliary(9);auxiliary.ClearUnits();
    auxiliary.AddUnit(Side::Hearthwell,Class::Legionary,5,12);
    auxiliary.AddUnit(Side::Raiders,Class::Engineer,6,12);
    assert(!auxiliary.QueueResupply(0));
    assert(auxiliary.QueueAttack(0,1));
    auxiliary.Step();
    assert(auxiliary.unit(1)->hp<9 && auxiliary.unit(0)->ammo==0);

    // Meridian's engineer-priority doctrine must not magically shoot a
    // hidden target through an actual wall.
    Battle screened(11);screened.ClearUnits();
    assert(screened.SetGround(9,11,Ground::Wall));
    screened.AddUnit(Side::Hearthwell,Class::Rifle,8,12);
    screened.AddUnit(Side::Hearthwell,Class::Engineer,10,11);
    screened.AddUnit(Side::Raiders,Class::Sentinel,8,11);
    assert(screened.ConfigureDoctrine(Doctrine::Meridian));
    screened.Step();
    assert(screened.last_event().target==0);

    // New roles, doctrine, and transferred stock replay identically in v3.
    Controller saved(773);
    saved.battle=theft;
    Checkpoint::Bytes bytes{};
    assert(Checkpoint::Encode(saved,&bytes));
    assert(bytes[8]==3 && bytes[9]==0);
    Controller loaded(1);
    assert(Checkpoint::Decode(&loaded,bytes.data(),bytes.size()));
    assert(loaded.battle.Checksum()==saved.battle.Checksum());
    for(int i=0;i<6;++i){saved.battle.Step();loaded.battle.Step();
       assert(loaded.battle.Checksum()==saved.battle.Checksum());}
    // Correctly resealed illegal doctrine and non-Scrapline stolen stock fail.
    auto corrupt=bytes;corrupt[1077]=17;Reseal(corrupt);
    assert(!Checkpoint::Decode(&loaded,corrupt.data(),corrupt.size()));
    corrupt=bytes;corrupt[1077]=2;Reseal(corrupt);
    assert(!Checkpoint::Decode(&loaded,corrupt.data(),corrupt.size()));
    // V2 restoration retains all original mission/units and defaults to
    // unaligned historical AI without creating stolen equipment.
    Controller old(13);
    assert(Checkpoint::Encode(old,&bytes));
    bytes[8]=2;bytes[9]=0;bytes[1077]=0;bytes[1078]=0;Reseal(bytes);
    assert(Checkpoint::Decode(&loaded,bytes.data(),bytes.size()));
    assert(loaded.battle.doctrine()==Doctrine::Unaligned);
    assert(loaded.battle.stolen_rounds()==0);
    assert(loaded.battle.Checksum()==old.battle.Checksum());
    std::cout<<"Omega Front faction doctrine, finite salvage and v2 migration regressions passed\n";
}
