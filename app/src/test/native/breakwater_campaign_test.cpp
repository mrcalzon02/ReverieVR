#include "breakwater_campaign.h"
#include <cassert>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    Campaign c(Difficulty::Casual, 41);
    assert(c.Start() && c.phase() == Phase::Shop);
    assert(c.waves_this_period() == 2);
    assert(c.Purchase(Upgrade::FireRate));
    assert(c.stats().fire_delay_ms == 265);
    assert(c.Purchase(Upgrade::Magazine));
    assert(c.stats().magazine == 16);
    assert(c.BeginWave());
    for (int i=0;i<50;i++) c.Tick(0.05f);
    bool saw_rowboat=false;
    for (const Enemy &e:c.enemies()) {
        if (e.active) {
            assert(e.type == UnitType::Rowboat && e.passengers == 1);
            saw_rowboat=true;
        }
    }
    assert(saw_rowboat);
    Campaign sunk(Difficulty::Casual,41);
    assert(sunk.Start() && sunk.BeginWave());
    for (int i=0;i<180;i++) {
        sunk.Tick(0.05f);
        for (uint8_t n=0;n<sunk.enemies().size();++n) {
            if (sunk.enemies()[n].active) sunk.FireAt(n);
        }
        if (sunk.phase()!=Phase::Combat) break;
    }
    assert(sunk.wave_status().boats_sunk>=1);
    assert(sunk.wave_status().troops_landed==0);
    assert(sunk.infantry()==0);

    Campaign landed(Difficulty::Casual,41);
    assert(landed.Start() && landed.BeginWave());
    for(int i=0;i<4500 && landed.wave_status().troops_landed==0;i++)
        landed.Tick(0.05f);
    assert(landed.wave_status().troops_landed>0);
    assert(landed.infantry()>0);
    const int32_t integrity=landed.integrity();
    for(int i=0;i<82;i++) landed.Tick(0.05f);
    assert(landed.integrity()<integrity);
    while(landed.infantry()) {
        landed.Tick(0.05f);
        landed.FireAtInfantry();
    }
    assert(landed.wave_status().troops_stopped>=1);
    assert(landed.waves_this_period()==2);

    Campaign veteran(Difficulty::Siege,7);
    assert(veteran.Start());
    assert(veteran.waves_this_period()==5);
    assert(veteran.price(Upgrade::AntiAir)>0);
    assert(!veteran.FireAt(0));
    assert(veteran.Purchase(Upgrade::AntiAir));
    assert(veteran.stats().anti_air);
    assert(!veteran.Purchase(Upgrade::AntiAir));
    assert(veteran.Purchase(Upgrade::Artillery));
    assert(veteran.Purchase(Upgrade::Airstrike)==false); // insufficient credits
    assert(veteran.BeginWave());
    assert(veteran.CallArtillery());
    assert(!veteran.CallArtillery());
    assert(!veteran.Purchase(Upgrade::Damage));

    Campaign turret(Difficulty::Casual,41);
    assert(turret.Start());
    assert(turret.Purchase(Upgrade::SupportTurret));
    assert(turret.stats().turret);
    assert(turret.BeginWave());
    for(int i=0;i<160;i++) turret.Tick(0.05f);
    assert(turret.wave_status().boats_sunk>=1);
    assert(turret.wave_status().troops_landed==0);

    // Snapshot only stable shop state. Corrupt/mismatched snapshots must
    // leave the live campaign untouched, including purchased upgrades.
    Campaign writer(Difficulty::Veteran,1234);
    assert(writer.Start());
    assert(writer.Purchase(Upgrade::Damage));
    assert(writer.Purchase(Upgrade::Artillery));
    uint8_t encoded[Campaign::kSerializedSize]={};
    assert(writer.SerializeShop(encoded,sizeof(encoded)));
    Campaign restored;
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.phase()==Phase::Shop);
    assert(restored.credits()==writer.credits());
    assert(restored.level(Upgrade::Damage)==1);
    assert(restored.artillery_charges()==1);
    assert(restored.waves_this_period()==4);
    const int credit_before=restored.credits();
    encoded[0]^=0xffu;
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    encoded[0]^=0xffu;
    encoded[28]=0xffu; // invalid wave index, at word 7
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    assert(writer.BeginWave());
    assert(!writer.SerializeShop(encoded,sizeof(encoded)));

    // A location-stage fixture proves the military landing craft is an
    // actual transport containing 3–6 soldiers, not a decorative rowboat.
    Campaign stage(Difficulty::Casual,6);
    assert(stage.Start());
    uint8_t stage_bytes[Campaign::kSerializedSize]={};
    assert(stage.SerializeShop(stage_bytes,sizeof(stage_bytes)));
    stage_bytes[16]=1; // advance location from Shingle Bay to Iron Quay
    Campaign landing_stage;
    assert(landing_stage.DeserializeShop(stage_bytes,sizeof(stage_bytes)));
    assert(landing_stage.location()==1);
    assert(landing_stage.BeginWave());
    landing_stage.Tick(0.05f);
    const Enemy first_craft=landing_stage.enemies()[0];
    assert(first_craft.active && first_craft.type==UnitType::LandingCraft);
    assert(first_craft.passengers>=3 && first_craft.passengers<=6);
    for(int i=0;i<35;i++) {
        landing_stage.FireAt(0);
        landing_stage.Tick(0.05f);
    }
    assert(landing_stage.wave_status().boats_sunk>=1);
    assert(landing_stage.wave_status().troops_landed==0);

    // Day and night each contain their own shop-separated waves.
    Campaign clock(Difficulty::Casual,41);
    assert(clock.Start());
    for(int i=0;i<1800 && clock.period()!=Period::Night;i++) {
        if(clock.phase()==Phase::Shop) assert(clock.BeginWave());
        clock.Tick(0.05f);
        for(uint8_t n=0;n<clock.enemies().size();n++)
            if(clock.enemies()[n].active) clock.FireAt(n);
        if(clock.infantry()>0) clock.FireAtInfantry();
    }
    assert(clock.period()==Period::Night && clock.day()==1);
    assert(clock.wave_index()==1);
    assert(clock.waves_this_period()==1);

    assert(Campaign::Locations()[0].days==2);
    assert(Campaign::Locations()[1].days==3);
    assert(Campaign::Locations()[2].days==4);
    std::cout << "Breakwater boat / landing / upgrade smoke passed\n";
}
